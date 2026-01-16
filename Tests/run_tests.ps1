# QuakeSpasm Unit Tests - Test Runner
$vsPath = "..\Windows\VisualStudio\Build-quakespasm-tests\x64\Debug\quakespasm-tests.exe"
if (Test-Path $vsPath) {
    & $vsPath
    if ($LASTEXITCODE -eq 0) {
        Write-Host "All tests passed!" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "Some tests failed!" -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "Tests not built. Run build.ps1 first." -ForegroundColor Red
    exit 1
}