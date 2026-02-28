#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Test 2: State changes on primary are forwarded to replica" << std::endl << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    
    RPNClient primary(3601);
    RPNClient replica(3602);
    
    std::cout << "Push 100.0 via primary" << std::endl;
    primary.push(100.0f);
    GetResult result1 = primary.read();
    std::cout << "Primary read: " << result1.value << std::endl;
    
    std::cout << std::endl << "Read through replica (redirects to primary)" << std::endl;
    GetResult result2 = replica.read();
    std::cout << "Replica read: " << result2.value << std::endl;
    
    if ((result1.value > 99.9f && result1.value < 100.1f) &&
        (result2.value > 99.9f && result2.value < 100.1f)) {
        std::cout << "Pass: replica has same state as primary" << std::endl;
    } else {
        std::cout << "Fail: expected 100.0, got " << result2.value << std::endl;
    }
    
    return 0;
}