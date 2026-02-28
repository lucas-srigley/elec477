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
#include <vector>

#define MAGIC_NUMBER 0x52504E43
#define VERSION 1
#define BUFFER_SIZE 4096

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

static bool resolveAddr(const std::string& host, uint16_t port, struct sockaddr_in& addr) {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) return false;
    addr.sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return true;
}

static bool isStateChanging(const rpn::RPCMessage& req) {
    return req.has_push_req() || req.has_pop_req() ||
           req.has_swap_req() || req.has_op_req();
}

static void applyToCalc(RPNCalculator& calc, const rpn::RPCMessage& req, rpn::RPCMessage& resp) {
    if (req.has_push_req()) {
        bool status = calc.push(req.push_req().value());
        resp.mutable_push_resp()->set_status(status);
    } else if (req.has_pop_req()) {
        bool status = calc.pop();
        resp.mutable_pop_resp()->set_status(status);
    } else if (req.has_read_req()) {
        float val;
        bool status = calc.read(val);
        resp.mutable_read_resp()->set_status(status);
        resp.mutable_read_resp()->set_value(val);
    } else if (req.has_swap_req()) {
        bool status = calc.swap();
        resp.mutable_swap_resp()->set_status(status);
    } else if (req.has_op_req()) {
        char op;
        switch(req.op_req().op()) {
            case rpn::ADD:      op = '+'; break;
            case rpn::SUBTRACT: op = '-'; break;
            case rpn::MULTIPLY: op = '*'; break;
            case rpn::DIVIDE:   op = '/'; break;
            default:            op = '+'; break;
        }
        float result;
        bool status = calc.operation(op, result);
        resp.mutable_op_resp()->set_status(status);
        resp.mutable_op_resp()->set_value(result);
    }
}

static void forwardToReplicas(int sockfd, const rpn::RPCMessage& originalReq,
                               const std::vector<uint16_t>& replicaPorts) {
    if (replicaPorts.empty()) return;

    rpn::RPCMessage fwd;
    fwd.set_magic(MAGIC_NUMBER);
    fwd.set_version(VERSION);
    fwd.set_message_id(originalReq.message_id());

    rpn::ReplicaUpdateRequest* upd = fwd.mutable_replica_update_req();
    if (originalReq.has_push_req())
        *upd->mutable_push_req() = originalReq.push_req();
    else if (originalReq.has_pop_req())
        upd->mutable_pop_req();
    else if (originalReq.has_swap_req())
        upd->mutable_swap_req();
    else if (originalReq.has_op_req())
        *upd->mutable_op_req() = originalReq.op_req();
    else
        return;

    std::string fwdData;
    fwd.SerializeToString(&fwdData);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (uint16_t rport : replicaPorts) {
        struct sockaddr_in raddr;
        if (!resolveAddr("localhost", rport, raddr)) continue;

        sendto(sockfd, fwdData.c_str(), fwdData.size(), 0,
               (struct sockaddr*)&raddr, sizeof(raddr));

        char buf[BUFFER_SIZE];
        struct sockaddr_in fromAddr;
        socklen_t fromLen = sizeof(fromAddr);
        ssize_t n = recvfrom(sockfd, buf, BUFFER_SIZE, 0,
                             (struct sockaddr*)&fromAddr, &fromLen);
        if (n > 0) {
            rpn::RPCMessage ack;
            ack.ParseFromArray(buf, n);
        }
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void run_server(uint16_t port, const std::string& service_name, bool isPrimary,
                const std::string& primaryHost, uint16_t primaryPort,
                const std::vector<uint16_t>& replicaPorts) {
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
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Error binding socket to port " << port << std::endl;
        close(sockfd);
        return;
    }
    
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

        uint16_t srcPort = ntohs(client_addr.sin_port);
        
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

        if (!isPrimary) {
            if (request.has_replica_update_req() && srcPort == primaryPort) {
                const auto& upd = request.replica_update_req();
                rpn::RPCMessage synthetic;
                if (upd.has_push_req())      *synthetic.mutable_push_req() = upd.push_req();
                else if (upd.has_pop_req())   synthetic.mutable_pop_req();
                else if (upd.has_swap_req())  synthetic.mutable_swap_req();
                else if (upd.has_op_req())    *synthetic.mutable_op_req() = upd.op_req();
                rpn::RPCMessage dummy;
                applyToCalc(calc, synthetic, dummy);
                response.mutable_replica_update_resp()->set_status(true);
            } else {
                response.mutable_redirect_resp()->set_primary_host(primaryHost);
                response.mutable_redirect_resp()->set_primary_port(primaryPort);
            }

            std::string response_data;
            response.SerializeToString(&response_data);
            sendto(sockfd, response_data.c_str(), response_data.size(), 0,
                   (struct sockaddr*)&client_addr, client_len);
            continue;
        }

        if (isStateChanging(request)) {
            forwardToReplicas(sockfd, request, replicaPorts);
        }

        applyToCalc(calc, request, response);
        
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