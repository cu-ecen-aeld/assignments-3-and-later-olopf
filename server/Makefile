
SOURCE_FILES = $(wildcard *.c *.h)
OBJ_FILES = $(subst .c,.o,$(wildcard *.c))

all: aesdsocket

aesdsocket: aesdsocket.o
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJ_FILES} -o aesdsocket

aesdsocket.o:
	@echo "Compiler: ${CC}"
	${CC} ${CFLAGS} -c ${SOURCE_FILES}

clean:
	rm aesdsocket ${OBJ_FILES} || true

list:
	@echo "Source Files: ${SOURCE_FILES}"
	@echo "Object Files: ${OBJ_FILES}"
