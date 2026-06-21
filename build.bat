@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "SRC_DIR=%SCRIPT_DIR%source"
set "BUILD_ROOT=%SRC_DIR%\build"
set "JOBS=%NUMBER_OF_PROCESSORS%"

call :build Debug   double 1 || exit /b 1
call :build Release double 1 || exit /b 1
call :build Debug   float  0 || exit /b 1
call :build Release float  0 || exit /b 1

echo All builds succeeded.
exit /b 0

:build
set "build_type=%~1"
set "precision=%~2"
set "dbl=%~3"
set "build_dir=%BUILD_ROOT%\%build_type%-%precision%"

echo ==============================================================
echo  %build_type% / %precision%  -^> %build_dir%
echo ==============================================================

if exist "%build_dir%" rmdir /s /q "%build_dir%"

cmake -S "%SRC_DIR%" -B "%build_dir%" -G Ninja -DCMAKE_BUILD_TYPE=%build_type% -DUNIGINE_DOUBLE=%dbl%
if errorlevel 1 exit /b 1

cmake --build "%build_dir%" -j %JOBS%
if errorlevel 1 exit /b 1

exit /b 0
