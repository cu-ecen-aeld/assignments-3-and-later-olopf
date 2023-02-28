
ifeq (${CROSS_COMPILE},)
gcc = gcc
else
gcc = aarch64-none-linux-gnu-gcc
endif

build-writer:
	${gcc} finder-app/writer.c -o finder-app/writer

clean:
	rm finder-app/writer
