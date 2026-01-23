#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Test 1: Push and read a value" << std::endl;
    
    RPNClient client(port);
    
    std::cout << "Push 2.0" << std::endl;
    bool pushResult = client.push(2.0f);
    std::cout << "Push: " << (pushResult ? "pass" : "fail") << std::endl;
    
    GetResult readResult = client.read();
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Read: " << readResult.value << std::endl;
    
    if (readResult.status && readResult.value == 2.0f) {
        std::cout << "Pass: got 2.0 back" << std::endl;
    } else {
        std::cout << "Fail: expected 2.0, got " << readResult.value << std::endl;
    }
    
    std::cout << std::endl << "Pop stack" << std::endl;
    bool popResult = client.pop();
    std::cout << "Pop: " << (popResult ? "pass" : "fail") << std::endl;
    
    readResult = client.read();
    std::cout << "Read after pop: " << readResult.value << std::endl;
    
    if (readResult.status && readResult.value == 0.0f) {
        std::cout << "Pass: stack now 0" << std::endl;
    }
    
    return 0;
}