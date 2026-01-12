#ifndef RPN_SERVER_H
#define RPN_SERVER_H

#include <cstdint>

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

void run_server(uint16_t port);

#endif