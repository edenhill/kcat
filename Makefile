
BIN=	kafkacat

SRCS=	kafkacat.c
OBJS=	$(SRCS:.c=.o)

.PHONY:

all: $(BIN)

include mklove/Makefile.base

install: bin-install

clean: bin-clean
