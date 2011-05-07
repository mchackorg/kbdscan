VERSION=20110507
DIST=kbdscan-$(VERSION)
DISTFILES=Makefile README kbdscan.c

CFLAGS=-g -W -Wall -Wextra -std=c99

TARGETS=kbdscan

RM=/bin/rm

all: $(TARGETS)

$(DIST).tar.bz2:
	mkdir $(DIST)
	cp $(DISTFILES) $(DIST)/
	tar cf $(DIST).tar --exclude .git $(DIST)
	bzip2 -9 $(DIST).tar
	$(RM) -rf $(DIST)

dist: $(DIST).tar.bz2

TAGS: *.c *.h
	-etags *.[ch]

clean:
	$(RM) -f $(TARGETS) *.o $(DIST).tar.bz2
