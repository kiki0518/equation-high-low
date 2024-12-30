include Make.defines

PROGS =	game server

all:	${PROGS}

client:	client.o
		${CC} ${CFLAGS} -o $@ client.o unpv13e/libunp.a -lpthread

clean:
		rm -f ${PROGS} ${CLEANFILES}
