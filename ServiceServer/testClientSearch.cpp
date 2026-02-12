
#include <iostream>
#include <string>

#include "svcDirClient.hpp"

using namespace svcDir;

int main(int argc, char * argv[]){
    if (argc != 2){
        std::cerr << "Usage: " << argv[0] << " serviceName" << std::endl;
        return 1;
    }

    serviceServer svcServer;

    std::string serviceName = argv[1];

    std::cout << "***searching " << serviceName  << std::endl;
    serverEntity se = svcServer.searchService(serviceName);
    std::cout << ">>search returned " << se << std::endl;
}
