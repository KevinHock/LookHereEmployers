include /users/cse533/Stevens/unpv13e/Make.defines

FLAGS = -g -O2
CFLAGS = -I../lib -g -O2 -D_REENTRANT
LIBS = /users/cse533/Stevens/unpv13e/libunp.a

all: clean api.o arp tour

tour: tour.o
	${CC} ${FLAGS} -o kaichen2_tour tour.o api.c wrapsock.c error.c get_hw_addrs.c ${LIBS}
tour.o: tour.c
	${CC} ${CFLAGS} -c tour.c api.c wrapsock.c error.c get_hw_addrs.c

arp: arp.o
	${CC} ${FLAGS} -o kaichen2_arp arp.o  wrapsock.c error.c get_hw_addrs.c ${LIBS}
arp.o: arp.c
	${CC} ${CFLAGS} -c arp.c  wrapsock.c error.c get_hw_addrs.c 

##odr_process: odr_process.o
##	${CC} ${FLAGS} -o odr_process odr_process.o wrapsock.c error.c get_hw_addrs.c ${LIBS}
##odr_process.o: odr_process.c
##	${CC} ${CFLAGS} -c odr_process.c wrapsock.c error.c
#api: api.o
	#${CC} ${FLAGS} -o api api.o wrapsock.c error.c ${LIBS}
api.o: api.c
	${CC} ${CFLAGS} -c api.c 

clean:
	rm -f arp  api tour *.o