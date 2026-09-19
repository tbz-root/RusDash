@echo off
REM Run the Android CPython build inside WSL (recommended on Windows).
REM Usage:
REM   set ANDROID_NDK=/path/inside/wsl/to/ndk
REM   scripts\build_python_android.bat

setlocal
if "%ANDROID_NDK%"=="" (
  echo Set ANDROID_NDK to the NDK path visible from WSL, e.g.
  echo   set ANDROID_NDK=/home/you/Android/Sdk/ndk/27.0.12077973
  exit /b 1
)

wsl bash -lc "export ANDROID_NDK='%ANDROID_NDK%' PYTHON_VERSION='%PYTHON_VERSION%' ANDROID_API='%ANDROID_API%' CLEAN='%CLEAN%'; cd '%CD:\=/%' 2>/dev/null || cd \"$(wslpath '%CD%')\"; ./scripts/build_python_android.sh"
endlocal
