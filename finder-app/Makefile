
ifeq (${CROSS_COMPILE},)
gcc = gcc
else
gcc = aarch64-none-linux-gnu-gcc
endif

build-writer:
	${gcc} writer.c -o writer

clean:
	rm writer || true
