

all: run


run:
	@(for test in 0[0-9][0-9][0-9]*.sh ; do \
		( ./$$test && \
		echo "=== Test $$test PASSED ===" ) || \
		(echo "=== Test $$test FAILED ===" ; exit 1) ;\
	done)
