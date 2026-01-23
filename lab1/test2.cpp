#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port1 = 3600;
    uint16_t port2 = 3601;
    
    std::cout << "Test 2: Independent servers" << std::endl;
    std::cout << "Servers on ports " << port1 << " and " << port2 << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    
    RPNClient client1(port1);
    RPNClient client2(port2);
    
    std::cout << std::endl << "Server 1 (port " << port1 << ")" << std::endl;
    client1.push(100.0f);
    GetResult result1 = client1.read();
    std::cout << "Pushed 100.0, read: " << result1.value << std::endl;
    
    std::cout << std::endl << "Server 2 (port " << port2 << ")" << std::endl;
    client2.push(200.0f);
    GetResult result2 = client2.read();
    std::cout << "Pushed 200.0, read: " << result2.value << std::endl;
    
    std::cout << std::endl << "check independence" << std::endl;
    result1 = client1.read();
    result2 = client2.read();
    std::cout << "Server 1: " << result1.value << std::endl;
    std::cout << "Server 2: " << result2.value << std::endl;
    
    if ((result1.value > 99.9f && result1.value < 100.1f) &&
        (result2.value > 199.9f && result2.value < 200.1f)) {
        std::cout << "Pass: servers are independent" << std::endl;
    }
    
    return 0;
}