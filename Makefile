CC      ?= cc
CFLAGS  ?= -O2
VERSION ?= 0.1.0-dev
SRC      = $(wildcard src/*.c)

ouroc0: $(SRC) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -std=c99 -Wall -Wextra -Isrc \
		-DOURO_VERSION='"$(VERSION)"' -o $@ $(SRC)

test: ouroc0
	bash tests/run.sh ./ouroc0

clean:
	rm -f ouroc0

.PHONY: test clean
