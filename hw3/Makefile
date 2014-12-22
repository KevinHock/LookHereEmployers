include /users/cse533/Stevens/unpv13e/Make.defines

FLAGS = -g -O2
CFLAGS = -I../lib -g -O2 -D_REENTRANT
LIBS = /users/cse533/Stevens/unpv13e/libunp.a

all: clean client server odr_process api.o

server: server.o
	${CC} ${FLAGS} -o server server.o api.c wrapsock.c error.c
server.o: server.c
	${CC} ${CFLAGS} -c server.c api.c wrapsock.c error.c

client: client.o
	${CC} ${FLAGS} -o client client.o api.c wrapsock.c error.c
client.o: client.c
	${CC} ${CFLAGS} -c client.c api.c wrapsock.c error.c

odr_process: odr_process.o
	${CC} ${FLAGS} -o odr_process odr_process.o wrapsock.c error.c get_hw_addrs.c ${LIBS}
odr_process.o: odr_process.c
	${CC} ${CFLAGS} -c odr_process.c wrapsock.c error.c

api.o: api.c
	${CC} ${CFLAGS} -c api.c wrapsock.c error.c

clean:
	rm -f server client odr_process api *.o