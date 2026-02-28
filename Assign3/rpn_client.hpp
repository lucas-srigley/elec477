#ifndef RPN_CLIENT_HPP
#define RPN_CLIENT_HPP

#include <cstdint>
#include <string>

struct GetResult {
    bool status;
    float value;
};

class RPNClient {
private:
    int sockfd;
    std::string server_hostname;
    uint16_t server_port;
    uint64_t message_counter;
    
public:
    RPNClient(const std::string& service_name);
    
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
    void init_socket();
    GetResult operation(char op);
    bool sendAndReceive(const std::string& requestData, uint64_t msgId, std::string& responseData);
};

#endif