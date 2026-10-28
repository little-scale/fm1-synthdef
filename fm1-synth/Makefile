CC ?= cc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Werror -ffp-contract=off -fno-fast-math
ENGINE = ../synthdef-engine/src
SOURCES = $(ENGINE)/fm_load.c $(ENGINE)/fm_render.c src/performance.c src/slots.c src/protocol.c src/codeview.c tools/bridge.c
.PHONY: all test serve sanitize
all: build/libinstrument.dylib
build/libinstrument.dylib: $(SOURCES) $(wildcard src/*.h $(ENGINE)/*.h $(ENGINE)/*.inc)
	mkdir -p build
	$(CC) $(CFLAGS) -Isrc -I$(ENGINE) -fPIC -shared $(SOURCES) -lm -o $@
test: all
	$(CC) $(CFLAGS) -Isrc -I$(ENGINE) $(SOURCES) tests/core.c -lm -o build/core-test
	build/core-test
	python3 -m unittest discover -s tests -p 'test_*.py' -v
	node tests/screen.mjs
	node tests/transport.mjs
serve: all
	../.venv/bin/python tools/server.py
sanitize:
	$(CC) -std=c99 -O1 -g -ffp-contract=off -fsanitize=undefined,bounds,float-cast-overflow -fno-sanitize-recover=all -Isrc -I$(ENGINE) $(SOURCES) tests/core.c -lm -o build/core-ubsan
	build/core-ubsan
