#include "rpn_client.hpp"
#include <iostream>
#include <iomanip>
#include <string>

int main(int argc, char* argv[]) {
    std::string service_name = "calc_server_1";
    
    if (argc > 1) {
        service_name = argv[1];
    }
    
    std::cout << "Test 3: Validate all operations" << std::endl;
    std::cout << "Using service: " << service_name << std::endl << std::endl;
    std::cout << std::fixed << std::setprecision(1);
    
    RPNClient client(service_name);
    
    std::cout << std::endl << "Add: 5.0 + 3.0" << std::endl;
    client.push(5.0f);
    client.push(3.0f);
    GetResult result = client.add();
    if (result.status && result.value == 8.0f) {
        std::cout << "Pass: 5.0 + 3.0 = 8.0" << std::endl;
    } else {
        std::cout << "Fail: expected 8.0, got " << result.value << std::endl;
    }

    std::cout << std::endl << "Subtract: 10.0 - 4.0" << std::endl;
    client.push(10.0f);
    client.push(4.0f);
    result = client.subtract();
    if (result.status && result.value == 6.0f) {
        std::cout << "Pass: 10.0 - 4.0 = 6.0" << std::endl;
    } else {
        std::cout << "Fail: expected 6.0, got " << result.value << std::endl;
    }

    std::cout << std::endl << "Multiply: 3.0 * 4.0" << std::endl;
    client.push(3.0f);
    client.push(4.0f);
    result = client.multiply();
    if (result.status && result.value == 12.0f) {
        std::cout << "Pass: 3.0 * 4.0 = 12.0" << std::endl;
    } else {
        std::cout << "Fail: expected 12.0, got " << result.value << std::endl;
    }

    std::cout << std::endl << "Divide: 20.0 / 5.0" << std::endl;
    client.push(20.0f);
    client.push(5.0f);
    result = client.divide();
    if (result.status && result.value == 4.0f) {
        std::cout << "Pass: 20.0 / 5.0 = 4.0" << std::endl;
    } else {
        std::cout << "Fail: expected 4.0, got " << result.value << std::endl;
    }

    std::cout << std::endl << "Swap: push 1.0, push 2.0, swap" << std::endl;
    client.push(1.0f);
    client.push(2.0f);
    result = client.read();
    std::cout << "Before swap: " << result.value << std::endl;
    client.swap();
    result = client.read();
    std::cout << "After swap: " << result.value << std::endl;
    if (result.status && result.value == 1.0f) {
        std::cout << "Pass: top is now 1.0 after swap" << std::endl;
    } else {
        std::cout << "Fail: expected 1.0 after swap, got " << result.value << std::endl;
    }

    std::cout << std::endl << "Push 4 values and pop 3 times" << std::endl;
    client.push(1.0f);
    client.push(2.0f);
    client.push(3.0f);
    client.push(4.0f);

    result = client.read();
    std::cout << "Top after 4 pushes: " << result.value << std::endl;
    if (result.status && result.value == 4.0f) {
        std::cout << "Pass: top is 4.0" << std::endl;
    } else {
        std::cout << "Fail: expected 4.0, got " << result.value << std::endl;
    }

    client.pop();
    result = client.read();
    std::cout << "Top after pop 1: " << result.value << std::endl;
    if (result.status && result.value == 3.0f) {
        std::cout << "Pass: top is 3.0" << std::endl;
    } else {
        std::cout << "Fail: expected 3.0, got " << result.value << std::endl;
    }

    client.pop();
    result = client.read();
    std::cout << "Top after pop 2: " << result.value << std::endl;
    if (result.status && result.value == 2.0f) {
        std::cout << "Pass: top is 2.0" << std::endl;
    } else {
        std::cout << "Fail: expected 2.0, got " << result.value << std::endl;
    }

    client.pop();
    result = client.read();
    std::cout << "Top after pop 3: " << result.value << std::endl;
    if (result.status && result.value == 1.0f) {
        std::cout << "Pass: top is 1.0" << std::endl;
    } else {
        std::cout << "Fail: expected 1.0, got " << result.value << std::endl;
    }

    client.pop();
    result = client.read();
    std::cout << "Top after pop 4: " << result.value << std::endl;
    if (result.status && result.value == 1.0f) {
        std::cout << "Pass: stack replicated" << std::endl;
    } else {
        std::cout << "Fail: expected 1.0, got " << result.value << std::endl;
    }

    return 0;
}