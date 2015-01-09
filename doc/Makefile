all: doc

%.1 %.1.html: %.md
	ronn $^

doc: kafkacat.1 kafkacat.1.html

.PHONY: doc
