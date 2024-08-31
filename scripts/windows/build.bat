@echo off

echo "[xStarbound::Build] Starting build..."

cd /D "%~dp0"
cd ..\..

if exist "%PROGRAMFILES(X86)%\Inno Setup 6" (
    call :yesNoBox "Do you want to build the xStarbound installer? Click 'No' if you want to use this script for direct installation." "xStarbound Build Script"
    if "%YesNo%"=="6" (
        echo "[xStarbound::Build] Will build installer."
        set "buildInstaller=yes"
    ) else (
        echo "[xStarbound::Build] Will not build installer."
        set "buildInstaller=no"
    )
) else (
    call :yesNoBox "If you want to build the xStarbound installer, click 'No' to open the Inno Setup download page in your browser. Restart this script once you've installed Inno Setup. Otherwise, click 'Yes' to continue; you may still use this script for direct installation." "xStarbound Build Script"
    set "buildInstaller=no"
    if "%YesNo%"=="6" (
        echo "[xStarbound::Build] Will not build installer."
    ) else (
        echo "[xStarbound::Build] Opening Inno Setup 6 website..."
        rundll32 url.dll,FileProtocolHandler "https://jrsoftware.org/isdl.php"
        exit /b
    )
)

"%PROGRAMFILES%\CMake\bin\cmake.exe" --preset "windows-x64" -DSTAR_INSTALL_VCPKG=ON -DBUILD_INNOSETUP=OFF -UCMAKE_TOOLCHAIN_FILE
if %ERRORLEVEL% neq 0 (
    color 04
    set buildError=%ERRORLEVEL%
    echo "[xStarbound::Build] Configuration failed!"
    call :messageBox "Failed to configure xStarbound! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
    exit /b %buildError%
)

if "%buildInstaller%"=="yes" (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --build --preset "windows-x64-release" --target INSTALL
) else (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --build --preset "windows-x64-release"
)
if %ERRORLEVEL% neq 0 (
    color 04
    set buildError=%ERRORLEVEL%
    echo "[xStarbound::Build] Build failed!"
    call :messageBox "Failed to build xStarbound! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
    exit /b %buildError%
)

goto skipOver
:selectDirectory
echo "[xStarbound::Build] Waiting for directory selection."
:: Borrowed from this batch script on TenForums.com: https://www.tenforums.com/general-support/179377-bat-script-select-folder-update-path-bat-file.html
call :@ "Either select your Starbound install directory and click OK to install, or click Cancel to skip installation." SourceFolder
:@
set "@="(new-object -COM 'Shell.Application').BrowseForFolder(0,'%1',0x200,0).self.path""
for /f "usebackq delims=" %%# in (`PowerShell %@%`) do set "sbInstall=%%#"
:: ----------
:skipOver

if "%buildInstaller%"=="yes" (
    echo "[xStarbound::Build] Building installer..."
    mkdir dist-windows\installer
    "%PROGRAMFILES(X86)%\Inno Setup 6\ISCC.exe" /Odist-windows\installer cmake-build-windows-x64\inno-installer\xsb-installer.iss
    if %ERRORLEVEL% neq 0 (
        color 04
        set buildError=%ERRORLEVEL%
        echo "[xStarbound::Build] Installer build failed!"
        call :messageBox "Failed to build the installer! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
        exit /b %buildError%
    )
    call :messageBox "The xStarbound installer has been built successfully. Click OK to open the installer folder in Explorer." "xStarbound Build Script"
    exit /b
)
if "%sbInstall%"=="" (
    echo "[xStarbound::Build] Build complete!"
    call :messageBox "xStarbound has been built successfully. Click OK to open the asset and binary directories in Explorer. Everything is already set up for testing. Make sure to place a copy of Starbound's packed.pak in the opened assets\ folder." "xStarbound Build Script"
    explorer assets\
    explorer cmake-build-windows-x64\source\client\Release\
    explorer cmake-build-windows-x64\source\server\Release\
    explorer cmake-build-windows-x64\source\utility\Release\
    exit /b
)
if exist "%sbInstall%\assets\packed.pak" (
    echo "[xStarbound::Build] Installing xClient into chosen Starbound directory."
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --install cmake-build-windows-x64\ --config Release --prefix "%sbInstall%"
    if %ERRORLEVEL% neq 0 (
        color 04
        set buildError=%ERRORLEVEL%
        echo "[xStarbound::Build] Installation failed!"
        call :messageBox "Failed to install xStarbound! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
        exit /b %buildError%
    )
) else (
    echo "[xStarbound::Build] Not a valid Starbound directory!"
    call :messageBox "The selected folder does not contain a Starbound installation. Click OK to go back to folder selection." "xStarbound Build Script - Error"
    goto selectDirectory
)

echo "[xStarbound::Build] Installation complete!"
call :messageBox "Successfully installed xStarbound. Click OK to open the folder containing the newly installed xStarbound binaries." "xStarbound Build Script"
explorer "%sbInstall%\xsb-win64"
exit /b

:: For a yes/no dialogue.
:yesNoBox
REM returns 6 = Yes, 7 = No. Type=4 = Yes/No
set YesNo=
set MsgType=4
set heading=%~2
set message=%~1
echo wscript.echo msgbox(WScript.Arguments(0),%MsgType%,WScript.Arguments(1)) >"%temp%\input.vbs"
for /f "tokens=* delims=" %%a in ('cscript //nologo "%temp%\input.vbs" "%message%" "%heading%"') do set YesNo=%%a
exit /b

:: For a message dialogue.
:messageBox
set heading=%~2
set message=%~1
echo msgbox WScript.Arguments(0),0,WScript.Arguments(1) >"%temp%\input.vbs"
cscript //nologo "%temp%\input.vbs" "%message%" "%heading%"
exit /b