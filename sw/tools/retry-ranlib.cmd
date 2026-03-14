@echo off
setlocal

set TOOL=%~1
shift
set RETRIES=5
set DELAY=2
set COUNT=0

:retry
"%TOOL%" %*
if %ERRORLEVEL%==0 exit /b 0

set /a COUNT+=1
if %COUNT% GEQ %RETRIES% exit /b %ERRORLEVEL%

timeout /t %DELAY% /nobreak >nul
goto retry
