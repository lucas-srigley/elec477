#include "rpn_client.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Test 2: All Operations Test" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    RPNClient client(port);
    
    std::cout << "\n--- Testing Addition (5.3 + 7.2) ---" << std::endl;
    client.push(5.3f);
    std::cout << "Pushed 5.3" << std::endl;
    client.push(7.2f);
    std::cout << "Pushed 7.2" << std::endl;
    
    RPNValueResult add_result = client.add();
    std::cout << "Addition result: " << add_result.value << std::endl;
    if (add_result.status && (add_result.value > 12.49f && add_result.value < 12.51f)) {
        std::cout << "✓ Addition test PASSED" << std::endl;
    } else {
        std::cout << "✗ Addition test FAILED" << std::endl;
    }
    
    RPNValueResult read_result = client.read();
    std::cout << "Top of stack after add: " << read_result.value << std::endl;
    
    std::cout << "\n--- Testing Subtraction (10.0 - 3.5) ---" << std::endl;
    client.push(10.0f);
    std::cout << "Pushed 10.0" << std::endl;
    client.push(3.5f);
    std::cout << "Pushed 3.5" << std::endl;
    
    RPNValueResult sub_result = client.subtract();
    std::cout << "Subtraction result: " << sub_result.value << std::endl;
    if (sub_result.status && (sub_result.value > 6.49f && sub_result.value < 6.51f)) {
        std::cout << "✓ Subtraction test PASSED" << std::endl;
    } else {
        std::cout << "✗ Subtraction test FAILED" << std::endl;
    }
    
    std::cout << "\n--- Testing Multiplication (4.0 * 2.5) ---" << std::endl;
    client.push(4.0f);
    std::cout << "Pushed 4.0" << std::endl;
    client.push(2.5f);
    std::cout << "Pushed 2.5" << std::endl;
    
    RPNValueResult mul_result = client.multiply();
    std::cout << "Multiplication result: " << mul_result.value << std::endl;
    if (mul_result.status && (mul_result.value > 9.99f && mul_result.value < 10.01f)) {
        std::cout << "✓ Multiplication test PASSED" << std::endl;
    } else {
        std::cout << "✗ Multiplication test FAILED" << std::endl;
    }
    
    std::cout << "\n--- Testing Division (20.0 / 4.0) ---" << std::endl;
    client.push(20.0f);
    std::cout << "Pushed 20.0" << std::endl;
    client.push(4.0f);
    std::cout << "Pushed 4.0" << std::endl;
    
    RPNValueResult div_result = client.divide();
    std::cout << "Division result: " << div_result.value << std::endl;
    if (div_result.status && (div_result.value > 4.99f && div_result.value < 5.01f)) {
        std::cout << "✓ Division test PASSED" << std::endl;
    } else {
        std::cout << "✗ Division test FAILED" << std::endl;
    }
    
    std::cout << "\n--- Testing Swap ---" << std::endl;
    client.push(1.0f);
    std::cout << "Pushed 1.0" << std::endl;
    client.push(2.0f);
    std::cout << "Pushed 2.0" << std::endl;
    
    read_result = client.read();
    std::cout << "Top before swap: " << read_result.value << std::endl;
    
    client.swap();
    std::cout << "Swapped top two elements" << std::endl;
    
    read_result = client.read();
    std::cout << "Top after swap: " << read_result.value << std::endl;
    if (read_result.status && (read_result.value > 0.99f && read_result.value < 1.01f)) {
        std::cout << "✓ Swap test PASSED" << std::endl;
    } else {
        std::cout << "✗ Swap test FAILED" << std::endl;
    }
    
    std::cout << "\nAll Operations Test Complete" << std::endl;
    
    return 0;
}