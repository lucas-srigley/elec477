#include "rpn_client.h"
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
#define TIMEOUT_SEC 2

RPNClient::RPNClient(uint16_t port) : server_port(port), message_counter(0) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket" << std::endl;
    }
    
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

RPNClient::~RPNClient() {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

bool RPNClient::push(float value) {
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::PushRequest* req = request.mutable_push_req();
    req->set_value(value);
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            std::cerr << "Timeout or error receiving response" << std::endl;
            return false;
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
            return response.push_resp().status();
        }
    }
    
    return false;
}

bool RPNClient::pop() {
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::PopRequest* req = request.mutable_pop_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            return false;
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
            return response.pop_resp().status();
        }
    }
    
    return false;
}

RPNValueResult RPNClient::read() {
    RPNValueResult result = {false, 0.0f};
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::ReadRequest* req = request.mutable_read_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            return result;
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
            return result;
        }
    }
    
    return result;
}

bool RPNClient::swap() {
    message_counter++;
    
    rpn::RPCMessage request;
    request.set_magic(MAGIC_NUMBER);
    request.set_version(VERSION);
    request.set_message_id(message_counter);
    
    rpn::SwapRequest* req = request.mutable_swap_req();
    
    std::string request_data;
    request.SerializeToString(&request_data);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            return false;
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
            return response.swap_resp().status();
        }
    }
    
    return false;
}

RPNValueResult RPNClient::operation(char op) {
    RPNValueResult result = {false, 0.0f};
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
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(server_port);
    
    sendto(sockfd, request_data.c_str(), request_data.size(), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    while (true) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        
        ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr*)&from_addr, &from_len);
        
        if (recv_len < 0) {
            return result;
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
            return result;
        }
    }
    
    return result;
}

RPNValueResult RPNClient::add() {
    return operation('+');
}

RPNValueResult RPNClient::subtract() {
    return operation('-');
}

RPNValueResult RPNClient::multiply() {
    return operation('*');
}

RPNValueResult RPNClient::divide() {
    return operation('/');
}