CFLAGS=-g -W -Wall -Wextra -ansi -pedantic

TARGETS=kbdscan

all: $(TARGETS)

clean:
	rm -f $(TARGETS)
