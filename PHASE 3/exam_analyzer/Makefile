CC     ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LIB    := $(filter-out src/main.c,$(wildcard src/*.c))
HDR    := $(wildcard src/*.h)

all: exam_analyzer

exam_analyzer: $(LIB) src/main.c $(HDR)
	$(CC) $(CFLAGS) -o $@ $(LIB) src/main.c

test_ds: $(LIB) tests/test_ds.c $(HDR)
	$(CC) $(CFLAGS) -Isrc -o $@ $(LIB) tests/test_ds.c

test: test_ds
	./test_ds

run: exam_analyzer
	./exam_analyzer data/questions.txt data/syllabus.txt data/prereqs.txt

clean:
	rm -f exam_analyzer test_ds

.PHONY: all test run clean
