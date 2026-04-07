@echo off
setlocal
set ARCH=
set CC=gcc
set TEST=
goto :p1

:usage
echo usage: test-win32.bat [ options ... ] test targets...
echo options:
echo   -t target            target architecture (required)
echo   -c compiler          reference compiler (gcc or clang)
echo   -p path              prepend path to PATH
exit /B 1

:p0
shift
:p1
if "%1"=="-t" set ARCH=%2&&shift&&goto p0
if "%1"=="-c" set CC=%2&&shift&&goto p0
if "%1"=="-p" set PATH=%2;%PATH%&&shift&&goto p0
if not "%1"=="" set TEST=%TEST% %1&&goto p0
if "%ARCH%"=="" goto :usage
if "%TEST%"=="" set TEST=all -k

set CFG_MAK=..\config.mak
set CFG_H=..\config.h

set TFLAGS=msvcrt_start.c -nostdlib -lgcc -lkernel32 -Wl,-entry=_start
set TLIB=libtcc.dll
if not %CC%==clang goto :is_set
set TFLAGS=msvcrt_start.c -nostdlib -lkernel32
:is_set

echo>>%CFG_H% #define CC_NAME CC_%CC%
echo>%CFG_MAK% prefix = .
echo>>%CFG_MAK% CC = %CC%.exe
echo>>%CFG_MAK% CC_NAME = %CC%
echo>>%CFG_MAK% GCC_MAJOR = 15
echo>>%CFG_MAK% CFLAGS = -O0
echo>>%CFG_MAK% LDFLAGS =
echo>>%CFG_MAK% LIBSUF = .lib
echo>>%CFG_MAK% EXESUF = .exe
echo>>%CFG_MAK% DLLSUF = .dll
echo>>%CFG_MAK% ARCH = %ARCH%
echo>>%CFG_MAK% TARGETOS = WIN32
echo>>%CFG_MAK% CONFIG_WIN32 = yes
echo>>%CFG_MAK% VERSION = 0.9.28rc
echo>>%CFG_MAK% TOPSRC = $(TOP)
echo>>%CFG_MAK% TESTREF_FLAGS = %TFLAGS%
echo>>%CFG_MAK% SHELL = sh

set PATH=%CD%\..\win32;%PATH%
set TCC_LOCAL=%CD:\=/%/../tcc.exe
echo on
make TCC_LOCAL=%TCC_LOCAL% LIBTCC=win32/%TLIB% %TEST%
