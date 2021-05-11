TESTS?=	$(wildcard [0][0-9][0-9][0-9]*.sh)

all: run


run:
	@(for test in $(TESTS) ; do \
		./$$test || \
		(echo "=== Test $$test FAILED ===" ; exit 1) ;\
	done)
