TESTS?=	$(wildcard [0][0-9][0-9][0-9]*.sh)

all: run


run:
	@(for test in $(TESTS) ; do \
		./$$test || \
		(echo "=== Test $$test FAILED ===" ; exit 1) ;\
	done)


trivup:
	python3 -m pip install -r requirements.txt
	python3 -m trivup.clusters.KafkaCluster --sr --version 2.8.0 \
		--cmd "make run"
