echo off

echo "Starting build..."

cd /d %~dp0
cd ..\..

mkdir build
cd build

"C:\Program Files (x86)\CMake\bin\cmake.exe" --build --preset "windows-x64-release"

cd ..
copy source\extern\steam\lib\windows\steam_api64.dll .\dist\steam_api64.dll

if %ERRORLEVEL% NEQ 0 (
    echo "Build error occurred! Ensure VS 2022 and CMake are installed and check logs."
) else (
    echo "Finished build."
)
