CC=gcc
CFLAGS=-Wall -g
BINS= libmythreads

all: $(BINS)
libmythreads:  libmythreads.c
	$(CC) $(CFLAGS) -c libmythreads.c
	ar -cvrs libmythreads.a libmythreads.o

createtest:
	$(CC) $(CFLAGS) -o test cooperative_test.c libmythreads.a
	$(CC) $(CFLAGS) -o test2 preemtive_test.c libmythreads.a
	$(CC) $(CFLAGS) -o locktest pthreadex.c libmythreads.a
	$(CC) $(CFLAGS) -o final condtest.c libmythreads.a

clean:
		rm -f libmythreads.o
		rm -f libmythreads.a
project:
		rm -f project2.tgz
		tar cvzf project2.tgz makefile libmythreads.c mythreads.h README
test_coop:
		./test > out.txt
		meld out.txt coop_expected.txt
