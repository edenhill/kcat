SRCS=src/common.c src/producer.c src/consumer.c src/metadata.c src/kfc.c
OBJS=$(SRCS:.c=.o)
BIN=src/kfc

.PHONY: doc

all: $(BIN)

include mklove/Makefile.base

# librdkafka must be compiled with -gstrict-dwarf, but kfc must not,
# due to some clang bug on OSX 10.9
CPPFLAGS := $(subst strict-dwarf,,$(CPPFLAGS))
CFLAGS := $(CFLAGS) -std=gnu99

install: bin-install install-man

install-man:
	echo $(INSTALL) -d $$DESTDIR$(man1dir) && \
	echo $(INSTALL) doc/kfc.1 $$DESTDIR$(man1dir)

doc:
	$(MAKE) -C doc

clean: bin-clean

dist-clean: clean
	rm -rf tmp
	rm -f config.* Makefile.config

-include $(DEPS)
