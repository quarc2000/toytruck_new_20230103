@echo off
setlocal

set RETRIES=5
set DELAY=2
set COUNT=0

:retry
xtensa-esp32-elf-ranlib.exe %*
if %ERRORLEVEL%==0 exit /b 0

set /a COUNT+=1
if %COUNT% GEQ %RETRIES% exit /b %ERRORLEVEL%

timeout /t %DELAY% /nobreak >nul
goto retry
