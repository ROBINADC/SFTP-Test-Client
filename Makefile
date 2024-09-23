CC := gcc
CFLAGS := -std=c++11 -g -O2
LDLIBS := -lstdc++ -lssh2 -lpthread
LDFLAGS :=

.PHONY: all clean prepare

all: test

test: test.cpp sftp.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LDLIBS)

prepare:
	mkdir -p /tmp/sftp/local
	mkdir -p /tmp/sftp/remote
	for i in {1..10}; do dd if=/dev/zero of=/tmp/sftp/remote/$${i}.txt bs=1M count=1; done

clean:
	rm -f test