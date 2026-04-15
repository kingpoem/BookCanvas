@echo off
setlocal

set SCRIPT_DIR=%~dp0
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%package-windows.ps1" %*
set EXIT_CODE=%ERRORLEVEL%

if not "%EXIT_CODE%"=="0" (
  echo.
  echo Packaging failed with exit code %EXIT_CODE%.
  exit /b %EXIT_CODE%
)

echo.
echo Packaging completed successfully.
exit /b 0
