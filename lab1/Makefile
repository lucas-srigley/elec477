CXX = clang++
CXXFLAGS = -std=c++11 -Wall -I. -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1 -I/usr/local/opt/protobuf@21/include
LDFLAGS = -lprotobuf -lpthread -L/usr/local/opt/protobuf@21/lib

PROTO_SRC = rpn.proto
PROTO_GEN = rpn.pb.cc rpn.pb.h

PROTO_OBJ = rpn.pb.o
SERVER_OBJ = rpn_server.o server_main.o
CLIENT_OBJ = rpn_client.o

SERVER_EXE = server
TEST_EXES = test1 test2 test3 test4

all: $(SERVER_EXE) $(TEST_EXES)

$(PROTO_GEN): $(PROTO_SRC)
	protoc --cpp_out=. $(PROTO_SRC)

rpn.pb.o: rpn.pb.cc rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn.pb.cc -o rpn.pb.o

rpn_server.o: rpn_server.cpp rpn_server.h rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn_server.cpp -o rpn_server.o

server_main.o: server_main.cpp rpn_server.h
	$(CXX) $(CXXFLAGS) -c server_main.cpp -o server_main.o

rpn_client.o: rpn_client.cpp rpn_client.h rpn.pb.h
	$(CXX) $(CXXFLAGS) -c rpn_client.cpp -o rpn_client.o

$(SERVER_EXE): $(SERVER_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJ) $(PROTO_OBJ) -o $(SERVER_EXE) $(LDFLAGS)

test1.o: test1.cpp rpn_client.h
	$(CXX) $(CXXFLAGS) -c test1.cpp -o test1.o

test1: test1.o $(CLIENT_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test1.o $(CLIENT_OBJ) $(PROTO_OBJ) -o test1 $(LDFLAGS)

test2.o: test2.cpp rpn_client.h
	$(CXX) $(CXXFLAGS) -c test2.cpp -o test2.o

test2: test2.o $(CLIENT_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test2.o $(CLIENT_OBJ) $(PROTO_OBJ) -o test2 $(LDFLAGS)

test3.o: test3.cpp rpn_client.h
	$(CXX) $(CXXFLAGS) -c test3.cpp -o test3.o

test3: test3.o $(CLIENT_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test3.o $(CLIENT_OBJ) $(PROTO_OBJ) -o test3 $(LDFLAGS)

test4.o: test4.cpp rpn_client.h
	$(CXX) $(CXXFLAGS) -c test4.cpp -o test4.o

test4: test4.o $(CLIENT_OBJ) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) test4.o $(CLIENT_OBJ) $(PROTO_OBJ) -o test4 $(LDFLAGS)

clean:
	rm -f *.o $(PROTO_GEN) $(SERVER_EXE) $(TEST_EXES)

.PHONY: all clean