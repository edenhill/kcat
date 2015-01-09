all: doc

%.1: %.md
	ronn -r $^

%.1.html: %.md
	ronn -5 $^

doc: kafkacat.1 kafkacat.1.html

.PHONY: doc
