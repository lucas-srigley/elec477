#ifndef RPN_SERVER_HPP
#define RPN_SERVER_HPP

#include <cstdint>
#include <string>
#include <vector>

class RPNCalculator {
private:
    float stack[4];
    
public:
    RPNCalculator();
    bool push(float value);
    bool pop();
    bool read(float& value);
    bool swap();
    bool operation(char op, float& result);
};

void run_server(uint16_t port, const std::string& service_name, bool isPrimary,
                const std::string& primaryHost, uint16_t primaryPort,
                const std::vector<uint16_t>& replicaPorts);

#endif