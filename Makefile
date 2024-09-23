CC := gcc
CFLAGS := -std=c++11 -g -O3
LDLIBS := -lstdc++ -lssh2 -lpthread
LDFLAGS :=

.PHONY: all clean

all: run

run: run.cpp
	$(CC) -o $@ $^ $(CFLAGS) $(LDLIBS)

clean:
	rm -f run