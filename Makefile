CC := gcc
SRCS := $(wildcard *.c)
OBJS := $(SRCS:c=o)

all: $(OBJS)
	$(CC) -g $^
mm.o: mm.c
	$(CC) -c $^ -o $@
testapp.o: testapp.c
	$(CC) -c $^ -o $@
glthread.o: glthread.c
	$(CC) -c $^ -o $@
clean:
	[ -f a.out ] && rm a.out || true
	for obj in $(OBJS); do [ -f $$obj ] && rm $$obj || true; done
