@echo off

echo "[xStarbound::Build] Starting build..."

cd /D "%~dp0"
cd ..\..

"%PROGRAMFILES%\CMake\bin\cmake.exe" --preset "windows-x64"
IF %ERRORLEVEL% NEQ 0 (
    color 04
    echo "[xStarbound::Build] Configuration failed!"
    pause
    exit /b %ERRORLEVEL%
)

"%PROGRAMFILES%\CMake\bin\cmake.exe" --build --preset "windows-x64-release"
IF %ERRORLEVEL% NEQ 0 (
    color 04
    echo "[xStarbound::Build] Build failed!"
    pause
    exit /b %ERRORLEVEL%
)

:selectDirectory
echo "[xStarbound::Build] Waiting for directory selection."
:: Borrowed from this batch script on TenForums.com: https://www.tenforums.com/general-support/179377-bat-script-select-folder-update-path-bat-file.html
call :@ "Either select your Starbound install directory and click OK to install, or click Cancel to skip installation." SourceFolder
:@
set "@="(new-object -COM 'Shell.Application').BrowseForFolder(0,'%1',0x200,0).self.path""
for /f "usebackq delims=" %%# in (`PowerShell %@%`) do set "sbInstall=%%#"
:: ----------

if "%sbInstall%"=="" (
    echo "[xStarbound::Build] Build complete!"
    pause
    exit
)
if exist "%sbInstall%\assets\packed.pak" (
    echo "[xStarbound::Build] Installing xClient into chosen Starbound directory."
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --install cmake-build-windows-x64\ --prefix "%sbInstall%"
    IF %ERRORLEVEL% NEQ 0 (
        color 04
        echo "[xStarbound::Build] Installation failed!"
        exit /b %ERRORLEVEL%
    )
) else (
    echo "[xStarbound::Build] Not a valid Starbound directory!"
    goto selectDirectory
)

echo "[xStarbound::Build] Installation complete!"
pause