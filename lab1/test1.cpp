#include "rpn_client.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Test 1: Basic Push and Read" << std::endl;
    std::cout << "Testing push(4.1) and read()" << std::endl << std::endl;
    
    RPNClient client(port);
    
    std::cout << "Pushing value 4.1..." << std::endl;
    bool push_result = client.push(4.1f);
    std::cout << "Push status: " << (push_result ? "SUCCESS" : "FAILED") << std::endl;
    
    std::cout << "Reading top of stack..." << std::endl;
    RPNValueResult read_result = client.read();
    std::cout << "Read status: " << (read_result.status ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Read value: " << read_result.value << std::endl;
    
    if (read_result.status && read_result.value == 4.1f) {
        std::cout << "\n✓ TEST PASSED: Value 4.1 was successfully pushed and read back" << std::endl;
    } else {
        std::cout << "\n✗ TEST FAILED: Expected 4.1, got " << read_result.value << std::endl;
    }
    
    std::cout << "\nPopping stack..." << std::endl;
    bool pop_result = client.pop();
    std::cout << "Pop status: " << (pop_result ? "SUCCESS" : "FAILED") << std::endl;
    
    read_result = client.read();
    std::cout << "Reading after pop..." << std::endl;
    std::cout << "Read value: " << read_result.value << std::endl;
    
    if (read_result.status && read_result.value == 0.0f) {
        std::cout << "✓ After pop, stack shows 0 as expected" << std::endl;
    }
    
    return 0;
}