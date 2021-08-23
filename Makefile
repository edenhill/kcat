include Makefile.config

BIN=	kcat

SRCS_y=	kcat.c format.c tools.c input.c
SRCS_$(ENABLE_JSON) += json.c
SRCS_$(ENABLE_AVRO) += avro.c
OBJS=	$(SRCS_y:.c=.o)

.PHONY:

all: $(BIN) TAGS

include mklove/Makefile.base

# librdkafka must be compiled with -gstrict-dwarf, but kcat must not,
# due to some clang bug on OSX 10.9
CPPFLAGS := $(subst strict-dwarf,,$(CPPFLAGS))

install: bin-install install-man

install-man:
	echo $(INSTALL) -d $$DESTDIR$(man1dir) && \
	echo $(INSTALL) kcat.1 $$DESTDIR$(man1dir)


clean: bin-clean

test:
	$(MAKE) -C tests

TAGS: .PHONY
	@(if which etags >/dev/null 2>&1 ; then \
		echo "Using etags to generate $@" ; \
		git ls-tree -r --name-only HEAD | egrep '\.(c|cpp|h)$$' | \
			etags -f $@.tmp - ; \
		cmp $@ $@.tmp || mv $@.tmp $@ ; rm -f $@.tmp ; \
	 elif which ctags >/dev/null 2>&1 ; then \
		echo "Using ctags to generate $@" ; \
		git ls-tree -r --name-only HEAD | egrep '\.(c|cpp|h)$$' | \
			ctags -e -f $@.tmp -L- ; \
		cmp $@ $@.tmp || mv $@.tmp $@ ; rm -f $@.tmp ; \
	fi)


-include $(DEPS)
