CC = gcc
CFLAGS = -Wall -Werror -std=gnu11
PROFILE_FLAGS = -fprofile-arcs -ftest-coverage  # or: --coverage
TEST_LIBS = $(shell pkg-config --libs check)
COV_LIBS = -lgcov  # or: --coverage

BUILD_DIR = build
EXECUTABLE = $(BUILD_DIR)/hw2
TEST_EXECUTABLE = $(BUILD_DIR)/hw2_test

SRC_DIR = src
TEST_DIR = tests
SRCS = $(shell find $(SRC_DIR) -name '.ccls-cache' -type d -prune -o -type f -name '*.c' -print)
HEADERS = $(shell find $(SRC_DIR) -name '.ccls-cache' -type d -prune -o -type f -name '*.h' -print)
TEST_SRCS = $(shell find $(TEST_DIR) -name '.ccls-cache' -type d -prune -o -type f -name '*.c' -print)

GCOV = gcovr
GCOV_HTML_TARGET = $(BUILD_DIR)/coverage_report.html
GCOV_FLAGS = -r src --txt --html=$(GCOV_HTML_TARGET) --html-details -d $(BUILD_DIR)

RED = \033[0;31m
YELLOW = \033[1;33m
GREEN = \033[0;32m
CYAN = \033[0;36m
NC = \033[0m


.PHONY: all release debug --build-test test valgrind clean
.SILENT: --build-test test valgrind clean


all: release

release: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -O2 $(SRCS) main.c -o $(EXECUTABLE)

debug: $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) -O0 $(SRCS) main.c -o $(EXECUTABLE)

--build-test: clean $(SRCS) $(HEADERS) $(TEST_SRCS)
	$(CC) $(CFLAGS) -O2 $(PROFILE_FLAGS) $(TEST_SRCS) $(SRCS) $(TEST_LIBS) $(COV_LIBS) -o $(TEST_EXECUTABLE)

test: --build-test
	printf "${YELLOW}================\nRunning tests...\n================\n\n${NC}"
	$(TEST_EXECUTABLE) && \
		printf "${GREEN}\n=================\nAll tests passed!\n=================\n\n${NC}" || \
		printf "${RED}\n====================\nSome tests failed :(\n====================\n\n${NC}"
	printf "${YELLOW}" && \
		$(GCOV) $(GCOV_FLAGS) && \
		printf "${CYAN}\nOpen coverage report via link:\nfile://$(shell realpath $(GCOV_HTML_TARGET))\n${NC}"
	clang-tidy $(SRCS) $(HEADERS) -- && \
		printf "${GREEN}\n======================\nNo style errors found!\n======================\n${NC}" || \
		printf "${RED}\n====================\nStyle errors found :(\n====================\n${NC}"

valgrind: --build-test
	printf "${YELLOW}================\nRunning tests...\n================\n\n${NC}"
	CK_FORK=no valgrind --leak-check=full $(TEST_EXECUTABLE) && \
	    printf "${GREEN}\n=================\nAll tests passed!\n=================\n${NC}" || \
	    printf "${RED}\n====================\nSome tests failed :(\n====================\n${NC}"

clean:
	# *.o $(EXECUTABLE) $(TEST_EXECUTABLE) *.gcno *.gcda *.css *.html
	rm -f $(BUILD_DIR)/*

