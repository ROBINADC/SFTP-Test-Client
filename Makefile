CC := gcc
CFLAGS := -std=c++11 -g -O2
LDLIBS := -lstdc++ -lssh2 -lpthread -lyaml-cpp

.PHONY: all clean prepare

all: test

test: test.cpp ssh.cpp ssh.h
	$(CC) -o $@ test.cpp ssh.cpp $(CFLAGS) $(LDLIBS)

prepare:
	mkdir -p /tmp/sftp/local
	mkdir -p /tmp/sftp/remote
	for i in {1..10}; do dd if=/dev/zero of=/tmp/sftp/remote/$${i}.txt bs=1M count=1; done

clean:
	rm -f test
