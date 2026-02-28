#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main(int argc, char* argv[]) {
    std::string service_name = "calc_server";
    
    if (argc > 1) {
        service_name = argv[1];
    }
    
    std::cout << "Test 1: Client messages to replica are redirected to primary" << std::endl;
    std::cout << "Using service: " << service_name << std::endl << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    
    RPNClient client(3602);
    
    std::cout << "Push 2.0 to replica" << std::endl;
    bool pushResult = client.push(2.0f);
    std::cout << "Push: " << (pushResult ? "pass" : "fail") << std::endl;
    
    GetResult readResult = client.read();
    std::cout << "Read: " << readResult.value << std::endl;
    
    if (readResult.status && readResult.value == 2.0f) {
        std::cout << "Pass: got 2.0 back from primary" << std::endl;
    } else {
        std::cout << "Fail: expected 2.0, got " << readResult.value << std::endl;
    }
    
    return 0;
}