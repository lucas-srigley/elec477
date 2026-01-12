#ifndef RPN_CLIENT_H
#define RPN_CLIENT_H

#include <cstdint>

struct RPNValueResult {
    bool status;
    float value;
};

class RPNClient {
private:
    int sockfd;
    uint16_t server_port;
    uint64_t message_counter;
    
public:
    RPNClient(uint16_t port);
    ~RPNClient();
    
    bool push(float value);
    bool pop();
    RPNValueResult read();
    bool swap();
    RPNValueResult add();
    RPNValueResult subtract();
    RPNValueResult multiply();
    RPNValueResult divide();
    
private:
    RPNValueResult operation(char op);
};

#endif