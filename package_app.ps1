# ============================================================================
# OwnCAD - Build and Package Script (PowerShell)
# ============================================================================

Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host "OwnCAD - Building and Packaging for Distribution" -ForegroundColor Cyan
Write-Host "============================================================================" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "D:\OwnCAD"
$BuildDir = "$ProjectRoot\build"
$PackageDir = "$ProjectRoot\OwnCAD_Package"
$QtBinDir = "C:\Qt\6.10.1\mingw_64\bin"

# Step 1: Clean and build the project
Write-Host "[1/5] Building OwnCAD in Release mode..." -ForegroundColor Yellow

# Clean build directory to avoid generator conflicts
Write-Host "  Cleaning build directory..." -ForegroundColor Gray
if (Test-Path "$BuildDir\CMakeCache.txt") {
    Remove-Item "$BuildDir\CMakeCache.txt" -Force
}
if (Test-Path "$BuildDir\CMakeFiles") {
    Remove-Item "$BuildDir\CMakeFiles" -Recurse -Force
}

# Configure CMake with MinGW (to match Qt installation)
Write-Host "  Configuring with CMake..." -ForegroundColor Gray
$QtPath = "C:\Qt\6.10.1\mingw_64"
$MinGWPath = "C:\Qt\Tools\mingw1310_64\bin"

# Add MinGW to PATH for this session
$env:Path = "$MinGWPath;$env:Path"

cmake -B "$BuildDir" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QtPath" -G "MinGW Makefiles"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: CMake configuration failed!" -ForegroundColor Red
    Write-Host "Tip: Make sure MinGW is installed at: $MinGWPath" -ForegroundColor Yellow
    exit 1
}

# Build with MinGW
Write-Host "  Building..." -ForegroundColor Gray
cmake --build "$BuildDir" --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Error: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host ""

# Step 2: Find the executable
Write-Host "[2/5] Locating executable..." -ForegroundColor Yellow
$ExePath = "$BuildDir\OwnCAD.exe"

if (-not (Test-Path $ExePath)) {
    # Try Release subdirectory
    $ExePath = "$BuildDir\Release\OwnCAD.exe"
}

if (-not (Test-Path $ExePath)) {
    Write-Host "Error: OwnCAD.exe not found!" -ForegroundColor Red
    Write-Host "Searched in:" -ForegroundColor Red
    Write-Host "  - $BuildDir\OwnCAD.exe" -ForegroundColor Red
    Write-Host "  - $BuildDir\Release\OwnCAD.exe" -ForegroundColor Red
    exit 1
}

Write-Host "Found: $ExePath" -ForegroundColor Green
Write-Host ""

# Step 3: Create package directory
Write-Host "[3/5] Creating package directory..." -ForegroundColor Yellow
if (Test-Path $PackageDir) {
    Remove-Item -Recurse -Force $PackageDir
}
New-Item -ItemType Directory -Path $PackageDir | Out-Null

# Copy executable
Copy-Item $ExePath $PackageDir
Write-Host "Executable copied" -ForegroundColor Green
Write-Host ""

# Step 4: Deploy Qt dependencies
Write-Host "[4/5] Deploying Qt dependencies..." -ForegroundColor Yellow
$WinDeployQt = "$QtBinDir\windeployqt.exe"

if (-not (Test-Path $WinDeployQt)) {
    Write-Host "Error: windeployqt not found at $WinDeployQt" -ForegroundColor Red
    exit 1
}

& $WinDeployQt --release --no-translations "$PackageDir\OwnCAD.exe"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Warning: windeployqt encountered issues" -ForegroundColor Yellow
}
Write-Host "Qt dependencies deployed" -ForegroundColor Green
Write-Host ""

# Step 5: Copy MinGW runtime DLLs
Write-Host "[5/5] Copying MinGW runtime DLLs..." -ForegroundColor Yellow
$MinGWFiles = @("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll")
$CopiedCount = 0

foreach ($file in $MinGWFiles) {
    if (Test-Path "$QtBinDir\$file") {
        Copy-Item "$QtBinDir\$file" $PackageDir -Force
        $CopiedCount++
    } else {
        Write-Host "Warning: $file not found in $QtBinDir" -ForegroundColor Yellow
    }
}

Write-Host "Copied $CopiedCount MinGW runtime DLLs" -ForegroundColor Green
Write-Host ""

# Success!
Write-Host "============================================================================" -ForegroundColor Green
Write-Host "SUCCESS! Package created at:" -ForegroundColor Green
Write-Host "  $PackageDir" -ForegroundColor White
Write-Host ""
Write-Host "To share with your friend:" -ForegroundColor Cyan
Write-Host "  1. Right-click on 'OwnCAD_Package' folder" -ForegroundColor White
Write-Host "  2. Send to > Compressed (zipped) folder" -ForegroundColor White
Write-Host "  3. Send the ZIP file" -ForegroundColor White
Write-Host ""
Write-Host "Your friend can:" -ForegroundColor Cyan
Write-Host "  1. Extract the ZIP" -ForegroundColor White
Write-Host "  2. Run OwnCAD.exe (no installation needed!)" -ForegroundColor White
Write-Host "============================================================================" -ForegroundColor Green
Write-Host ""

# Calculate package size
$PackageSize = (Get-ChildItem -Path $PackageDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "Package size: $([math]::Round($PackageSize, 2)) MB" -ForegroundColor Cyan
Write-Host ""
