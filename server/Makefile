include Make.defines

PROGS =	client

all:	${PROGS}

game_logic:	game_logic.o
		${CC} ${CFLAGS} -o $@ game_logic.o unpv13e/libunp.a -lpthread -lm

server:	server.o
		${CC} ${CFLAGS} -o $@ server.o unpv13e/libunp.a -lpthread

clean:
		rm -f ${PROGS} ${CLEANFILES}
