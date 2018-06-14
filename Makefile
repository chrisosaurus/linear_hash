.POSIX:

include config.mk

SRC = linear_hash.c
OBJ = ${SRC:.c=.o}

EXTRAFLAGS =

# default to error all: linear_hash 
%.o: %.c
	@echo COMPILING CC $< with extra flags \"${EXTRAFLAGS}\"
	@${CC} -g -c ${CFLAGS} $< ${EXTRAFLAGS} -o $@

linear_hash: ${OBJ}
	@echo "ERROR: unable to compile linear_hash on it's own as it is a library"
	exit 1

cleanobj:
	@echo cleaning objects and temporary files
	@find . -iname '*.o' -delete

clean: cleanobj
	@echo cleaning tests
	@rm -f test_lh
	@rm -f test_glh
	@rm -f profile_lh
	@rm -f example
	@echo cleaning gcov guff
	@find . -iname '*.gcda' -delete
	@find . -iname '*.gcov' -delete
	@find . -iname '*.gcno' -delete
	@rm -rf profile_keys.c
	@rm -rf moby_dick.txt

test: clean run_tests

test_lh: ${OBJ}
	@echo "compiling tests"
	@${CC} test_linear_hash.c -o test_lh ${LDFLAGS} ${OBJ}
	@make -s cleanobj

run_tests: test_lh
	@echo -e "\n\nrunning test_lh"
	./test_lh
	@echo -e "\n"

profile_lh: ${OBJ} profile_keys.c
	@echo ${OBJ}
	@echo "compiling profile_lh"
	@${CC} profile_lh.c -o profile_lh ${LDFLAGS} ${OBJ}

profile: clean profile_lh
	@echo -e "\n\nrunning profile_lh"
	./profile_lh
	@echo -e "\n"

example: ${OBJ}
	@echo "compiling and running example"
	@${CC} example.c -o example ${LDFLAGS} ${OBJ}
	./example

moby_dick.txt:
	@echo "Downloading moby dick text ~1.2 MiB"
	curl http://www.gutenberg.org/files/2701/2701.txt -o moby_dick.txt

profile_keys.c: moby_dick.txt
	@echo -e "Generating profile_keys.sh\n"
	./generate_profile_keys.sh

.PHONY: all clean cleanobj linear_hash profle run_tests test

