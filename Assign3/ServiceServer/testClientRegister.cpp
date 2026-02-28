
#include <iostream>
#include <string>

#include "svcDirClient.hpp"

using namespace svcDir;

int main(int argc, char * argv[]){
    if (argc != 4){
        std::cerr << "Usage: " << argv[0] << " serviceName serverName port" << std::endl;
        return 1;
    }

    std::string serviceName = argv[1];
    serverEntity se = serverEntity{argv[2],static_cast<uint16_t>(atoi(argv[3]))};
    serviceServer svcServer;

    std::cout << "***registering server " << se << " as " << serviceName  << std::endl;
    bool abc = svcServer.registerService(serviceName,se);

    std::cout << "registerService returned " << abc << std::endl;
}
