/*
QuakeSpasm Unit Tests - Math Library Tests
Copyright (C) 2026 QuakeSpasm developers
*/

#include "unity/unity.h"
#include <math.h>
#include "quakedef.h"
#include "mathlib.h"
#include "mathlib.c"



/*-------------------------------------------------------
 * Vector Math Tests
 *-------------------------------------------------------*/

void test_DotProduct_orthogonal_vectors_returns_zero(void)
{
    vec3_t a = {1.0f, 0.0f, 0.0f};
    vec3_t b = {0.0f, 1.0f, 0.0f};
    float result = DotProduct(a, b);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result);
}

void test_DotProduct_parallel_vectors(void)
{
    vec3_t a = {3.0f, 0.0f, 0.0f};
    vec3_t b = {2.0f, 0.0f, 0.0f};
    float result = DotProduct(a, b);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, result);
}

void test_DotProduct_general_case(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    float result = DotProduct(a, b);
    /* 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32 */
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 32.0f, result);
}

void test_VectorLength_unit_vector(void)
{
    vec3_t v = {1.0f, 0.0f, 0.0f};
    float len = VectorLength(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, len);
}

void test_VectorLength_3_4_5_triangle(void)
{
    vec3_t v = {3.0f, 4.0f, 0.0f};
    float len = VectorLength(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, len);
}

void test_VectorLength_zero_vector(void)
{
    vec3_t v = {0.0f, 0.0f, 0.0f};
    float len = VectorLength(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, len);
}

void test_VectorNormalize_returns_length(void)
{
    vec3_t v = {3.0f, 4.0f, 0.0f};
    float len = VectorNormalize(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, len);
}

void test_VectorNormalize_produces_unit_vector(void)
{
    vec3_t v = {3.0f, 4.0f, 0.0f};
    VectorNormalize(v);
    float len = VectorLength(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, len);
}

void test_VectorNormalize_zero_vector_safe(void)
{
    vec3_t v = {0.0f, 0.0f, 0.0f};
    float len = VectorNormalize(v);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, len);
}

void test_CrossProduct_orthogonal_basis(void)
{
    vec3_t x_axis = {1.0f, 0.0f, 0.0f};
    vec3_t y_axis = {0.0f, 1.0f, 0.0f};
    vec3_t result;
    
    CrossProduct(x_axis, y_axis, result);
    
    /* X cross Y should give Z */
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, result[2]);
}

void test_CrossProduct_anticommutative(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    vec3_t result1, result2;
    
    CrossProduct(a, b, result1);
    CrossProduct(b, a, result2);
    
    /* a × b = -(b × a) */
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -result2[0], result1[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -result2[1], result1[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -result2[2], result1[2]);
}

void test_VectorAdd_simple(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {4.0f, 5.0f, 6.0f};
    vec3_t result;
    
    VectorAdd(a, b, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 7.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 9.0f, result[2]);
}

void test_VectorSubtract_simple(void)
{
    vec3_t a = {4.0f, 5.0f, 6.0f};
    vec3_t b = {1.0f, 2.0f, 3.0f};
    vec3_t result;
    
    VectorSubtract(a, b, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, result[2]);
}

void test_VectorScale_simple(void)
{
    vec3_t v = {1.0f, 2.0f, 3.0f};
    vec3_t result;
    
    VectorScale(v, 2.0f, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, result[2]);
}

void test_VectorScale_zero(void)
{
    vec3_t v = {1.0f, 2.0f, 3.0f};
    vec3_t result;
    
    VectorScale(v, 0.0f, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[2]);
}

void test_VectorInverse_simple(void)
{
    vec3_t v = {1.0f, -2.0f, 3.0f};
    
    VectorInverse(v);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -1.0f, v[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, v[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, -3.0f, v[2]);
}

void test_VectorCopy_simple(void)
{
    vec3_t src = {1.0f, 2.0f, 3.0f};
    vec3_t dst;
    
    VectorCopy(src, dst);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, dst[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, dst[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, dst[2]);
}

void test_VectorCompare_equal(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {1.0f, 2.0f, 3.0f};
    
    int result = VectorCompare(a, b);
    
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_VectorCompare_not_equal(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {1.0f, 2.1f, 3.0f};
    
    int result = VectorCompare(a, b);
    
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_VectorMA_simple(void)
{
    vec3_t a = {1.0f, 2.0f, 3.0f};
    vec3_t b = {1.0f, 0.0f, 0.0f};
    vec3_t result;
    
    /* result = a + 5 * b */
    VectorMA(a, 5.0f, b, result);
    
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, result[2]);
}

void test_ProjectPointOnPlane_simple(void)
{
    vec3_t point = {1.0f, 1.0f, 1.0f};
    vec3_t normal = {0.0f, 0.0f, 1.0f};  /* Z-axis */
    vec3_t result;
    
    ProjectPointOnPlane(result, point, normal);
    
    /* Projection onto XY plane should zero out Z */
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, result[2]);
}

void test_PerpendicularVector_is_orthogonal(void)
{
    vec3_t src = {1.0f, 0.0f, 0.0f};
    vec3_t dst;
    
    PerpendicularVector(dst, src);
    
    /* Result should be perpendicular (dot product = 0) */
    float dot = DotProduct(src, dst);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, dot);
}

void test_PerpendicularVector_is_unit_length(void)
{
    vec3_t src = {1.0f, 0.0f, 0.0f};
    vec3_t dst;
    
    PerpendicularVector(dst, src);
    
    /* Result should be normalized */
    float len = VectorLength(dst);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, len);
}

/*-------------------------------------------------------
 * Angle/Rotation Tests
 *-------------------------------------------------------*/

void test_anglemod_positive(void)
{
    float angle = anglemod(370.0f);
    /* Should wrap to ~10 degrees */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 10.0f, angle);
}

void test_anglemod_negative(void)
{
    float angle = anglemod(-10.0f);
    /* Should wrap to ~350 degrees */
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 350.0f, angle);
}

void test_anglemod_zero(void)
{
    float angle = anglemod(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, angle);
}

/*-------------------------------------------------------
 * Utility Tests
 *-------------------------------------------------------*/

void test_Q_log2_powers_of_two(void)
{
    TEST_ASSERT_EQUAL_INT(0, Q_log2(1));
    TEST_ASSERT_EQUAL_INT(1, Q_log2(2));
    TEST_ASSERT_EQUAL_INT(2, Q_log2(4));
    TEST_ASSERT_EQUAL_INT(3, Q_log2(8));
    TEST_ASSERT_EQUAL_INT(4, Q_log2(16));
    TEST_ASSERT_EQUAL_INT(10, Q_log2(1024));
}

void test_Q_log2_non_powers_of_two(void)
{
    TEST_ASSERT_EQUAL_INT(1, Q_log2(3));  /* floor(log2(3)) = 1 */
    TEST_ASSERT_EQUAL_INT(2, Q_log2(5));  /* floor(log2(5)) = 2 */
    TEST_ASSERT_EQUAL_INT(2, Q_log2(7));  /* floor(log2(7)) = 2 */
}

/*-------------------------------------------------------
 * Test Suite Runner
 *-------------------------------------------------------*/

int run_mathlib_tests(void)
{
    UNITY_BEGIN();
    
    /* Vector Math Tests */
    RUN_TEST(test_DotProduct_orthogonal_vectors_returns_zero);
    RUN_TEST(test_DotProduct_parallel_vectors);
    RUN_TEST(test_DotProduct_general_case);
    RUN_TEST(test_VectorLength_unit_vector);
    RUN_TEST(test_VectorLength_3_4_5_triangle);
    RUN_TEST(test_VectorLength_zero_vector);
    RUN_TEST(test_VectorNormalize_returns_length);
    RUN_TEST(test_VectorNormalize_produces_unit_vector);
    RUN_TEST(test_VectorNormalize_zero_vector_safe);
    RUN_TEST(test_CrossProduct_orthogonal_basis);
    RUN_TEST(test_CrossProduct_anticommutative);
    RUN_TEST(test_VectorAdd_simple);
    RUN_TEST(test_VectorSubtract_simple);
    RUN_TEST(test_VectorScale_simple);
    RUN_TEST(test_VectorScale_zero);
    RUN_TEST(test_VectorInverse_simple);
    RUN_TEST(test_VectorCopy_simple);
    RUN_TEST(test_VectorCompare_equal);
    RUN_TEST(test_VectorCompare_not_equal);
    RUN_TEST(test_VectorMA_simple);
    RUN_TEST(test_ProjectPointOnPlane_simple);
    RUN_TEST(test_PerpendicularVector_is_orthogonal);
    RUN_TEST(test_PerpendicularVector_is_unit_length);
    
    /* Angle Tests */
    RUN_TEST(test_anglemod_positive);
    RUN_TEST(test_anglemod_negative);
    RUN_TEST(test_anglemod_zero);
    
    /* Utility Tests */
    RUN_TEST(test_Q_log2_powers_of_two);
    RUN_TEST(test_Q_log2_non_powers_of_two);
    
    return UNITY_END();
}
