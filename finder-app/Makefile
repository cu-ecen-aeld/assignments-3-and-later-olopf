all: writer

writer.o: writer.c
	${CC} ${CFLAGS} -c writer.c

writer: writer.o
	${CC} ${CFLAGS} ${LDFLAGS} writer.o -o writer.c

clean:
	rm writer writer.o || true
