CFLAGS=-Wall -DLINUX -I../include

all: demo

demo:	demo.c

clean:
	rm -f demo
	
install: demo switch
	install -m 0755 demo /usr/local/bin
