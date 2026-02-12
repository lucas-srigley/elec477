//+
// File:   svcDirClient.cpp
//
// This is the client interface for the ELEC 477 service directory 
// server. 
//
// Approach:
//   Since this is typically only called once (client) or twice (server), we do the socket setup
//   and teardown for each request. If for some reason there are multiple threads,
//   then the sockets will use different epherimal port nubers, but since the
//   server will be replying to the sending port, there should not be and issue
//
//-

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
#include <atomic>
#include <cstring>
#include <regex>

#include "svcDirClient.hpp"
#include "HexDump.hpp"

#ifdef __APPLE__
#define MSG_CONFIRM 0
#endif

using namespace std;

//+
// Packet header (12 bytes)
//    magic: 4 bytes - value SRVC
//    version: 2 bytes - high odeer byte major, low order byte minor
//    opcode: 2 bytes - opcode 1 = register, 2 = remove, 3 = search, 4 = reset
//    serial: 4 bytes - serial number of request.
//
//  serverEntity(max 64 bytes)
//     serverNameLength : 1 byte
//     serviceName Max 63 bytes, null padded so that length name + length byte divisible by 2
//     portNumber 2 bytes
//
// register/remove packt format (max 144 bytes)
//     header (12 bytes)
//     serviceNameLength = 1 bytes
//     serviceName Max 63 bytes, null padded so that length name + length byte divisible by 4
//     serverEntity (max 64 bytes)
//     
// search format (max 46 bytes)
//     header (12 bytes)
//     serviceNameLength = 1 bytes
//     serviceName Max 63 bytes
//     does not need to be null padded because nothing following it.
//_

namespace svcDir {

// This is all inline so that the only header file for
// the students is the client interface file.

// packet layout
// defines so can be used for array lengths.
#define VERSIONOFFSET   4
#define OPOFFSET        6
#define SERIALOFFSET    8
#define DETAILOFFSET   12
#define MAXSERVERNAME  63
#define MAXSERVICENAME 63
// maximum send message length
#define SENDBUFFLEN  (12 + MAXSERVERNAME + 1 + MAXSERVERNAME + 1 + 2)





// implementation class
class serviceServer::serviceServerImpl{
public:
    serviceServerImpl(){};
    ~serviceServerImpl(){};
    bool registerService(string serviceName, serverEntity &server);
    bool removeService(string serviceName, serverEntity &server);
    serverEntity searchService(string serviceName);
    bool resetServiceServer();

private:
    const uint32_t magic = 'SRVC';
    const uint16_t version = 0x1000;
    enum opCode { regService = 1, remService, srchService, resetServer };
    string opString[5] = {
        "none", "register", "remove", "search", "reset"
    };

    // serial number for packets.
    // To make thead safe, this declaration has to be atomic
    // this makes the ++ operator atomic
    std::atomic<uint32_t> serial = 0;
    
    // server information.
    // this is only set once, and must be set before any other functions are called
    string serverAddressName;
    struct sockaddr_in servaddr;
    uint16_t port;
    int sockfd;
    bool addressInitialized = false;
    
    bool setupNetwork();
    void buildHeader(uint8_t *buff,opCode op, uint32_t ser);
    
    // header data structure to read return packets into.
    struct header {
       uint32_t magic;
       uint16_t version;
       uint16_t opCode;
       uint32_t serial;
    };

    bool parseHeader(uint8_t * buff, int32_t l, header & hdr );

};

//+
//**********************************************
// implementation of serviceServer methods.
// just forward to serviceServerImpl routines
//**********************************************
//-

serviceServer::serviceServer(){
    pImpl = make_unique<serviceServer::serviceServerImpl>();
}
serviceServer::~serviceServer(){
}

bool serviceServer::registerService(std::string serviceName, serverEntity &server){
    return pImpl->registerService(serviceName,server);
}

bool serviceServer::removeService(std::string serviceName, serverEntity &server){
    return pImpl->removeService(serviceName,server);
}

serverEntity serviceServer::searchService(std::string serviceName){
    return pImpl->searchService(serviceName);
}

bool serviceServer::resetServiceServer(){
    return pImpl->resetServiceServer();
}


//+
//**********************************************
// implementation of serviceServerImpl methods.
// just forward to serviceServerImpl routines
//**********************************************
//-


//+
// buildHeader
//
// write the four key values into the 12 byte packet header.
//-

void serviceServer::serviceServerImpl::buildHeader(uint8_t *buff,opCode op, uint32_t ser){
    *(uint32_t*)buff = htonl(magic);
    *((uint16_t*)(buff+VERSIONOFFSET)) = htons(version);
    *((uint16_t*)(buff+OPOFFSET)) = htons(op);
    *((uint32_t*)(buff+SERIALOFFSET)) = htonl(ser);
}

//+
// parseHeader
//
// extract four fields form first 12 bytes of packet
//
//-

bool serviceServer::serviceServerImpl::parseHeader(uint8_t * buff, int32_t l, header & hdr ){
    
    if (l < 12) return false;
    hdr.magic = ntohl(*((uint32_t*)buff));
    if (hdr.magic != magic){
       cerr << "Magic on packet not correct" << endl;
       return false;
    }
    hdr.version = ntohs(*(uint16_t*)(buff + VERSIONOFFSET));
    if (hdr.version != version){
       cerr << "Version on packet not correct" << endl;
       return false;
    }
    hdr.opCode = ntohs(*(uint16_t*)(buff + OPOFFSET));
    if (hdr.opCode > 4 || hdr.opCode == 0) {
       cerr << "opCode on packet not correct" << endl;
       return false;
    }
    hdr.serial = ntohl(*(uint32_t*)(buff + SERIALOFFSET));


    return true;
}


//+
// registerService client stub
//
// build a register packet, send it to the server and
// wait for the reply.
//
// TODO - update to at least once semantics
//-

bool serviceServer::serviceServerImpl::registerService(string serviceName, serverEntity &server){
    uint32_t sendBuffAligned[SENDBUFFLEN/4+1];
    uint8_t * sendBuff = (uint8_t*)sendBuffAligned;
    uint32_t serialForThisRequest = serial++;
    bool res = false;
    
    // cjeck parameters for errors.
    uint32_t svcNameLen = serviceName.length();
    if (svcNameLen > MAXSERVICENAME){
        errno = E_SERVICENAME;
        return false;
    }
    uint32_t servNameLen = server.name.length();
    if (servNameLen > MAXSERVERNAME){
        errno = E_SERVERNAME;
        return false;
    }
    if (server.port == 0){
        errno = E_PORTNUM;
        return false;
    }
    // verify aligned to 4 byte boundary (bottom two bits are zero
    //   0b11111100
    if ((long long)sendBuff & 3){
        cerr << "buffer is not word aligned" << endl;
        exit(1);
    }

    buildHeader(sendBuff, regService, serialForThisRequest);

    // add the rest of the message
    uint8_t * curPos = ((uint8_t*)(sendBuff + DETAILOFFSET));

    // add the service name
    *curPos++ = svcNameLen;
    memcpy(curPos,serviceName.data(), svcNameLen);
    curPos += svcNameLen;

    // align to 4 byte boundary
    curPos = (uint8_t*)(((uint64_t)(curPos + 3)) & 0xFFFFFFFFFFFFFFFCllu);

    // add the server details
    // server Name
    *curPos++ = servNameLen;
    memcpy(curPos,server.name.data(), servNameLen);
    curPos += servNameLen;

    // align to 2 byte boundary
    curPos = (uint8_t*)((uint64_t)(curPos + 1) & 0xFFFFFFFFFFFFFFFEllu);

    // port
    *((uint16_t*)curPos) = htons(server.port);
    curPos += 2;

    // total msg length
    uint32_t msgLen = curPos - sendBuff;

#ifdef DEBUG
    cout << " Send buffer ready to go = "<< endl << HexDump{sendBuff,msgLen};
#endif
    

    if (!setupNetwork()){
        errno = E_NOSERVER;
        cerr << "failed to setup Network" << endl;
        return false;
    }

    int numTries = 0;
    bool completed = false;
    
    // TODO need start loop here for at least once semantics
    
    // send the packet
#ifdef DEBUG
    cout << "  -------> sending " << endl;
#endif
    
    int n = sendto(sockfd, (const char *) sendBuff, msgLen,
	MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef DEBUG
    cout << "  sendto returned " << dec << n << endl;
#endif

    bool gotMessage = true;
    do {
        uint32_t recvBuffAligned[1024];
        uint8_t* recvBuffer = (uint8_t*)recvBuffAligned;
        static struct sockaddr_in servaddrreply;
        uint32_t len = sizeof(struct sockaddr_in);
        memset(&servaddrreply, 0, sizeof(servaddrreply));

        // wait for result from server
        n = recvfrom(sockfd, (char *) recvBuffer, 1024, MSG_WAITALL,
            (struct sockaddr*)&servaddrreply, & len);

#ifdef DEBUG
        cout << "  <------ got " << n << " bytes back from server" << endl;
#endif
        
        if (n == -1){
            // if timeout
        } else {
            // parse result message
            header hdr;
            if (parseHeader(recvBuffer, n, hdr)){
            // check magic, serial and op
            if (hdr.magic  == magic && hdr.serial == serialForThisRequest){
                if (n > DETAILOFFSET){
                    gotMessage = true;
                    res = recvBuffer[DETAILOFFSET];
#ifdef DEBUG
                    cout << "  register Service returned " << res << endl;
#endif
                }
            }
	    }
	    // go around again
	}

    } while(!gotMessage);

    close(sockfd);

    errno = 0;
    return res;
}


//+
// removeService client stub
//
// build a remove packet, send it to the server and
// wait for the reply.
//
// TODO - update to at least once semantics
//-

bool serviceServer::serviceServerImpl::removeService(string serviceName, serverEntity &server){
    uint32_t sendBuffAligned[SENDBUFFLEN/4+1];
    uint8_t * sendBuff = (uint8_t*)sendBuffAligned;
    uint32_t serialForThisRequest = serial++;
    bool res = false;

    uint32_t svcNameLen = serviceName.length();
    if (svcNameLen > MAXSERVICENAME){
        errno = E_SERVICENAME;
        return false;
    }
    uint32_t servNameLen = server.name.length();
    if (servNameLen > MAXSERVERNAME){
        errno = E_SERVERNAME;
        return false;
    }
    if (server.port == 0){
        errno = E_PORTNUM;
        return false;
    }

    buildHeader(sendBuff, remService, serialForThisRequest);

    // add the rest of the message
    uint8_t * curPos = ((uint8_t*)(sendBuff + DETAILOFFSET));

    // add the service name
    *curPos++ = svcNameLen;
    memcpy(curPos,serviceName.data(), svcNameLen);
    curPos += svcNameLen;

    // align to 4 byte boundary
    curPos = (uint8_t*)(((uint64_t)(curPos + 3)) & 0xFFFFFFFFFFFFFFFCllu);

    // add the details of the server to be removed
    // server Name
    *curPos++ = servNameLen;
    memcpy(curPos,server.name.data(), servNameLen);
    curPos += servNameLen;

    // align to 2 byte boundary
    curPos = (uint8_t*)((uint64_t)(curPos + 1) & 0xFFFFFFFFFFFFFFFEllu);

    // port
    *((uint16_t*)curPos) = htons(server.port);
    curPos += 2;

    // total msg length
    uint32_t msgLen = curPos - sendBuff;

#ifdef DEBUG
    //cout << "Send buffer ready to go = "<< endl << HexDump{sendBuff,msgLen};
#endif
    
    if (!setupNetwork()){
        errno = E_NOSERVER;
        cerr << "failed to setup Network" << endl;
        return false;
    }

    int numTries = 0;
    bool completed = false;
    
    // TODO need start loop here for at least once semantics

    // send the packet
#ifdef DEBUG
    cout << " ----> sending " << endl;
#endif
    
    int n = sendto(sockfd, (const char *) sendBuff, msgLen,
        MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef DEBUG
    cout << "  sendto returned " << dec << n << endl;
#endif
    
    bool gotMessage = true;
    do {
        uint32_t recvBuffAligned[1024];
        uint8_t* recvBuffer = (uint8_t*)recvBuffAligned;
        static struct sockaddr_in servaddrreply;
        uint32_t len = sizeof(struct sockaddr_in);
        memset(&servaddrreply, 0, sizeof(servaddrreply));

        // wait for result
        n = recvfrom(sockfd, (char *) recvBuffer, 1024, MSG_WAITALL,
            (struct sockaddr*)&servaddrreply, & len);

#ifdef DEBUG
        cout << " <------- got " << n << " bytes back from server" << endl;
#endif
        
        if (n == -1){
            // if timeout
        } else {
            // parse result message
            header hdr;
            if (parseHeader(recvBuffer, n, hdr)){
                // check magic, serial and op
                if (hdr.magic  == magic && hdr.serial == serialForThisRequest){
                    if (n > DETAILOFFSET){
                        gotMessage = true;
                        res = recvBuffer[DETAILOFFSET];
#ifdef DEBUG
                        cout << " remove Service returned " << res << endl;
#endif
                    }
                }
            }
            // go around again
        }
    } while(!gotMessage);

    close(sockfd);

    errno = 0;
    return res;

}

//+
// searchService client stub
//
// build a search packet, send it to the server and
// wait for the reply. Sender is smaller than the previous two
// since server details not sent. Receive logic is larger
// since server details are received.
//
// TODO - update to at least once semantics
//-

serverEntity serviceServer::serviceServerImpl::searchService(string serviceName){
    serverEntity res = serverEntity{"init",0};
    uint8_t sendBuff[SENDBUFFLEN];
    uint32_t serialForThisRequest = serial++;

    uint32_t svcNameLen = serviceName.length();
    if (svcNameLen > MAXSERVICENAME){
        errno = E_SERVICENAME;
        return res;
    }

    buildHeader(sendBuff, srchService, serialForThisRequest);

    // add the rest of the message
    uint8_t * curPos = ((uint8_t*)(sendBuff + DETAILOFFSET));

    // add the service name
    *curPos++ = svcNameLen;
    memcpy(curPos,serviceName.data(), svcNameLen);
    curPos += svcNameLen;
    // end of packet.
    
    // total msg length
    uint32_t msgLen = curPos - sendBuff;

#ifdef DEBUG
    //cout << "Send buffer ready to go = "<< endl << HexDump{sendBuff,msgLen};
#endif
    
    if (!setupNetwork()){
        errno = E_NOSERVER;
        cerr << "failed to setup Network" << endl;
        return res;
    }

    int numTries = 0;
    bool completed = false;
    
    // TODO need start loop here for at least once semantics

    // send the packet
#ifdef DEBUG
    cout << " -------> sending " << endl;
#endif
    
    int n = sendto(sockfd, (const char *) sendBuff, msgLen,
        MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef DEBUG
    cout << "  sendto returned " << dec << n << endl;
#endif
    
    bool gotMessage = true;
    do {
        uint32_t recvBuffAligned[1024];
        uint8_t* recvBuffer = (uint8_t*)recvBuffAligned;
        static struct sockaddr_in servaddrreply;
        uint32_t len = sizeof(struct sockaddr_in);
        memset(&servaddrreply, 0, sizeof(servaddrreply));

        // wait for result
        n = recvfrom(sockfd, (char *) recvBuffer, 1024, MSG_WAITALL, (struct sockaddr*)&servaddrreply, & len);
#ifdef DEBUG
        cout << " <------ recieved " << dec <<  n << " bytes back from server" << endl;
#endif
        if (n == -1){
            // if timeout
        } else {
            // parse result message
#ifdef DEBUG
            //cout << " Recived:" << endl << HexDump{sendBuff,n};
#endif
            
            header hdr;
            if (parseHeader(recvBuffer, n, hdr)){
                // check magic, serial and op
                if (hdr.magic  == magic && hdr.serial == serialForThisRequest){
                    if (n > DETAILOFFSET){
                        gotMessage = true;
                        // parse the
                        //res = recvBuffer[DETAILOFFSET];
                        uint32_t curPos = DETAILOFFSET;
                        int nameLen = *((uint8_t*)(recvBuffer+curPos++));
                        cout << "  reply server name length is " << nameLen << endl;
                        
                        if (n < curPos + nameLen){
                           cerr << "reply packet doesn't contain server name data" << endl;
                           return res;
                        }
                        res.name = string((char*)(recvBuffer+curPos),nameLen);
                        curPos += nameLen;
                        
#ifdef DEBUG
                        cout << "  reply server name is " << res.name << endl;
#endif
                        
                        // align to 2 bytes
                        curPos = ((curPos + 1) & 0xFFFFFFFE);
                        
                        if ((curPos + 2) > n) {
                           cerr << "packet doesn't contain port" << endl;
                           return res;
                        }
                        res.port = ntohs(*((uint16_t*)(recvBuffer+curPos)));
                        return res;
                    } else {
                        // no results.. error
                    }
                }
            }
            // go around again
        }
    } while(!gotMessage);

    close(sockfd);
    
    errno = 0;
    return res;
}



//+
// resetServiceServer client stub
//
// send packet to reset the service dictionary for this network.
// this packet doesn't have a body, no supplemental data
//
// TODO - not implemented yet (mplemented on server side)
// TODO - update to at least once semantics
//-

bool serviceServer::serviceServerImpl::resetServiceServer(){
    uint8_t sendBuff[SENDBUFFLEN];
    uint32_t serialForThisRequest = serial++;

    buildHeader(sendBuff, resetServer, serialForThisRequest);

    // this message has only a header with the op in it.
    // no other dat.
    
    if (!setupNetwork()){
        errno = E_NOSERVER;
        cerr << "failed to setup Network" << endl;
        return false;
    }

    int numTries = 0;
    bool completed = false;
    
    // TODO need start loop here for at least once semantics
    
    // send the packet
#ifdef DEBUG
    cout << " -----> sending " << endl;
#endif
    
    int n = sendto(sockfd, (const char *) sendBuff, sizeof(header),
    MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef DEBUG
    cout << "  sendto returned " << dec << n << endl;
#endif

    bool gotMessage = true;
    bool res = false;
    do {
        uint32_t recvBuffAligned[1024];
        uint8_t* recvBuffer = (uint8_t*)recvBuffAligned;
        static struct sockaddr_in servaddrreply;
        uint32_t len = sizeof(struct sockaddr_in);
        memset(&servaddrreply, 0, sizeof(servaddrreply));

        // wait for result from server
        n = recvfrom(sockfd, (char *) recvBuffer, 1024, MSG_WAITALL,
            (struct sockaddr*)&servaddrreply, & len);

#ifdef DEBUG
        cout << " <----- recieved " << n << " bytes back from server" << endl;
#endif
        
        if (n == -1){
            // if timeout
        } else {
            // parse result message
            header hdr;
            if (parseHeader(recvBuffer, n, hdr)){
                // check magic, serial and op
                if (hdr.magic  == magic && hdr.serial == serialForThisRequest){
                    if (n > DETAILOFFSET){
                        gotMessage = true;
                        res = recvBuffer[DETAILOFFSET];
#ifdef DEBUG
                        cout << "  reset Service Server returned " << res << endl;
#endif
                    }
                }
            }
            // go around again
        }

    } while(!gotMessage);

    close(sockfd);

    errno = 0;
    return res;
}

//+
// setupNetwork()
// the evironment variable SERVICEADDR has the form addr::port
//  example: "localhost:3001"
// fetch the address and setup the network data structures
//-

bool serviceServer::serviceServerImpl::setupNetwork(){
    // since we are running on a single node, just need the port
    // number from the environment.
    char* addrStr = getenv("SERVICEADDR");
    if (addrStr == NULL){
        std::cerr << "Must set service server in environment" << std::endl;
        std::cerr << "Example:: export SERVICEADDR=localhost:3001" << std::endl;
        return false;
    }
    std::string addrString = string(addrStr);
    
#ifdef DEBUG
    std::cout << "SERVICEADDR environment variable is '" << addrStr << "'" << std::endl;
#endif
    
    std::smatch matches;
    std::regex addressPattern("([a-zA-Z0-9]+):([0-9]+)");
    
    if (!std::regex_search(addrString, matches, addressPattern)) {
        std::cerr << "Server Address must have the form 'serverName:portnum'" << std::endl;
        return false;
    }
    
#ifdef DEBUG
    std::cout << "Server Name: " << matches[1].str() << std::endl;
    std::cout << "Server Port: " << matches[2].str() << std::endl;
#endif

    // use localhost as the server address.
    serverAddressName = string(matches[1].str());
    port = std::stoi(matches[2].str());
    
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    struct addrinfo * addr_result;
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = 0;
    int errcode = getaddrinfo (serverAddressName.c_str(), nullptr, &hints, &addr_result);
    if (errcode != 0){
        cout << "error finding address of " <<   serverAddressName << gai_strerror(errcode) << endl;
        errno=E_NOSERVER;
        return false;
    } else {
	if (addr_result -> ai_next != nullptr){
	    cout << "More than one result:" << endl;
	    int count = 0;
	    addrinfo * rover = addr_result;
	    while (rover != nullptr){
		cout << "  " << count++ <<  ": " << inet_ntoa(((sockaddr_in*)rover->ai_addr)->sin_addr)
		    << endl;
		rover = rover -> ai_next;
	    }
	} 
	// use the first if there is more than one
	cout << " address of service dir server is: " << inet_ntoa(((sockaddr_in*)addr_result->ai_addr)->sin_addr) <<endl;
	servaddr.sin_addr = ((sockaddr_in*)addr_result->ai_addr)->sin_addr;
	freeaddrinfo(addr_result);
    }
    
    // create socket and set timeout.
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        perror("socket creation failed");
        errno = E_SOCKET;
        return false;
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
        perror("proboem setting timeout");
        errno = E_SOCKET;
        return false;
    }

    return true;
}

// for pretty printing a server entity object.
ostream &operator << (ostream&s,serverEntity se){
    s << "{"<< se.name << ", " << dec << se.port << "}";
    return s;
}

}

