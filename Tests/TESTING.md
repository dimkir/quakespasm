# QuakeSpasm Unit Tests - Complete Setup Guide

## Overview

This directory contains unit tests for QuakeSpasm using the Unity Test Framework. The tests focus on verifying core functionality of math operations, CRC calculations, and other utility functions.

## Directory Structure

```
Tests/
├── unity/              # Unity Test Framework
│   ├── unity.h
│   └── unity.c
├── test_mathlib.c      # Math library tests (vectors, angles, etc.)
├── test_crc.c          # CRC calculation tests
├── Makefile            # Linux/macOS/MinGW build
├── build.bat           # Windows batch build script
├── build.ps1           # Windows PowerShell build script
├── run_tests.ps1       # Windows test runner
└── README.md           # This file
```

## Building Tests

### Linux / macOS

```bash
cd Tests
make
```

### Windows - Option 1: Visual Studio

1. Open `Windows\VisualStudio\quakespasm.sln`
2. Select the `quakespasm-tests` project
3. Build (F7)
4. Run from: `Windows\VisualStudio\Build-quakespasm-tests\x64\Debug\quakespasm-tests.exe`

### Windows - Option 2: Command Line (with MinGW/MSYS2)

```bash
cd Tests
make
```

### Windows - Option 3: Batch Script

```cmd
cd Tests
build.bat
```

### Windows - Option 4: PowerShell Script

```powershell
cd Tests
.\build.ps1
```

## Running Tests

### Linux / macOS

```bash
cd Tests
make run
# Or run individually:
./test_mathlib
./test_crc
```

### Windows

```cmd
cd Tests
test_mathlib.exe
test_crc.exe
```

Or use the PowerShell test runner:

```powershell
cd Tests
.\run_tests.ps1
```

## Test Coverage

### test_mathlib.c
Tests for vector and math operations from `mathlib.c`:

**Vector Operations:**
- `DotProduct()` - Dot product calculations
- `CrossProduct()` - Cross product calculations
- `VectorLength()` - Vector magnitude
- `VectorNormalize()` - Vector normalization
- `VectorAdd()`, `VectorSubtract()` - Basic arithmetic
- `VectorScale()` - Scalar multiplication
- `VectorInverse()` - Vector negation
- `VectorCopy()` - Vector copying
- `VectorCompare()` - Equality testing
- `VectorMA()` - Multiply and add
- `ProjectPointOnPlane()` - Projection operations
- `PerpendicularVector()` - Perpendicular vector generation

**Angle Operations:**
- `anglemod()` - Angle modulo operations

**Utilities:**
- `Q_log2()` - Integer log2 calculations

**Total: 29 test cases**

### test_crc.c
Tests for CRC calculations from `crc.c`:

- `CRC_Init()` - Initialization
- `CRC_Value()` - Final value calculation
- `CRC_ProcessByte()` - Incremental processing
- `CRC_Block()` - Block processing
- Known value verification
- Consistency testing
- Order sensitivity

**Total: 10 test cases**

## Continuous Integration

Tests are automatically run on GitHub Actions for Linux builds. See `.github/workflows/build-linux.yml`.

## Adding New Tests

### 1. Create a new test file

```c
#include "unity/unity.h"

/* Include minimal type definitions needed */
/* Include the module to test */

void setUp(void) { }
void tearDown(void) { }

void test_your_function_does_something(void)
{
    /* Arrange */
    int input = 5;
    
    /* Act */
    int result = your_function(input);
    
    /* Assert */
    TEST_ASSERT_EQUAL_INT(10, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_your_function_does_something);
    return UNITY_END();
}
```

### 2. Add to Makefile

```makefile
TEST_YOUR_MODULE = test_your_module
ALL_TESTS = $(TEST_MATHLIB) $(TEST_CRC) $(TEST_YOUR_MODULE)

$(TEST_YOUR_MODULE): test_your_module.c $(UNITY_OBJ)
	$(CC) $(CFLAGS) test_your_module.c $(UNITY_OBJ) -o $(TEST_YOUR_MODULE) $(LDFLAGS)
```

### 3. Add to Visual Studio project

1. Right-click `quakespasm-tests` project
2. Add → Existing Item
3. Select `Tests\test_your_module.c`
4. Rebuild

### 4. Update CI workflow

Edit `.github/workflows/build-linux.yml`:

```yaml
- name: Run Unit Tests
  run: cd Tests && ./test_mathlib && ./test_crc && ./test_your_module
```

## Unity Test Framework Assertions

Common assertions available:

```c
TEST_ASSERT(condition)
TEST_ASSERT_TRUE(condition)
TEST_ASSERT_FALSE(condition)
TEST_ASSERT_NULL(pointer)
TEST_ASSERT_NOT_NULL(pointer)
TEST_ASSERT_EQUAL_INT(expected, actual)
TEST_ASSERT_EQUAL_HEX16(expected, actual)
TEST_ASSERT_EQUAL_HEX32(expected, actual)
TEST_ASSERT_EQUAL_FLOAT(expected, actual)
TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)
```

## Best Practices

1. **Test one thing per test** - Keep tests focused
2. **Use descriptive names** - `test_module_function_condition()`
3. **Follow AAA pattern** - Arrange, Act, Assert
4. **Test edge cases** - Zero values, negative numbers, boundaries
5. **Test error conditions** - Invalid inputs, NULL pointers
6. **Keep tests independent** - No shared state between tests

## Troubleshooting

### "Cannot find mathlib.h"
Make sure include paths are correct: `-I. -I../Quake`

### "Multiple definition of 'vec3_origin'"
This happens when including .c files directly. Only include the specific .c file being tested.

### "Undefined reference to Sys_Error"
Some Quake functions depend on engine infrastructure. You may need to provide stub implementations or refactor to make the code more testable.

### Tests compile but crash
Check that all type definitions (vec3_t, byte, etc.) are properly defined before including test code.

## License

Unity Test Framework: MIT License  
QuakeSpasm Tests: GPL v2 (same as QuakeSpasm)

## References

- Unity Test Framework: http://www.throwtheswitch.org/unity
- QuakeSpasm: http://quakespasm.sourceforge.net/
