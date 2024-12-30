include Make.defines

PROGS =	game server

all:	${PROGS}

game:	game.o
		${CC} ${CFLAGS} -o $@ game.o unpv13e/libunp.a -lpthread

server:	server.o
		${CC} ${CFLAGS} -o $@ server.o unpv13e/libunp.a -lpthread

clean:
		rm -f ${PROGS} ${CLEANFILES}
