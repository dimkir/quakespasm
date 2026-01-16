/*
QuakeSpasm Unit Tests - Main Test Runner
Copyright (C) 2026 QuakeSpasm developers
*/

#include "unity/unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Stub for Quake's Sys_Error function */
void Sys_Error(const char *error, ...)
{
    va_list argptr;
    char string[1024];
    
    va_start(argptr, error);
    vsnprintf(string, sizeof(string), error, argptr);
    va_end(argptr);
    
    fprintf(stderr, "Sys_Error: %s\n", string);
    exit(1);
}

/* Forward declarations for test suites */
extern int run_mathlib_tests(void);
extern int run_crc_tests(void);

int main(void)
{
    int failures = 0;
    
    failures += run_mathlib_tests();
    failures += run_crc_tests();
    
    return (failures == 0) ? 0 : 1;
}
