/* ==========================================
    Unity Project - A Test Framework for C
    Copyright (c) 2007-24 Mike Karlesky, Mark VanderVoord, Greg Williams
    [Released under MIT License. Please refer to license.txt for details]
========================================== */

#ifndef UNITY_FRAMEWORK_H
#define UNITY_FRAMEWORK_H
#define UNITY

#include <setjmp.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------------
 * Test Setup / Teardown
 *-------------------------------------------------------*/
/* These functions are intended to be called before and after each test. */
extern void (*setUp)(void);
extern void (*tearDown)(void);

/*-------------------------------------------------------
 * Test Running Macros
 *-------------------------------------------------------*/
#define RUN_TEST(testfunc) UnityDefaultTestRun(testfunc, #testfunc, __LINE__)

/*-------------------------------------------------------
 * Test Assertions
 *-------------------------------------------------------*/
#define TEST_ASSERT(condition)                                                          UNITY_TEST_ASSERT(       (condition), __LINE__, " Expected TRUE Was FALSE")
#define TEST_ASSERT_TRUE(condition)                                                     UNITY_TEST_ASSERT(       (condition), __LINE__, " Expected TRUE Was FALSE")
#define TEST_ASSERT_FALSE(condition)                                                    UNITY_TEST_ASSERT(      !(condition), __LINE__, " Expected FALSE Was TRUE")
#define TEST_ASSERT_NULL(pointer)                                                       UNITY_TEST_ASSERT_NULL(    (pointer), __LINE__, " Expected NULL")
#define TEST_ASSERT_NOT_NULL(pointer)                                                   UNITY_TEST_ASSERT_NOT_NULL((pointer), __LINE__, " Expected Non-NULL")

#define TEST_ASSERT_EQUAL_INT(expected, actual)                                         UNITY_TEST_ASSERT_EQUAL_INT((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_HEX16(expected, actual)                                       UNITY_TEST_ASSERT_EQUAL_HEX16((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_EQUAL_HEX32(expected, actual)                                       UNITY_TEST_ASSERT_EQUAL_HEX32((expected), (actual), __LINE__, NULL)

#define TEST_ASSERT_EQUAL_FLOAT(expected, actual)                                       UNITY_TEST_ASSERT_EQUAL_FLOAT((expected), (actual), __LINE__, NULL)
#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                               UNITY_TEST_ASSERT_FLOAT_WITHIN((delta), (expected), (actual), __LINE__, NULL)

/*-------------------------------------------------------
 * Test Framework Internals
 *-------------------------------------------------------*/
struct UNITY_STORAGE_T
{
    const char* TestFile;
    const char* CurrentTestName;
    unsigned int CurrentTestLineNumber;
    int NumberOfTests;
    int TestFailures;
    int TestIgnores;
    int CurrentTestFailed;
    int CurrentTestIgnored;
    jmp_buf AbortFrame;
};

extern struct UNITY_STORAGE_T Unity;

void UnityBegin(const char* filename);
int  UnityEnd(void);
void UnityConcludeTest(void);
void UnityDefaultTestRun(void (*Func)(void), const char* FuncName, const int FuncLineNum);

void UNITY_TEST_ASSERT(int condition, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_NULL(const void* pointer, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_NOT_NULL(const void* pointer, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_EQUAL_INT(int expected, int actual, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_EQUAL_HEX16(unsigned short expected, unsigned short actual, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_EQUAL_HEX32(unsigned int expected, unsigned int actual, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_EQUAL_FLOAT(float expected, float actual, unsigned int line, const char* msg);
void UNITY_TEST_ASSERT_FLOAT_WITHIN(float delta, float expected, float actual, unsigned int line, const char* msg);

/*-------------------------------------------------------
 * Convenience Macros
 *-------------------------------------------------------*/
#define UNITY_BEGIN()    UnityBegin(__FILE__)
#define UNITY_END()      UnityEnd()

#ifdef __cplusplus
}
#endif

#endif /* UNITY_FRAMEWORK_H */
