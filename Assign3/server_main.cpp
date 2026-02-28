#include "rpn_server.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <port> <service> primary [replica_port ...]" << std::endl;
        std::cout << "       " << argv[0] << " <port> <service> replica <primary_host> <primary_port>" << std::endl;
        return 1;
    }
    
    uint16_t port = atoi(argv[1]);
    std::string service_name = argv[2];
    std::string mode = argv[3];

    bool isPrimary = false;
    std::string primaryHost;
    uint16_t primaryPort = 0;
    std::vector<uint16_t> replicaPorts;

    if (mode == "primary") {
        isPrimary = true;
        for (int i = 4; i < argc; i++) {
            replicaPorts.push_back(static_cast<uint16_t>(atoi(argv[i])));
        }
    } else if (mode == "replica") {
        if (argc < 6) {
            std::cerr << "Replica mode requires: <primary_host> <primary_port>" << std::endl;
            return 1;
        }
        primaryHost = argv[4];
        primaryPort = static_cast<uint16_t>(atoi(argv[5]));
    } else {
        std::cerr << "Mode must be 'primary' or 'replica'" << std::endl;
        return 1;
    }

    std::cout << "Starting server on port " << port << " with service name '" 
              << service_name << "'" << std::endl;
    run_server(port, service_name, isPrimary, primaryHost, primaryPort, replicaPorts);
    
    return 0;
}