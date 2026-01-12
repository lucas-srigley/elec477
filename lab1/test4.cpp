#include "rpn_client.h"
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    uint16_t port1 = 3600;
    uint16_t port2 = 3601;
    
    std::cout << "Test 4: Multiple Independent Servers" << std::endl;
    std::cout << "This test requires two servers running on ports " 
              << port1 << " and " << port2 << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    RPNClient client1(port1);
    RPNClient client2(port2);
    
    std::cout << "\n--- Server 1 (port " << port1 << ") ---" << std::endl;
    client1.push(100.0f);
    std::cout << "Pushed 100.0 to server 1" << std::endl;
    
    RPNValueResult result1 = client1.read();
    std::cout << "Server 1 top of stack: " << result1.value << std::endl;
    
    std::cout << "\n--- Server 2 (port " << port2 << ") ---" << std::endl;
    client2.push(200.0f);
    std::cout << "Pushed 200.0 to server 2" << std::endl;
    
    RPNValueResult result2 = client2.read();
    std::cout << "Server 2 top of stack: " << result2.value << std::endl;
    
    std::cout << "\n--- Verifying Independence ---" << std::endl;
    result1 = client1.read();
    result2 = client2.read();
    
    std::cout << "Server 1 still has: " << result1.value << std::endl;
    std::cout << "Server 2 still has: " << result2.value << std::endl;
    
    bool independent = (result1.value > 99.9f && result1.value < 100.1f) &&
                      (result2.value > 199.9f && result2.value < 200.1f);
    
    if (independent) {
        std::cout << "\n✓ TEST PASSED: Servers are independent" << std::endl;
    } else {
        std::cout << "\n✗ TEST FAILED: Servers are not independent" << std::endl;
    }
    
    std::cout << "\n--- Different Operations on Each Server ---" << std::endl;
    
    client1.push(50.0f);
    RPNValueResult add1 = client1.add();
    std::cout << "Server 1: 100.0 + 50.0 = " << add1.value << std::endl;
    
    client2.push(100.0f);
    RPNValueResult sub2 = client2.subtract();
    std::cout << "Server 2: 200.0 - 100.0 = " << sub2.value << std::endl;
    
    if (add1.status && sub2.status &&
        (add1.value > 149.9f && add1.value < 150.1f) &&
        (sub2.value > 99.9f && sub2.value < 100.1f)) {
        std::cout << "\n✓ Both servers computed correct independent results" << std::endl;
    }
    
    std::cout << "\nMultiple Servers Test Complete" << std::endl;
    
    return 0;
}