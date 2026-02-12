#include "rpn_server.hpp"
#include "ServiceServer/svcDirClient.hpp"
#include "rpn.pb.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <csignal>

#define MAGIC_NUMBER 0x52504E43
#define VERSION 1
#define BUFFER_SIZE 1024

static volatile bool server_running = true;
static std::string global_service_name;
static std::string global_hostname;
static uint16_t global_port;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    server_running = false;
}

RPNCalculator::RPNCalculator() {
    for (int i = 0; i < 4; i++) {
        stack[i] = 0.0f;
    }
}

bool RPNCalculator::push(float value) {
    for (int i = 3; i > 0; i--) {
        stack[i] = stack[i-1];
    }
    stack[0] = value;
    return true;
}

bool RPNCalculator::pop() {
    for (int i = 0; i < 3; i++) {
        stack[i] = stack[i+1];
    }
    return true;
}

bool RPNCalculator::read(float& value) {
    value = stack[0];
    return true;
}

bool RPNCalculator::swap() {
    float temp = stack[0];
    stack[0] = stack[1];
    stack[1] = temp;
    return true;
}

bool RPNCalculator::operation(char op, float& result) {
    float second = stack[0];
    float first = stack[1];
    
    switch(op) {
        case '+':
            result = first + second;
            break;
        case '-':
            result = first - second;
            break;
        case '*':
            result = first * second;
            break;
        case '/':
            if (second == 0.0f) {
                return false;
            }
            result = first / second;
            break;
        default:
            return false;
    }
    
    pop();
    pop();
    push(result);
    
    return true;
}

void run_server(uint16_t port, const std::string& service_name) {
    RPNCalculator calc;
    
    global_service_name = service_name;
    global_hostname = "localhost";
    global_port = port;
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        std::perror("sigaction");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        std::perror("sigaction");
    }
    
    svcDir::serviceServer svcServer;
    if (!service_name.empty()) {
        std::cout << "Registering service '" << service_name << "' at " 
                  << global_hostname << ":" << port << std::endl;
        
        svcDir::serverEntity server = {global_hostname, port};
        if (svcServer.registerService(service_name, server)) {
            std::cout << "Successfully registered with service directory" << std::endl;
        } else {
            std::cerr << "Failed to register with service directory" << std::endl;
        }
    }
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    std::string port_str = std::to_string(port);
    if (getaddrinfo("localhost", port_str.c_str(), &hints, &res) != 0) {
        std::cerr << "Error resolving localhost" << std::endl;
        close(sockfd);
        return;
    }
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Error binding socket to port " << port << std::endl;
        freeaddrinfo(res);
        close(sockfd);
        return;
    }
    
    freeaddrinfo(res);
    
    std::cout << "Server listening on port " << port << std::endl;
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    while (server_running) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len <= 0) {
            continue;
        }
        
        rpn::RPCMessage request;
        if (!request.ParseFromArray(buffer, recv_len)) {
            std::cerr << "Error parsing request" << std::endl;
            continue;
        }
        
        if (request.magic() != MAGIC_NUMBER) {
            std::cerr << "Invalid magic number" << std::endl;
            continue;
        }
        
        if (request.version() != VERSION) {
            std::cerr << "Invalid version" << std::endl;
            continue;
        }
        
        rpn::RPCMessage response;
        response.set_magic(MAGIC_NUMBER);
        response.set_version(VERSION);
        response.set_message_id(request.message_id());
        
        if (request.has_push_req()) {
            float value = request.push_req().value();
            bool status = calc.push(value);
            
            rpn::PushResponse* resp = response.mutable_push_resp();
            resp->set_status(status);
        }
        else if (request.has_pop_req()) {
            bool status = calc.pop();
            
            rpn::PopResponse* resp = response.mutable_pop_resp();
            resp->set_status(status);
        }
        else if (request.has_read_req()) {
            float value;
            bool status = calc.read(value);
            
            rpn::ReadResponse* resp = response.mutable_read_resp();
            resp->set_status(status);
            resp->set_value(value);
        }
        else if (request.has_swap_req()) {
            bool status = calc.swap();
            
            rpn::SwapResponse* resp = response.mutable_swap_resp();
            resp->set_status(status);
        }
        else if (request.has_op_req()) {
            char op;
            switch(request.op_req().op()) {
                case rpn::ADD: op = '+'; break;
                case rpn::SUBTRACT: op = '-'; break;
                case rpn::MULTIPLY: op = '*'; break;
                case rpn::DIVIDE: op = '/'; break;
                default: op = '+'; break;
            }
            
            float result;
            bool status = calc.operation(op, result);
            
            rpn::OperationResponse* resp = response.mutable_op_resp();
            resp->set_status(status);
            resp->set_value(result);
        }
        
        std::string response_data;
        response.SerializeToString(&response_data);
        
        sendto(sockfd, response_data.c_str(), response_data.size(), 0, 
               (struct sockaddr*)&client_addr, client_len);
    }
    
    if (!service_name.empty()) {
        std::cout << "Deregistering service '" << service_name << "'" << std::endl;
        svcDir::serverEntity server = {global_hostname, global_port};
        if (svcServer.removeService(service_name, server)) {
            std::cout << "Successfully deregistered from service directory" << std::endl;
        } else {
            std::cerr << "Failed to deregister from service directory" << std::endl;
        }
    }
    
    close(sockfd);
    std::cout << "Server shut down cleanly" << std::endl;
}