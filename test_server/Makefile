CC=gcc
CFLAGS=-I. -g -D_LINUX -lm -lpthread  -pedantic
OBJ = httpparser.o

all: http_svr

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

http_svr: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f http_svr $(CLIENT_OBJ)
