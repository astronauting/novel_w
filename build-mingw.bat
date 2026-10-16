@echo off
setlocal
set "ROOT=%~dp0"
set "GXX="
set "WINDRES="
set "MINGWLIB="

if exist "%USERPROFILE%\.conda\envs\novelw-build\Library\bin\g++.exe" (
  set "GXX=%USERPROFILE%\.conda\envs\novelw-build\Library\bin\g++.exe"
  set "WINDRES=%USERPROFILE%\.conda\envs\novelw-build\Library\bin\x86_64-w64-mingw32-windres.exe"
  set "MINGWLIB=%USERPROFILE%\.conda\envs\novelw-build\Library\x86_64-w64-mingw32\sysroot\usr\lib"
  set "PATH=%USERPROFILE%\.conda\envs\novelw-build\Library\bin;%PATH%"
)
if "%GXX%"=="" (
  where g++ >nul 2>&1 && set "GXX=g++"
)
if "%WINDRES%"=="" (
  where x86_64-w64-mingw32-windres >nul 2>&1 && set "WINDRES=x86_64-w64-mingw32-windres"
)
if "%WINDRES%"=="" (
  where windres >nul 2>&1 && set "WINDRES=windres"
)
if "%WINDRES%"=="" (
  echo windres not found. Install MinGW-w64 and add its bin directory to PATH.
  exit /b 1
)
if "%GXX%"=="" (
  echo 未找到 g++。请安装 Visual Studio（C++ 桌面开发）后运行 build.bat，或安装 MinGW-w64。
  exit /b 1
)

mkdir "%ROOT%obj\Release" 2>nul
mkdir "%ROOT%bin\Release" 2>nul

echo Compiling resources...
"%WINDRES%" -I "%ROOT%src" --codepage=65001 "%ROOT%src\resource.rc" -O coff -o "%ROOT%obj\Release\resource.o"
if errorlevel 1 exit /b 1

set "BFLAG="
if not "%MINGWLIB%"=="" set "BFLAG=-B%MINGWLIB%/"

echo Compiling novel_w...
"%GXX%" -std=c++17 -municode -mwindows -O2 -I"%ROOT%src" -DUNICODE -D_UNICODE -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 %BFLAG% ^
  "%ROOT%src\WinMain.cpp" "%ROOT%src\AppWindow.cpp" "%ROOT%src\EditorCtrl.cpp" "%ROOT%src\ChapterIndex.cpp" "%ROOT%src\FileIo.cpp" "%ROOT%src\Settings.cpp" "%ROOT%src\Appearance.cpp" "%ROOT%src\Hotkeys.cpp" ^
  "%ROOT%obj\Release\resource.o" ^
  -o "%ROOT%bin\Release\novel_w.exe" ^
  -lcomctl32 -lcomdlg32 -lshlwapi -lshell32 -lgdi32 -luser32 -limm32 -lmsimg32 -luxtheme -static -static-libgcc -static-libstdc++
if errorlevel 1 exit /b 1
echo Built %ROOT%bin\Release\novel_w.exe
exit /b 0
