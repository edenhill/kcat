all: doc

%.1 %.1.html: %.md
	ronn $^

doc: kfc.1 kfc.1.html

.PHONY: doc

include ../mklove/Makefile.base

install: doc
	$(INSTALL) -d $$DESTDIR$(man1dir)
	$(INSTALL) kfc.1 $$DESTDIR$(man1dir)
