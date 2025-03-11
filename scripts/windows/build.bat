@echo off
setLocal enableDelayedExpansion

echo "[xStarbound::Build] Starting build..."

cd /D "%~dp0"
cd ..\..

if exist "%PROGRAMFILES(X86)%\Inno Setup 6" (
    call :yesNoBox "Do you want to build the xStarbound installer and ZIP archive? Click 'No' if you want to use this script for direct installation." "xStarbound Build Script"
    if "!YesNo!"=="6" (
        echo "[xStarbound::Build] Will build installer."
        set "buildInstaller=yes"
    ) else (
        echo "[xStarbound::Build] Will not build installer."
        set "buildInstaller=no"
    )
) else (
    set "buildInstaller=no"
    call :yesNoBox "If you want to build the xStarbound installer, click 'No' to open the Inno Setup download page in your browser. Restart this script once you've installed Inno Setup. Otherwise, click 'Yes' to continue. You may still use this script for direct installation." "xStarbound Build Script"
    if "!YesNo!"=="6" echo "[xStarbound::Build] Will not build installer."
    else (
        echo "[xStarbound::Build] Opening Inno Setup 6 website..."
        rundll32 url.dll,FileProtocolHandler "https://jrsoftware.org/isdl.php"
        exit /b
    )
)

call :yesNoBox "Do you want to build a debug build?" "xStarbound Build Script"
if "!YesNo!"=="6" (
    echo "[xStarbound::Build] Will build a debug build."
    set "relOrDbg=Debug"
) else (
    echo "[xStarbound::Build] Will build a release build."
    set "relOrDbg=Release"
)

if "%buildInstaller%"=="yes" (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --preset "windows-x64" -DSTAR_INSTALL_VCPKG=ON -DPACKAGE_XSB_ASSETS=ON -DBUILD_INNOSETUP=ON -UCMAKE_TOOLCHAIN_FILE
) else (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --preset "windows-x64" -DSTAR_INSTALL_VCPKG=ON -DPACKAGE_XSB_ASSETS=ON -DBUILD_INNOSETUP=OFF -UCMAKE_TOOLCHAIN_FILE
)
if !ERRORLEVEL! neq 0 (
    color 04
    set buildError=!ERRORLEVEL!
    echo "[xStarbound::Build] Configuration failed^!"
    call :messageBox "Failed to configure xStarbound^! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
    exit /b %buildError%
)

if "!relOrDbg!"=="Debug" (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --build --preset "windows-x64-debug"
) else (
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --build --preset "windows-x64-release"
)
if !ERRORLEVEL! neq 0 (
    color 04
    set buildError=!ERRORLEVEL!
    echo "[xStarbound::Build] Build failed^!"
    call :messageBox "Failed to build xStarbound! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
    exit /b %buildError%
)

if "%buildInstaller%"=="yes" goto skipOver
:selectDirectory
echo "[xStarbound::Build] Waiting for directory selection."
:: Borrowed from this batch script on TenForums.com: https://www.tenforums.com/general-support/179377-bat-script-select-folder-update-path-bat-file.html
call :@ "Either select your Starbound install directory and click OK to install, or click Cancel to skip installation." SourceFolder
:@
set "@="(new-object -COM 'Shell.Application').BrowseForFolder(0,'%1',0x200,0).self.path""
for /f "usebackq delims=" %%# in (`PowerShell %@%`) do set "sbInstall=%%#"
goto directorySelected
:: ----------
:skipOver

if "%buildInstaller%"=="yes" (
    echo "[xStarbound::Build] Building installer and ZIP archive..."
    mkdir dist-windows
    mkdir dist-windows\install-tree
    :: Don't forget to create `steam_appid.txt`!
    mkdir dist-windows\install-tree\xsb-win64
    <nul set /p=211820 > dist-windows\install-tree\xsb-win64\steam_appid.txt
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --install cmake-build-windows-x64\ --config "!relOrDbg!" --prefix dist-windows\install-tree\
    if !ERRORLEVEL! neq 0 (
        color 04
        set buildError=!ERRORLEVEL!
        echo "[xStarbound::Build] Installer setup failed^!"
        call :messageBox "Failed to set up the installation tree^! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
        exit /b %buildError%
    )
    :: Because the "DLL hell" fix stops CMake from copying *any* DLLs on Windows builds, the DLLs have to be manually copied in.
    xcopy /y /f "cmake-build-windows-x64\source\client\!relOrDbg!\*.dll" dist-windows\install-tree\xsb-win64\
    xcopy /y /f "cmake-build-windows-x64\source\server\!relOrDbg!\*.dll" dist-windows\install-tree\xsb-win64\
    xcopy /y /f "cmake-build-windows-x64\source\utility\!relOrDbg!\*.dll" dist-windows\install-tree\xsb-win64\
    :: Comment out the following line if using any Windows version older than Windows 10 1803.
    cd dist-windows\install-tree & tar -cavf ..\installer\windows.zip * & cd ..\..
    "%PROGRAMFILES(X86)%\Inno Setup 6\ISCC.exe" "/DXSBSourcePath=..\..\dist-windows\install-tree\" "/DXSBDistPath=..\..\dist-windows\install-tree\" /Odist-windows\installer cmake-build-windows-x64\inno-installer\xsb-installer.iss
    if !ERRORLEVEL! neq 0 (
        color 04
        set buildError=!ERRORLEVEL!
        echo "[xStarbound::Build] Installer build failed^!"
        call :messageBox "Failed to build the installer^! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
        exit /b %buildError%
    )
    call :messageBox "The xStarbound installer has been built successfully. Click OK to open the installer folder in Explorer." "xStarbound Build Script"
    explorer dist-windows\installer\
    exit /b
)
:directorySelected
if "%sbInstall%"=="" (
    echo "[xStarbound::Build] Build complete^!"
    call :messageBox "xStarbound has been built successfully. Click OK to open the asset and binary directories in Explorer. Everything is already set up for testing. Make sure to place a copy of Starbound's packed.pak in the opened assets\ folder." "xStarbound Build Script"
    explorer assets\
    explorer "cmake-build-windows-x64\source\client\!relOrDbg!\"
    explorer "cmake-build-windows-x64\source\server\!relOrDbg!\"
    explorer "cmake-build-windows-x64\source\utility\!relOrDbg!\"
    exit /b
)
if exist "%sbInstall%\assets\packed.pak" (
    echo "[xStarbound::Build] Installing xClient into chosen Starbound directory."
    "%PROGRAMFILES%\CMake\bin\cmake.exe" --install cmake-build-windows-x64\ --config "!relOrDbg!" --prefix "%sbInstall%"
    if !ERRORLEVEL! neq 0 (
        color 04
        set buildError=!ERRORLEVEL!
        echo "[xStarbound::Build] Installation failed^!"
        call :messageBox "Failed to install xStarbound^! Check the console window for details. Click OK to exit this script." "xStarbound Build Script - Error"
        exit /b %buildError%
    )
    :: Because the "DLL hell" fix stops CMake from copying *any* DLLs on Windows builds, the DLLs have to be manually copied in.
    mkdir "!sbInstall!\xsb-win64"
    :: Don't forget to create `steam_appid.txt`!
    <nul set /p=211820 > "!sbInstall!\xsb-win64\steam_appid.txt"
    xcopy /y /f "cmake-build-windows-x64\source\client\!relOrDbg!\*.dll" "!sbInstall!\xsb-win64\"
    xcopy /y /f "cmake-build-windows-x64\source\server\!relOrDbg!\*.dll" "!sbInstall!\xsb-win64\"
    xcopy /y /f "cmake-build-windows-x64\source\utility\!relOrDbg!\*.dll" "!sbInstall!\xsb-win64\"
) else (
    echo "[xStarbound::Build] Not a valid Starbound directory^!"
    call :messageBox "The selected folder does not contain a Starbound installation. Click OK to go back to folder selection." "xStarbound Build Script - Error"
    goto selectDirectory
)

echo "[xStarbound::Build] Installation complete^!"
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