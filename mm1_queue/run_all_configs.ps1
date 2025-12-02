# PowerShell script to run all M/M/1 queue configurations
# Usage: .\run_all_configs.ps1

Write-Host "Building M/M/1 Queue Simulation..." -ForegroundColor Green

# Build the simulation
if (Test-Path "Makefile") {
    Remove-Item "Makefile"
}

# Generate makefile and build
opp_makemake -f --deep
make

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build successful! Starting simulations..." -ForegroundColor Green
Write-Host ""

# List of all configurations
$configs = @("Default", "Balanced", "LightLoad", "HeavyLoad", "FastService", "ParameterStudy")

Write-Host "Running the following configurations:" -ForegroundColor Yellow
foreach ($config in $configs) {
    Write-Host "  - $config"
}
Write-Host ""

# Run all configurations
$configString = $configs -join ","
Write-Host "Executing: .\queue_sim -u Cmdenv -c $configString" -ForegroundColor Cyan

.\queue_sim -u Cmdenv -c $configString

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "All simulations completed successfully!" -ForegroundColor Green
    Write-Host "Results saved in results/ directory" -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "Simulation failed with exit code: $LASTEXITCODE" -ForegroundColor Red
}

Write-Host ""
Write-Host "Analysis commands:" -ForegroundColor Cyan
Write-Host "  opp_scavetool query results/*.sca"
Write-Host "  opp_scavetool export -f CSV -o results.csv results/*.sca"