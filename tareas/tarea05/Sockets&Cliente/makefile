CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -lssl -lcrypto

SRCS = Client.cc Socket.cc VSocket.cc SSLSocket.cc
OBJS = $(SRCS:.cc=.o)
TARGET = Client

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TARGET)
	./pruebas.sh

clean:
	rm -f $(TARGET) $(OBJS) resultados.txt

.PHONY: all clean test