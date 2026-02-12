
#include <iostream>

#include "svcDirClient.hpp"

using namespace svcDir;

int main(int argc, char * argv[]){
    
    serviceServer svcServer;

    std::cout << "***registering server axcd as mabxox "  << std::endl;
    serverEntity se = { "axcd", 3 };
    bool abc = svcServer.registerService("mabxox",se);
    
    std::cout << "***registering server take2 as mabxox "  << std::endl;
    se = serverEntity{"take2",25};
    abc = svcServer.registerService("mabxox",se);
   
    std::cout << "***registering server take3 as mabxox "  << std::endl;
    se = serverEntity{"take3",25};
    abc = svcServer.registerService("mabxox",se);
    
    std::cout << "***searching mabxox "  << std::endl;
    se = svcServer.searchService("mabxox");
    std::cout << ">>search returned " << se << std::endl;
    
    std::cout << "***removing server take2 as mabxox "  << std::endl;
    se = serverEntity{"take2",25};
    abc = svcServer.removeService("mabxox",se);

    std::cout << "***searching mabxox "  << std::endl;
    se = svcServer.searchService("mabxox");
    std::cout << ">>search returned " << se << std::endl;

    std::cout << "***searching mabxox "  << std::endl;
    se = svcServer.searchService("mabxox");
    std::cout << ">>search returned " << se << std::endl;

    std::cout << "***searching mabxox "  << std::endl;
    se = svcServer.searchService("mabxox");
    std::cout << ">>search returned " << se << std::endl;
    
    svcServer.resetServiceServer();
    
    std::cout << "***searching mabxox "  << std::endl;
    se = svcServer.searchService("mabxox");
    std::cout << ">>search returned " << se << std::endl;

}
