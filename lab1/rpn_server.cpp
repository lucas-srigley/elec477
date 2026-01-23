#include "rpn_server.hpp"
#include "rpn.pb.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define MAGIC_NUMBER 0x52504E43
#define VERSION 1
#define BUFFER_SIZE 1024

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

void run_server(uint16_t port) {
    RPNCalculator calc;
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket to port " << port << std::endl;
        close(sockfd);
        return;
    }
    
    std::cout << "Server listening on port " << port << std::endl;
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len < 0) {
            std::cerr << "Error receiving message" << std::endl;
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
        
        sendto(sockfd, response_data.c_str(), response_data.size(), 0, (struct sockaddr*)&client_addr, client_len);
    }
    
    close(sockfd);
}