
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


    std::cout << "*** removing  server " << se << " as " << serviceName  << std::endl;
    bool abc = svcServer.removeService(serviceName,se);

    std::cout << "removeSeervice returned " << abc << std::endl;
}
