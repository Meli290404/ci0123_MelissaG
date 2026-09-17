CC  := gcc
CXX := g++
CFLAGS := -g -c
CXXFLAGS := -g -c -std=c++11
LDFLAGS :=
SSLLIBS := -lssl -lcrypto
DEPS := VSocket.h Socket.h SSLSocket.h
TARGETS := SSLForkServer.out SSLThreadServer.out SSLClient.out

$(info $$TARGETS = $(TARGETS))

.PHONY: all clean cert
all: $(TARGETS)

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $<

SSLForkServer.out: SSLForkServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLForkServer.o VSocket.o Socket.o SSLSocket.o $(SSLLIBS) $(LDFLAGS) -o $@

SSLThreadServer.out: SSLThreadServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLThreadServer.o VSocket.o Socket.o SSLSocket.o -lpthread $(SSLLIBS) $(LDFLAGS) -o $@

SSLClient.out: SSLClient.o VSocket.o Socket.o SSLSocket.o
	$(CXX) SSLClient.o VSocket.o Socket.o SSLSocket.o $(SSLLIBS) $(LDFLAGS) -o $@

# genera el certificado autofirmado y su llave, pedidos por el enunciado
cert:
	openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
	  -keyout key0123.pem -out ci0123.pem \
	  -subj "/C=CR/ST=SanJose/L=SanJose/O=UCR/OU=ECCI/CN=localhost"

clean:
	rm -f *.o *.out