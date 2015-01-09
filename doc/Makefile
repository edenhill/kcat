kafkacat.1: kafkacat.md
	ronn -r $^

kafkacat.1.html: kafkacat.md
	ronn -5 $^

doc: kafkacat.1 kafkacat.1.html

.PHONY: doc
