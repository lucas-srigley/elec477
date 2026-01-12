#include "rpn_client.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Test 3: Stack Depth and Pop Replication" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    RPNClient client(port);
    
    std::cout << "\nPushing values to fill stack:" << std::endl;
    client.push(99.9f);
    std::cout << "Pushed 99.9 (will be at bottom)" << std::endl;
    
    client.push(1.0f);
    std::cout << "Pushed 1.0" << std::endl;
    
    client.push(2.0f);
    std::cout << "Pushed 2.0" << std::endl;
    
    client.push(3.0f);
    std::cout << "Pushed 3.0 (top of stack)" << std::endl;
    
    std::cout << "\nPopping 3 times to reach bottom value:" << std::endl;
    for (int i = 0; i < 3; i++) {
        client.pop();
        RPNValueResult result = client.read();
        std::cout << "After pop " << (i+1) << ", top = " << result.value << std::endl;
    }
    
    RPNValueResult result = client.read();
    std::cout << "\nBottom value (99.9) is now at top: " << result.value << std::endl;
    
    if (result.status && (result.value > 99.89f && result.value < 99.91f)) {
        std::cout << "✓ Successfully retrieved bottom stack value" << std::endl;
    } else {
        std::cout << "✗ Failed to retrieve correct bottom value" << std::endl;
    }
    
    std::cout << "\nPopping 4 more times to show bottom value replicates:" << std::endl;
    for (int i = 0; i < 4; i++) {
        client.pop();
        result = client.read();
        std::cout << "After pop " << (i+1) << ", top = " << result.value << std::endl;
    }
    
    if (result.status && (result.value > 99.89f && result.value < 99.91f)) {
        std::cout << "✓ Bottom value replicates correctly when stack is popped" << std::endl;
    } else {
        std::cout << "✗ Bottom value did not replicate" << std::endl;
    }
    
    std::cout << "\n--- Testing that operations lift stack like pop ---" << std::endl;
    
    client.push(5.0f);
    client.push(10.0f);
    client.push(2.0f);
    client.push(3.0f);
    
    std::cout << "Stack loaded: [3.0, 2.0, 10.0, 5.0]" << std::endl;
    
    RPNValueResult add_result = client.add();
    std::cout << "Addition (3.0 + 2.0) = " << add_result.value << std::endl;
    
    result = client.read();
    std::cout << "Top after addition: " << result.value << std::endl;
    
    std::cout << "\nStack Depth Test Complete" << std::endl;
    
    return 0;
}