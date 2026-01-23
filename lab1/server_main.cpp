#include "rpn_server.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    uint16_t port = 3600;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    std::cout << "Starting server on port " << port << std::endl;
    run_server(port);
    
    return 0;
}