# QuakeSpasm Unit Tests - Windows Build Script
# Requires Visual Studio or MinGW

Write-Host "QuakeSpasm Unit Tests - Build Script" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Check for compiler
$hasGcc = Get-Command gcc -ErrorAction SilentlyContinue
$hasCl = Get-Command cl -ErrorAction SilentlyContinue
$msbuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe"
if (-not (Test-Path $msbuildPath)) {
    $msbuildPath = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\msbuild.exe"
}
$hasMsbuild = Test-Path $msbuildPath

if ($hasMsbuild) {
    Write-Host "Using MSBuild (Visual Studio)..." -ForegroundColor Green
    
    $slnPath = "..\Windows\VisualStudio\quakespasm.sln"
    $projectName = "quakespasm-tests"
    
    Write-Host "Building $projectName project..."
    & $msbuildPath $slnPath /t:$projectName /p:Configuration=Debug /p:Platform=x64 /v:minimal
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Build completed!" -ForegroundColor Green
        Write-Host "Test executable: ..\Windows\VisualStudio\Build-quakespasm-tests\x64\Debug\quakespasm-tests.exe" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "Run tests with: ..\Windows\VisualStudio\Build-quakespasm-tests\x64\Debug\quakespasm-tests.exe" -ForegroundColor Cyan
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
} elseif ($hasGcc) {
    Write-Host "Using GCC compiler..." -ForegroundColor Green
    
    # Build Unity framework
    Write-Host "Building Unity framework..."
    gcc -Wall -Wextra -g -I. -I../Quake -c unity/unity.c -o unity/unity.o
    
    if ($LASTEXITCODE -eq 0) {
        # Build mathlib tests
        Write-Host "Building mathlib tests..."
        gcc -Wall -Wextra -g -I. -I../Quake test_mathlib.c unity/unity.o -o test_mathlib.exe -lm
        
        # Build CRC tests
        Write-Host "Building CRC tests..."
        gcc -Wall -Wextra -g -I. -I../Quake test_crc.c unity/unity.o -o test_crc.exe -lm
        
        Write-Host ""
        Write-Host "Build completed! Run .\run_tests.ps1 to execute tests." -ForegroundColor Green
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
} elseif ($hasCl) {
    Write-Host "Using MSVC compiler..." -ForegroundColor Green
    
    # Build Unity framework
    Write-Host "Building Unity framework..."
    cl /c /nologo /W3 /I. /I..\Quake unity\unity.c /Founity\unity.obj
    
    if ($LASTEXITCODE -eq 0) {
        # Build mathlib tests
        Write-Host "Building mathlib tests..."
        cl /nologo /W3 /I. /I..\Quake test_mathlib.c unity\unity.obj /Fe:test_mathlib.exe
        
        # Build CRC tests  
        Write-Host "Building CRC tests..."
        cl /nologo /W3 /I. /I..\Quake test_crc.c unity\unity.obj /Fe:test_crc.exe
        
        Write-Host ""
        Write-Host "Build completed! Run .\run_tests.ps1 to execute tests." -ForegroundColor Green
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
        exit 1
    }
    
} else {
    Write-Host "ERROR: No C compiler found!" -ForegroundColor Red
    Write-Host "Please install one of:" -ForegroundColor Yellow
    Write-Host "  - Visual Studio (MSBuild at: $msbuildPath)" -ForegroundColor Yellow
    Write-Host "  - MinGW/MSYS2 (gcc)" -ForegroundColor Yellow
    Write-Host "  - Visual Studio Build Tools (cl)" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Or open the Visual Studio solution and build the quakespasm-tests project." -ForegroundColor Yellow
    exit 1
}
