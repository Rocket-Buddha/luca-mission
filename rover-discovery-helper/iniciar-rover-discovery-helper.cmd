@echo off
setlocal

cd /d "%~dp0"
title Rover Discovery Helper

echo [SYS] starting rover discovery helper...
echo [SYS] working directory: %CD%
echo.

py -u ".\discovery_helper.py" %*
set "exit_code=%ERRORLEVEL%"

if not "%exit_code%"=="0" (
  echo.
  echo [SYS] helper exited with code %exit_code%.
  pause
)

exit /b %exit_code%
