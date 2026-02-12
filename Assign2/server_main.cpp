#include "rpn_server.hpp"
#include <iostream>
#include <cstdlib>
#include <string>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    std::string service_name = "";
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    if (argc > 2) {
        service_name = argv[2];
    }
    
    if (service_name.empty()) {
        std::cout << "Usage: " << argv[0] << " <port> <service_name>" << std::endl;
        std::cout << "Example: " << argv[0] << " 3600 calc_server_1" << std::endl;
        return 1;
    }
    
    std::cout << "Starting server on port " << port << " with service name '" 
              << service_name << "'" << std::endl;
    run_server(port, service_name);
    
    return 0;
}