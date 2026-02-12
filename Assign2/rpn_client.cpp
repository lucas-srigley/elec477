#include "rpn_client.hpp"
#include "ServiceServer/svcDirClient.hpp"
#include "rpn.pb.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define MAGIC_NUMBER 0x52504E43
#define VERSION 1
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 2

RPNClient::RPNClient(const std::string& service_name) : sockfd(-1), message_counter(0) {
    svcDir::serviceServer svcServer;
    svcDir::serverEntity serverInfo = svcServer.searchService(service_name);
    
    if (serverInfo.name == "None" || serverInfo.port == 0) {
        std::cerr << "Service '" << service_name << "' not found" << std::endl;
        server_hostname = "127.0.0.1";
        server_port = 0;
        return;
    }
    
    server_hostname = serverInfo.name;
    server_port = serverInfo.port;
    
    std::cout << "Found service '" << service_name << "' at " 
              << server_hostname << ":" << server_port << std::endl;
    
    init_socket();
}

RPNClient::RPNClient(uint16_t port) : sockfd(-1), server_port(port), message_counter(0) {
    server_hostname = "127.0.0.1";
    init_socket();
}

void RPNClient::init_socket() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd >= 0) {
        struct timeval tv;
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }
}

RPNClient::~RPNClient() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

bool RPNClient::push(float value) {
    if (sockfd < 0 || server_port == 0) return false;
    
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::PushRequest* req = request.mutable_push_req();
    req->set_value(value);
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    std::string port_str = std::to_string(server_port);
    if (getaddrinfo(server_hostname.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return false;
    }
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0, res->ai_addr, res->ai_addrlen);
    
    bool result = false;
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            break;
        }
        
        rpn::RPCMessage response;
        if (!response.ParseFromArray(buffer, recv_len)) {
            continue;
        }
        
        if (response.magic() != MAGIC_NUMBER || response.version() != VERSION) {
            continue;
        }
        
        if (response.message_id() != message_counter) {
            continue;
        }
        
        if (response.has_push_resp()) {
            result = response.push_resp().status();
            break;
        }
    }
    
    freeaddrinfo(res);
    return result;
}

bool RPNClient::pop() {
    if (sockfd < 0 || server_port == 0) return false;
    
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::PopRequest* req = request.mutable_pop_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    std::string port_str = std::to_string(server_port);
    if (getaddrinfo(server_hostname.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return false;
    }
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0, res->ai_addr, res->ai_addrlen);
    
    bool result = false;
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            break;
        }
        
        rpn::RPCMessage response;
        if (!response.ParseFromArray(buffer, recv_len)) {
            continue;
        }
        
        if (response.magic() != MAGIC_NUMBER || response.version() != VERSION) {
            continue;
        }
        
        if (response.message_id() != message_counter) {
            continue;
        }
        
        if (response.has_pop_resp()) {
            result = response.pop_resp().status();
            break;
        }
    }
    
    freeaddrinfo(res);
    return result;
}

GetResult RPNClient::read() {
    GetResult result = {false, 0.0f};
    if (sockfd < 0 || server_port == 0) return result;
    
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::ReadRequest* req = request.mutable_read_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    std::string port_str = std::to_string(server_port);
    if (getaddrinfo(server_hostname.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return result;
    }
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0, res->ai_addr, res->ai_addrlen);
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            break;
        }
        
        rpn::RPCMessage response;
        if (!response.ParseFromArray(buffer, recv_len)) {
            continue;
        }
        
        if (response.magic() != MAGIC_NUMBER || response.version() != VERSION) {
            continue;
        }
        
        if (response.message_id() != message_counter) {
            continue;
        }
        
        if (response.has_read_resp()) {
            result.status = response.read_resp().status();
            result.value = response.read_resp().value();
            break;
        }
    }
    
    freeaddrinfo(res);
    return result;
}

bool RPNClient::swap() {
    if (sockfd < 0 || server_port == 0) return false;
    
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::SwapRequest* req = request.mutable_swap_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    std::string port_str = std::to_string(server_port);
    if (getaddrinfo(server_hostname.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return false;
    }
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0, res->ai_addr, res->ai_addrlen);
    
    bool result = false;
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            break;
        }
        
        rpn::RPCMessage response;
        if (!response.ParseFromArray(buffer, recv_len)) {
            continue;
        }
        
        if (response.magic() != MAGIC_NUMBER || response.version() != VERSION) {
            continue;
        }
        
        if (response.message_id() != message_counter) {
            continue;
        }
        
        if (response.has_swap_resp()) {
            result = response.swap_resp().status();
            break;
        }
    }
    
    freeaddrinfo(res);
    return result;
}

GetResult RPNClient::operation(char op) {
    GetResult result = {false, 0.0f};
    if (sockfd < 0 || server_port == 0) return result;
    
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::OperationRequest* req = request.mutable_op_req();
    switch(op) {
        case '+': req->set_op(rpn::ADD); break;
        case '-': req->set_op(rpn::SUBTRACT); break;
        case '*': req->set_op(rpn::MULTIPLY); break;
        case '/': req->set_op(rpn::DIVIDE); break;
    }
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    std::string port_str = std::to_string(server_port);
    if (getaddrinfo(server_hostname.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return result;
    }
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0, res->ai_addr, res->ai_addrlen);
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            break;
        }
        
        rpn::RPCMessage response;
        if (!response.ParseFromArray(buffer, recv_len)) {
            continue;
        }
        
        if (response.magic() != MAGIC_NUMBER || response.version() != VERSION) {
            continue;
        }
        
        if (response.message_id() != message_counter) {
            continue;
        }
        
        if (response.has_op_resp()) {
            result.status = response.op_resp().status();
            result.value = response.op_resp().value();
            break;
        }
    }
    
    freeaddrinfo(res);
    return result;
}

GetResult RPNClient::add() {
    return operation('+');
}

GetResult RPNClient::subtract() {
    return operation('-');
}

GetResult RPNClient::multiply() {
    return operation('*');
}

GetResult RPNClient::divide() {
    return operation('/');
}