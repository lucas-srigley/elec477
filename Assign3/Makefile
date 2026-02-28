CXX = g++
CXXFLAGS = -std=c++17 -Wall -I. -IServiceServer
LDFLAGS = -lprotobuf -lpthread

PROTO_SRC = rpn.proto
PROTO_GEN = rpn.pb.cc rpn.pb.h

PROTO_OBJ = rpn.pb.o
SERVICE_OBJ = svcDirClient.o
SERVER_OBJ = rpn_server.o server_main.o
CLIENT_OBJ = rpn_client.o

SERVER_EXE = server
TEST_EXES = test1 test2 test3

all: $(SERVER_EXE) $(TEST_EXES)

$(PROTO_GEN): $(PROTO_SRC)
	protoc --cpp_out=. $(PROTO_SRC)

rpn.pb.o: rpn.pb.cc rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn.pb.cc -o rpn.pb.o

svcDirClient.o: ServiceServer/svcDirClient.cpp ServiceServer/svcDirClient.hpp
	$(CXX) $(CXXFLAGS) -c ServiceServer/svcDirClient.cpp -o svcDirClient.o

rpn_server.o: rpn_server.cpp rpn_server.hpp ServiceServer/svcDirClient.hpp rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn_server.cpp -o rpn_server.o

server_main.o: server_main.cpp rpn_server.hpp
	$(CXX) $(CXXFLAGS) -c server_main.cpp -o server_main.o

rpn_client.o: rpn_client.cpp rpn_client.hpp ServiceServer/svcDirClient.hpp rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn_client.cpp -o rpn_client.o

$(SERVER_EXE): $(SERVER_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ) -o $(SERVER_EXE) $(LDFLAGS)

test1.o: test1.cpp rpn_client.hpp
	$(CXX) $(CXXFLAGS) -c test1.cpp -o test1.o

test1: test1.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test1.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ) -o test1 $(LDFLAGS)

test2.o: test2.cpp rpn_client.hpp
	$(CXX) $(CXXFLAGS) -c test2.cpp -o test2.o

test2: test2.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test2.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ) -o test2 $(LDFLAGS)

test3.o: test3.cpp rpn_client.hpp
	$(CXX) $(CXXFLAGS) -c test3.cpp -o test3.o

test3: test3.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test3.o $(CLIENT_OBJ) $(SERVICE_OBJ) $(PROTO_OBJ) -o test3 $(LDFLAGS)

clean:
	rm -f *.o $(PROTO_GEN) $(SERVER_EXE) $(TEST_EXES)