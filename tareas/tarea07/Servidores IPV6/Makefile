CC  := gcc
CXX := g++
CFLAGS := -g -c
CXXFLAGS := -g -c -std=c++11
LDFLAGS :=
DEPS := Socket.h
TARGETS := ForkMirrorServer6.out MirrorClient6.out ThreadMirrorServer6.out

$(info $$TARGETS = $(TARGETS))

.PHONY: all clean
all: $(TARGETS)

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $<

%.out: %.o VSocket.o Socket.o
	$(info $$^ = $^)
	$(CXX) $^ $(LDFLAGS) -o $@

ForkMirrorServer6.out:   ForkMirrorServer6.o VSocket.o Socket.o
	$(CXX) ForkMirrorServer6.o VSocket.o Socket.o $(LDFLAGS) -o $@
ThreadMirrorServer6.out: ThreadMirrorServer6.o VSocket.o Socket.o
	$(CXX) ThreadMirrorServer6.o VSocket.o Socket.o -lpthread $(LDFLAGS) -o $@

clean:
	rm -f *.o *.out