# Linux Makefile
# Socket class with SSL
#

CC  := gcc
CXX := g++
CFLAGS := -g -c
CXXFLAGS := -g -c
LDFLAGS := -lssl -lcrypto
DEPS := VSocket.h Socket.h SSLSocket.h
TARGETS := ipv4-ssl-cli.out tcp-cli.out

ALL := $(TARGETS)
$(info $$TARGETS = $(TARGETS))

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $<

ipv4-ssl-cli.out: ipv4-ssl-cli.o VSocket.o Socket.o SSLSocket.o
	$(CXX) ipv4-ssl-cli.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

tcp-cli.out: tcp-cli.o VSocket.o Socket.o SSLSocket.o
	$(CXX) tcp-cli.o VSocket.o Socket.o SSLSocket.o $(LDFLAGS) -o $@

clean:
	@rm -f *.o *.out