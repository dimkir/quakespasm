@echo off
REM QuakeSpasm Unit Tests - Simple Windows Build Script

echo QuakeSpasm Unit Tests - Build Script
echo =====================================
echo.

REM Try to find a compiler
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using GCC compiler...
    echo Building Unity framework...
    gcc -Wall -Wextra -g -I. -I../Quake -c unity/unity.c -o unity/unity.o
    
    echo Building mathlib tests...
    gcc -Wall -Wextra -g -I. -I../Quake test_mathlib.c unity/unity.o -o test_mathlib.exe
    
    echo Building CRC tests...
    gcc -Wall -Wextra -g -I. -I../Quake test_crc.c unity/unity.o -o test_crc.exe
    
    echo.
    echo Build completed!
    echo Run: test_mathlib.exe
    echo Run: test_crc.exe
    goto :end
)

where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using MSVC compiler...
    echo Building Unity framework...
    cl /c /nologo /W3 /I. /I..\Quake unity\unity.c /Founity\unity.obj
    
    echo Building mathlib tests...
    cl /nologo /W3 /I. /I..\Quake test_mathlib.c unity\unity.obj /Fe:test_mathlib.exe
    
    echo Building CRC tests...
    cl /nologo /W3 /I. /I..\Quake test_crc.c unity\unity.obj /Fe:test_crc.exe
    
    echo.
    echo Build completed!
    echo Run: test_mathlib.exe
    echo Run: test_crc.exe
    goto :end
)

echo ERROR: No C compiler found!
echo Please install MinGW/MSYS2 or Visual Studio
echo Or use the Visual Studio solution: Windows\VisualStudio\quakespasm.sln

:end
