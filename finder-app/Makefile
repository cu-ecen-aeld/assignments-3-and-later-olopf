
all: writer

writer: writer.o
	${CC} ${CFLAGS} ${LDFLAGS} writer.o -o writer

writer.o: writer.c
	@echo "Compiler: ${CC}"
	${CC} ${CFLAGS} -c writer.c -o writer.o

clean:
	rm writer writer.o || true
