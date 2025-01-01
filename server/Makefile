include Make.defines

SRCS = main.c game_init.c game_logic.c
PROGS =	client

all:	${PROGS}

game:	${SRCS:.c=.o}
		${CC} ${CFLAGS} -o $@ ${SRCS:.c=.o} unpv13e/libunp.a -lpthread -lm

%.o: %.c
		${CC} ${CFLAGS} -c $<

server:	server.o
		${CC} ${CFLAGS} -o $@ server.o unpv13e/libunp.a -lpthread

clean:
		rm -f ${PROGS} ${CLEANFILES}