//+
// File:   svcDirClient.hpp
//
// This is the client interface for the ELEC 477 service directory 
//-

#include <string>
#include <vector>
#include <errno.h>
#include <cstdint>
#include <iostream>
#include <memory>

namespace svcDir {

#define E_SERVICENAME 1
#define E_SERVERNAME 2
#define E_PORTNUM 3
#define E_TIMEOUT 4
#define E_NOSERVER 5
#define E_SOCKET 6

//+
// a serverEntity is a serverName and a port on that server
//-

struct serverEntity {
    std::string name;
    uint16_t port;
};
std::ostream &operator << (std::ostream&s,serverEntity se);

class serviceServer{
public:
    serviceServer();
    ~serviceServer();
    bool registerService(std::string serviceName, serverEntity &server);
    bool removeService(std::string serviceName, serverEntity &server);
    serverEntity searchService(std::string serviceName);
    bool resetServiceServer();
private:
    class serviceServerImpl;
    std::unique_ptr<serviceServerImpl> pImpl;
};

}
