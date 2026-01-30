@echo off
REM ============================================================================
REM OwnCAD - Create Standalone Package
REM ============================================================================

echo Building OwnCAD Release...

REM Clean previous build
if exist build\Release rmdir /s /q build\Release

REM Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

if not exist build\Release\OwnCAD.exe (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.

REM Create package directory
set PACKAGE_DIR=OwnCAD_Package
if exist %PACKAGE_DIR% rmdir /s /q %PACKAGE_DIR%
mkdir %PACKAGE_DIR%

echo Creating standalone package...

REM Copy executable
copy build\Release\OwnCAD.exe %PACKAGE_DIR%\

REM Deploy Qt dependencies using windeployqt
REM This copies all required Qt DLLs and plugins
C:\Qt\6.10.1\mingw_64\bin\windeployqt.exe --release --no-translations %PACKAGE_DIR%\OwnCAD.exe

REM Copy Visual C++ Runtime (if needed)
REM Uncomment if you get missing VCRUNTIME DLL errors:
REM copy "C:\Windows\System32\msvcp140.dll" %PACKAGE_DIR%\
REM copy "C:\Windows\System32\vcruntime140.dll" %PACKAGE_DIR%\
REM copy "C:\Windows\System32\vcruntime140_1.dll" %PACKAGE_DIR%\

echo.
echo ============================================================================
echo Package created successfully!
echo Location: %PACKAGE_DIR%
echo.
echo You can now compress this folder and send to your friend:
echo - Right-click on "%PACKAGE_DIR%" folder
echo - Send to > Compressed (zipped) folder
echo.
echo Your friend just needs to:
echo 1. Extract the ZIP file
echo 2. Run OwnCAD.exe
echo ============================================================================
echo.

pause
