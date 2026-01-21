# QuakeSpasm Unit Tests Makefile
# Copyright (C) 2026 QuakeSpasm developers

CC = gcc
CFLAGS = -Wall -Wextra -g -I. -I../Quake
LDFLAGS = -lm

# Test executables
TEST_MATHLIB = test_mathlib
TEST_CRC = test_crc
ALL_TESTS = $(TEST_MATHLIB) $(TEST_CRC)

# Unity framework
UNITY_SRC = unity/unity.c
UNITY_OBJ = unity/unity.o

.PHONY: all clean run

all: $(ALL_TESTS)

# Unity framework
$(UNITY_OBJ): $(UNITY_SRC) unity/unity.h
	$(CC) $(CFLAGS) -c $(UNITY_SRC) -o $(UNITY_OBJ)

# Math library tests
$(TEST_MATHLIB): test_mathlib.c $(UNITY_OBJ)
	$(CC) $(CFLAGS) test_mathlib.c $(UNITY_OBJ) -o $(TEST_MATHLIB) $(LDFLAGS)

# CRC tests
$(TEST_CRC): test_crc.c $(UNITY_OBJ)
	$(CC) $(CFLAGS) test_crc.c $(UNITY_OBJ) -o $(TEST_CRC) $(LDFLAGS)

# Run all tests
run: all
	@echo "Running Math Library Tests..."
	@./$(TEST_MATHLIB)
	@echo ""
	@echo "Running CRC Tests..."
	@./$(TEST_CRC)
	@echo ""
	@echo "All tests completed!"

clean:
	rm -f $(ALL_TESTS) $(UNITY_OBJ)
	rm -f *.o *.exe
