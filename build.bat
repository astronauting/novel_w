@echo off
setlocal EnableDelayedExpansion
set "MSBUILD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
)
if not "%MSBUILD%"=="" (
  echo Building with MSBuild...
  "%MSBUILD%" "%~dp0NovelW.sln" /p:Configuration=Release /p:Platform=x64 /m
  exit /b !ERRORLEVEL!
  )
echo 未找到 Visual Studio MSBuild，改用 MinGW...
call "%~dp0build-mingw.bat"
exit /b %ERRORLEVEL%
