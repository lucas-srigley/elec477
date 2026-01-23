#ifndef RPN_CLIENT_HPP
#define RPN_CLIENT_HPP

#include <cstdint>

struct GetResult {
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
    GetResult read();
    bool swap();
    GetResult add();
    GetResult subtract();
    GetResult multiply();
    GetResult divide();
    
private:
    GetResult operation(char op);
};

#endif