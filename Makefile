CFLAGS = -std=c99 -O2

dlang: nfunc.o

all: dlang

clean:
	rm *.o dlang
