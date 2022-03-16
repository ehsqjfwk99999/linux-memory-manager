all: mm.o testapp.o
	gcc -g mm.o testapp.o
mm.o: mm.c
	gcc -c mm.c -o mm.o
testapp.o: testapp.c
	gcc -c testapp.c -o testapp.o
clean:
	[ -f a.out ] && rm a.out || true
	[ -f *.o ] && rm *.o || true