//+
// ServiceServer is a simple service discovery server.
// It allows servers to register for a given service,
// and for clients to add, remove and look up servers.
//
// Originally for multi project, it kept track of origin network, that
// has now been moved to separate servers for each grouop.
//
// There are four operations:
//
//    1. register service (add a server for a service)
//    2. remove service (delete a server from a service)
//    3. return a server for a service
//    4. reset the dictionary .
//
//-


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <poll.h>

#include <iostream>
#include <map>
#include <vector>
#include <random>

#include "HexDump.hpp"

// Port Range - set to your groups values
#define SERVICE_START_PORT 3600
#define SERVICE_END_PORT 3699

// bigger than anything that will happen.
#define BUFFSIZE 2048

#ifdef __APPLE__
// MacOS does not have the MSG_CONFIRM flag,
// set to 0 so no effect in flags.
#define MSG_CONFIRM 0
#endif

using namespace std;

// magic and version
// these are not defined in a .hpp file so that
// the client file can be self contained.

static const uint32_t magic = 'SRVC';
static const uint16_t version = 0x1000;

enum opCode { regService = 1, remService, srchService, resetServer };
string opString[] = {
    "none", "register", "remove", "search", "reset"
};

struct header {
   uint32_t magic;
   uint16_t version;
   uint16_t opCode;
   uint32_t serial;
};

// prototypes for service routines
bool registerService(uint8_t *buffer, int32_t & n);
bool removeService(uint8_t *buffer, int32_t & n);
bool searchService(uint8_t *buffer, int32_t & n);
bool resetServiceServer(uint8_t *buffer, int32_t & n);

// offsets into packets and max lengths
#define VERSIONOFFSET   4
#define OPOFFSET        6
#define SERIALOFFSET    8
#define DETAILOFFSET    12
#define MAXSERVERNAME   63
#define MAXSERVICENAME  63

// utility routine prototypes
bool parseHeader(uint8_t * buff, int32_t l, header & hdr );

// main continuation flag
// this will be set to false by the interrupt signal handler
// global because changed by the interupthandler routine
// TODO - investigate using poll/signalfd instead of sigaction
bool processing = false;

// Interrupt signal handler. Called when the user
// types Ctrl-C. Setts the processing to false
extern "C" void mainIntrHandler(int signal){
#ifdef DEBUG
    cerr << "***************** Interrupt **********************" << endl;
#endif
    processing = false;
}

//+
// main routine
//
// Opens the socket, receives messages and calls the appropriate
// service handling routine and sends back the packet.
//
//-

int main(int argc, char * argv[]) {
    int sockfd;
    uint32_t bufferInt[BUFFSIZE];
    uint8_t *buffer = (uint8_t*)bufferInt;
    struct sockaddr_in servaddr, cliaddr;
    
    if (argc != 2){
        std::cerr << "Usage: " << argv[0] << " portnumber" << std::endl;
        exit(1);
    }
    uint16_t port = atoi(argv[1]);
    if (port == 0){
        std::cerr << "Port number canot be zero" << std::endl;
        exit(1);
    }
    if (port < SERVICE_START_PORT || port > SERVICE_END_PORT){
        std::cerr << "Port number must be between "
            << SERVICE_START_PORT << " and "
            << SERVICE_END_PORT << std::endl;
        exit(1);
    }
    
    // seed the random number generator
    std::random_device rd; 
    std::mt19937 gen(rd());

    // double check buffer alignment for ARM machines
    if ((long long)buffer & 3){
        cerr << "buffer is not word aligned" << endl;
        exit(1);
    }


    // Create a socket to recieve messages
#ifdef DEBUG
    cout << "Server creating a socket" << endl;
#endif
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(errno);
    }
    
    // Initialize sockaddr_in structures to zero.
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
       
    // Bind the server, currently to
    // the port given on the command line and
    // all IPv4 interfaces on the machine.
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
       
    if (::bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // set global processing flag.
    processing = true;

    // direct cntrl C to our Interrupt Handler
    struct sigaction mySigHandler;
    mySigHandler.sa_handler = mainIntrHandler;
    mySigHandler.sa_flags = 0;
    sigaction(SIGINT,&mySigHandler, NULL);
    sigaction(SIGTERM,&mySigHandler, NULL);

    struct sigaction ignoreSignal;
    mySigHandler.sa_handler = SIG_IGN;
    mySigHandler.sa_flags = 0;
    sigaction(SIGHUP,&ignoreSignal, NULL);

    // processing is set to false by the signal handler
    while (processing){
        socklen_t len;
        int32_t n;
           
        // len field is a value/result parameter
        // It contains the number of bytes available in
        // the data structure to store the address of
        // the machine that sends the message (client).
        // after call, will contain the number of bytes
        // that was written into the structure.
        len = sizeof(cliaddr);
        
#ifdef DEBUG
        cerr << "******************************************************" << endl;
#endif
        cerr << "Service Server waiting for a message" << endl;

        // wait for packet from a client
        n = recvfrom(sockfd, (char *)buffer, BUFFSIZE, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        // n is the number of bytes received.
#ifdef DEBUG
        cerr << "Server received " << dec << n << " bytes from " << inet_ntoa(cliaddr.sin_addr)<< endl;
#endif
        if (processing && n > 0){

#ifdef DEBUG
            cout << "Server received -------"<< endl << HexDump{buffer, (uint32_t)n};
            cerr << "-----------------------" << endl;
#endif

            // the header contains the opcodes.
            // parseHeader checks the magic and signal.
            // We reuse the buffer, only modifying the bufer after
            // the header (header is 12 bytes), so all
            // the header fields will be returned to the client unchanged.
            
            header receivedHeader;
            if (parseHeader(buffer, n, receivedHeader)){

#ifdef DEBUG
                cerr << "  Opcode is " << opString[receivedHeader.opCode] << endl;
                cerr << "  Serial is " << receivedHeader.serial << endl;
#endif
                
                bool result;
                switch((opCode)receivedHeader.opCode){
                    case regService:
                        result = registerService(buffer,n);
                        break;
                    case remService:
                        result = removeService(buffer,n);
                        break;
                    case srchService:
                        result = searchService(buffer,n);
                        break;
                    case resetServer:
                        result = resetServiceServer(buffer,n);
                        break;
                }
                if (result){
                    // this should alwasy be true.
                    // service routines should handle errors and set an appropriate return
                    // value.
                    // TODO - not all errors are correctly handled must be updaqated
#ifdef DEBUG
                    cerr << "Buffer to return is:" << endl << HexDump{buffer,n};
#endif
                    
                    uint32_t sendn = sendto(sockfd,(char*)buffer, n,MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len);
                }
            } else {
               // invalid packet after header that was not handled by the service routines
               // currently the packet is simply ignored and the client will have to time out.
            }
        }
	     // if n < 0 then there was an error in reciving the packet, go arround again.
    }

    // we get here on an interrupt (Ctrl-C) or SIGTERM
#ifdef DEBUG
    cerr << "Server done" << endl;
#endif
    
    close(sockfd);
    return 0;
}

// ************************************

//+
// debug tracing routines
//=

void enterf(string routine){
#ifdef TRACE
    cerr << ">>>>>>> Entering " << routine << endl;
#endif
}

void exitf(string routine){
#ifdef TRACE
    cerr << "<<<<< Exiting " << routine << endl;
#endif
}
// ************************************

//+
// parseaHeader
//
// parse and extract the header information.
// returns false if the header cannot be parsed.
//-

bool parseHeader(uint8_t * buff, int32_t l, header & hdr ){
#ifdef TRACE
    enterf("parseHeader");
#endif
    
    if (l < 12) {
        exitf("parseHeader");
        return false;
    }
    
    hdr.magic = ntohl(*((uint32_t*)buff));
    if (hdr.magic != magic){
        cerr << "Magic in packet not correct" << endl;
        exitf("parseHeader");
        return false;
    }

    hdr.version = ntohs(*(uint16_t*)(buff + VERSIONOFFSET));
    if (hdr.version != version){
        cerr << "Packet version not correct" << endl;
        exitf("parseHeader");
        return false;
    }
    hdr.opCode = ntohs(*(uint16_t*)(buff + OPOFFSET));
    if (hdr.opCode > 4 || hdr.opCode == 0) {
        cerr << "opCode on packet not correct" << endl;
        exitf("parseHeader");
        return false;
    }
    hdr.serial = ntohl(*(uint32_t*)(buff + SERIALOFFSET));
    
#ifdef TRACE
    exitf("parseHeader");
#endif
    return true;
}


// Data structures describing in packets
// server name and port

struct serverEntity {
    string name;
    uint16_t port;
};

// stream formatter for debugging
ostream &operator << (ostream&s,serverEntity se){
    s << "{"<< se.name << ", " << dec << se.port << "}";
    return s;
}

// This is the main dictionary used by the server.

// Map service name to a vector of server/port entities
map<string,vector<serverEntity>> svcMap;

//+
// readName
//
// a name is stored as a single byte length followed the string data. No null
// byte is stored at the end. The curpos parameter is pass by reference and
// set to the position of the byte following the end of the name.
//
//-

bool readName(uint8_t * buffer, int32_t n, uint32_t &curPos, string & name, string errName){
#ifdef TRACE
    enterf("readName");
#endif
    
    // recover length of name
    if (curPos >= n){
       cerr << "packet doesn't contain " << errName << " name" << endl;
        exitf("readName");
        return false;
    }

    int nameLen = *((uint8_t*)(buffer+curPos++));
    
#ifdef DEBUG
    cout << "  " << errName << " name length is " << nameLen << endl;
#endif
    
    // recover name data
    if (n < curPos + nameLen){
        cerr << "packet doesn't contain " << errName << " name data" << endl;
        exitf("readName");
        return false;
    }
    name = string((char*)(buffer+curPos),nameLen);

#ifdef DEBUG
    cout << "  " << errName << " name is " << name << endl;
#endif
    
    // advance curPos
    curPos += nameLen;
    
#ifdef TRACE
    exitf("readName");
#endif
    return true;
}

//********************************************************************
// Service Routines
//********************************************************************

//+
// registerService
//
// extract the service name, server name and port from the packet data.
// add it to the dictionary given by the client address (masked to a network number)
//
// TODO - shoud create an appropriate response when the packet is malformed
//   - add a false byte at end of header and set length to header + 1
//-

bool registerService(uint8_t *buffer, int32_t & n){
    //note header is already read. So next byte availble in the record
    // is DETAILOFFSET (byte 13 at index 12).
    
    string svcName;
    uint32_t curPos = DETAILOFFSET;

#ifdef TRACE
    enterf("registerServce");
#endif

    // read the service name
    if (!readName(buffer,n,curPos,svcName,"service")){
        exitf("registerServce");
        return false;
    }

    // 4 byte alignment. Note this might take us past
    // the end of the buffer, must be checked.
    curPos = ((curPos + 3) & 0xFFFFFFFC);

    // check more data available.
    if (curPos >= n){
        cerr << "packet doesn't contain server name" << endl;
        exitf("registerServce");
        return false;
    }

    // get server data
    serverEntity server;

    // read the server name.
    if (!readName(buffer,n,curPos,server.name,"server")){
        exitf("registerServce");
        return false;
    }

#ifdef DEBUG
    cout << " server name is " << server.name << endl;
#endif
    // go to 2 byte alignment (again may take us past the end of the data)
    curPos = ((curPos + 1) & 0xFFFFFFFE);
        
    // check that room in the packet for the port
    if ((curPos + 2) > n) {
       cerr << "packet doesn't contain port" << endl;
       exitf("registerServce");
       return false;
    }
    // recover the port number
    server.port = ntohs(*((uint16_t*)(buffer+curPos)));
    curPos += 2;

#ifdef DEBUG
    cout << " port = " << dec << server.port << endl;
#endif
    
    cerr << "Adding " << svcName << " -> (" << server.name << "," << server.port << ")" << endl;
    // add the server to the service
    vector<serverEntity> servers = svcMap[svcName];
    bool found = false;
    for (auto it = servers.begin(); it != servers.end(); it++){
        if (it->name == server.name && it->port == server.port)
	   found = true;
    }
    if (!found){
#ifdef DEBUG
        cout << " adding to map" << endl;
#endif
        svcMap[svcName].push_back(server);
    }

    //return boolean true
    // store at first byte after the
    // header.
    buffer[DETAILOFFSET] = 1;
    n = DETAILOFFSET + 1;

#ifdef TRACE
    exitf("registerServce");
#endif
    return true;

}

//+
// removeService
//
// extract the service name, server name and port from the packet data.
// remove it to the dictionary given by the client address (masked to a network number)
// The bulk of the code is the same as registerService, only the final details.
//
// TODO - shoud create an appropriate response when the packet is malformed
//   - add a false byte at end of header and set length to header + 1
//-

bool removeService(uint8_t *buffer, int32_t & n){
    //note header is already read. So next byte availble in the record
    // is DETAILOFFSET (byte 13 at index 12).
    
    string svcName;
    uint32_t curPos = DETAILOFFSET;

#ifdef TRACE
    enterf("removeService");
#endif
    
    // read the service name
    if (!readName(buffer,n,curPos,svcName,"service")){
        exitf("removeService");
        return false;
    }

    // 4 byte alignment. Note this might take us past
    // the end of the buffer, must be checked.
    curPos = ((curPos + 3) & 0xFFFFFFFC);

    // check more data available.
    if (curPos >= n){
        cerr << "packet doesn't contain server name" << endl;
        exitf("removeService");
        return false;
    }

    // get server data
    serverEntity server;

    // read the server name.
    if (!readName(buffer,n,curPos,server.name,"server")) {
        exitf("removeService");
        return false;
    }
#ifdef DEBUG
    cout << "  server name is " << server.name << endl;
#endif

    // go to 2 byte alignment (again may take us past the end of the data)
    curPos = ((curPos + 1) & 0xFFFFFFFE);

    // check that room in the packet for the port
    if ((curPos + 2) > n) {
        cerr << "packet doesn't contain port" << endl;
        exitf("removeService");
        return false;
    }
    server.port = ntohs(*((uint16_t*)(buffer+curPos)));
    curPos += 2;

#ifdef DEBUG
    cout << "  port = " << dec << server.port << endl;
#endif
    
#ifdef DEBUG
    cout << "  Removing " << server << endl;
#endif
    
    cerr << "Removing " << svcName << " -> (" << server.name << "," << server.port << ")" << endl;
    // remove the server from the service If it is there.
    // iterate over the vectore of servers associated with the
    // service name. Note must be a reference variable or
    // we get a copy of the vecor and don't actually delete
    // from the vector in the map.
    vector<serverEntity> &servers = svcMap[svcName];
    bool found = false;
    auto it = servers.begin();
    while ( it != servers.end() && !found){

#ifdef DEBUG
        cout << "  checking " << *it << endl;
#endif
        
        if (it->name == server.name && it->port == server.port){
            found = true;
            break;
        }
        it++;
    }
    
    if (found) {
#ifdef DEBUG
        cout << "  removing " << *it << "from map" << endl;
#endif
	    svcMap[svcName].erase(it);
    }
    
    // alwasy return true. If it wasn't there it didn't need to be deleted,
    // so still not there.
    buffer[DETAILOFFSET] = 1;
    n = DETAILOFFSET + 1;

#ifdef TRACE
    exitf("removeService");
#endif
    return true;

}

//********************************************************************************
// Template to select a random element of a C++ Container
// from
// https://stackoverflow.com/questions/6942273/how-to-get-a-random-element-from-a-c-container
//********************************************************************************

#include  <iterator>

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}
//********************************************************************************

//+
// searchService
//
// extract the service name, server name and port from the packet data.
// add it to the dictionary given by the client address (masked to a network number)
//
// TODO - shoud create an appropriate response when the packet is malformed
//   - add a false byte at end of header and set length to header + 1
//-

bool searchService(uint8_t *buffer, int32_t & n){
    //note header is already read. So next byte availble in the record
    // is DETAILOFFSET (byte 13 at index 12).
    string svcName;
    uint32_t curPos = DETAILOFFSET;

#ifdef TRACE
    enterf("searchService");
#endif
    
    // read the service name
    if (!readName(buffer,n,curPos,svcName,"service")) {
        exitf("searchService");
        return false;
    }
    // there is only a service name in the packet,
    // so this is the end of the  input

    // pick random server from vector
    vector<serverEntity> &servers = svcMap[svcName];
    serverEntity se;
    if (servers.size() == 0){
        // no servers to pick from
        se = serverEntity{"None",0};
    } else{
        se = *select_randomly(servers.begin(), servers.end());
    }

    // return the server that was picked.
    // reset curPos back to the end of the
    curPos = DETAILOFFSET;

#ifdef DEBUG
    cout << "  Returning server " << se << endl;
#endif
    
    cerr << "Searching for " << svcName << " returned (" << se.name << "," << se.port << ")" << endl;
    
    // store server name lentth
    buffer[curPos++] = se.name.length();
    
    // store srver name data
    memcpy((buffer+curPos),se.name.data(), se.name.length());
    curPos += se.name.length();
    // align at 2 bytew
    curPos = ((uint64_t)(curPos + 1) & 0xFFFFFFFFFFFFFFFEllu);
    // store port number
    *((uint16_t*)(buffer+curPos)) = htons(se.port);
    curPos += 2;
    
    // set length of return packet.
    n = curPos;
    
    exitf("searchService");
    return true;

}

//+
// resetService
//
// Erase the inner map of all service names and servders
//
//-

bool resetServiceServer(uint8_t *buffer, int32_t & n){
    enterf("resetService");
    
    // start with the clint net address


    svcMap.clear();

    // return true
    buffer[DETAILOFFSET] = 1;
    n = DETAILOFFSET + 1;

    exitf("resetService");
    return true;
}

