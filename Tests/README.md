# QuakeSpasm Unit Tests

This directory contains unit tests for QuakeSpasm using the Unity Test Framework.

## Building and Running Tests

### Linux/macOS
```bash
cd Tests
make
make run
```

### Windows (MinGW/MSYS2)
```bash
cd Tests
make
make run
```

### Windows (Visual Studio)
Open `Windows/VisualStudio/quakespasm.sln` and build the `quakespasm-tests` project.

## Test Coverage

- **test_mathlib**: Tests for vector math operations (mathlib.c)
  - Vector operations (add, subtract, scale, normalize)
  - Dot product and cross product
  - Vector length calculations
  - Angle operations
  - Utility functions

- **test_crc**: Tests for CRC calculations (crc.c)
  - CRC initialization
  - CRC block calculations
  - Incremental CRC processing

## Adding New Tests

1. Create a new test file: `test_<module>.c`
2. Include the Unity framework: `#include "unity/unity.h"`
3. Write test functions with `test_` prefix
4. Add to `Makefile` following existing patterns
5. Run `make run` to verify

## Unity Test Framework

We use a minimal embedded version of Unity Test Framework.
- Homepage: http://www.throwtheswitch.org/unity
- License: MIT
