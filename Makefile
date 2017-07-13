include Makefile.config

BIN=	kafkacat

SRCS_y=	kafkacat.c format.c tools.c
SRCS_$(ENABLE_JSON) += json.c
OBJS=	$(SRCS_y:.c=.o)

.PHONY:

all: $(BIN)

include mklove/Makefile.base

# librdkafka must be compiled with -gstrict-dwarf, but kafkacat must not,
# due to some clang bug on OSX 10.9
CPPFLAGS := $(subst strict-dwarf,,$(CPPFLAGS))

install: bin-install install-man

install-man:
	echo $(INSTALL) -d $$DESTDIR$(man1dir) && \
	echo $(INSTALL) kafkacat.1 $$DESTDIR$(man1dir)


clean: bin-clean

-include $(DEPS)
