#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Test 3: Validate all operations" << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    
    RPNClient client(port);
    
    std::cout << std::endl << "Add: 5.0 + 3.0" << std::endl;
    client.push(5.0f);
    client.push(3.0f);
    GetResult result = client.add();
    std::cout << "Result: " << result.value << std::endl;
    
    std::cout << std::endl << "Subtract: 10.0 - 4.0" << std::endl;
    client.push(10.0f);
    client.push(4.0f);
    result = client.subtract();
    std::cout << "Result: " << result.value << std::endl;
    
    std::cout << std::endl << "Multiply: 3.0 * 4.0" << std::endl;
    client.push(3.0f);
    client.push(4.0f);
    result = client.multiply();
    std::cout << "Result: " << result.value << std::endl;
    
    std::cout << std::endl << "Divide: 20.0 / 5.0" << std::endl;
    client.push(20.0f);
    client.push(5.0f);
    result = client.divide();
    std::cout << "Result: " << result.value << std::endl;
    
    std::cout << std::endl << "Swap: push 1.0, push 2.0, swap" << std::endl;
    client.push(1.0f);
    client.push(2.0f);
    result = client.read();
    std::cout << "Before swap: " << result.value << std::endl;
    client.swap();
    result = client.read();
    std::cout << "After swap: " << result.value << std::endl;
    
    return 0;
}