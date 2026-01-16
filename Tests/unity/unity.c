/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-24 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#include "unity.h"
#include <stdio.h>
#include <string.h>

struct UNITY_STORAGE_T Unity;

void (*setUp)(void) = NULL;
void (*tearDown)(void) = NULL;

static const char UnityStrOk[] = "OK";
static const char UnityStrPass[] = "PASS";
static const char UnityStrFail[] = "FAIL";
static const char UnityStrIgnore[] = "IGNORE";

/*-------------------------------------------------------
 * Test Running
 *-------------------------------------------------------*/
void UnityBegin(const char* filename)
{
    Unity.TestFile = filename;
    Unity.CurrentTestName = NULL;
    Unity.CurrentTestLineNumber = 0;
    Unity.NumberOfTests = 0;
    Unity.TestFailures = 0;
    Unity.TestIgnores = 0;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

int UnityEnd(void)
{
    printf("\n-----------------------\n");
    printf("%d Tests %d Failures %d Ignored\n", 
           Unity.NumberOfTests, Unity.TestFailures, Unity.TestIgnores);
    
    if (Unity.TestFailures == 0)
        printf("%s\n", UnityStrOk);
    
    return (Unity.TestFailures == 0) ? 0 : 1;
}

void UnityConcludeTest(void)
{
    if (Unity.CurrentTestIgnored)
    {
        Unity.TestIgnores++;
        printf(":%d:%s:%s\n", Unity.CurrentTestLineNumber, Unity.CurrentTestName, UnityStrIgnore);
    }
    else if (!Unity.CurrentTestFailed)
    {
        printf(":%d:%s:%s\n", Unity.CurrentTestLineNumber, Unity.CurrentTestName, UnityStrPass);
    }
    else
    {
        Unity.TestFailures++;
    }
    
    Unity.NumberOfTests++;
    Unity.CurrentTestFailed = 0;
    Unity.CurrentTestIgnored = 0;
}

void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum)
{
    Unity.CurrentTestName = FuncName;
    Unity.CurrentTestLineNumber = (unsigned int)FuncLineNum;
    Unity.NumberOfTests++;
    
    if (setUp) setUp();
    
    if (setjmp(Unity.AbortFrame) == 0)
    {
        Func();
    }
    
    if (tearDown) tearDown();
    
    UnityConcludeTest();
}

/*-------------------------------------------------------
 * Assertion Functions
 *-------------------------------------------------------*/
void UNITY_TEST_ASSERT(int condition, unsigned int line, const char* msg)
{
    if (!condition)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL:%s\n", Unity.TestFile, line, Unity.CurrentTestName, msg);
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_NULL(const void* pointer, unsigned int line, const char* msg)
{
    if (pointer != NULL)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL:%s\n", Unity.TestFile, line, Unity.CurrentTestName, msg);
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_NOT_NULL(const void* pointer, unsigned int line, const char* msg)
{
    if (pointer == NULL)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL:%s\n", Unity.TestFile, line, Unity.CurrentTestName, msg);
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_EQUAL_INT(int expected, int actual, unsigned int line, const char* msg)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL: Expected %d Was %d", Unity.TestFile, line, Unity.CurrentTestName, expected, actual);
        if (msg) printf(" %s", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_EQUAL_HEX16(unsigned short expected, unsigned short actual, unsigned int line, const char* msg)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL: Expected 0x%04X Was 0x%04X", Unity.TestFile, line, Unity.CurrentTestName, expected, actual);
        if (msg) printf(" %s", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_EQUAL_HEX32(unsigned int expected, unsigned int actual, unsigned int line, const char* msg)
{
    if (expected != actual)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL: Expected 0x%08X Was 0x%08X", Unity.TestFile, line, Unity.CurrentTestName, expected, actual);
        if (msg) printf(" %s", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_EQUAL_FLOAT(float expected, float actual, unsigned int line, const char* msg)
{
    if (fabs(expected - actual) > 0.00001f)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL: Expected %f Was %f", Unity.TestFile, line, Unity.CurrentTestName, expected, actual);
        if (msg) printf(" %s", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}

void UNITY_TEST_ASSERT_FLOAT_WITHIN(float delta, float expected, float actual, unsigned int line, const char* msg)
{
    float diff = fabs(expected - actual);
    if (diff > delta)
    {
        Unity.CurrentTestFailed = 1;
        printf("%s:%d:%s:FAIL: Expected %f +/- %f Was %f (diff %f)", 
               Unity.TestFile, line, Unity.CurrentTestName, expected, delta, actual, diff);
        if (msg) printf(" %s", msg);
        printf("\n");
        longjmp(Unity.AbortFrame, 1);
    }
}
