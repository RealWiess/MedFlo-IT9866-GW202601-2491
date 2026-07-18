$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

$sourceRom = ".\build\openrtos\GW202601\project\GW202601\ITE_NOR.ROM"
$destRom = ".\ITE_NOR.ROM"
$destRomTimestamp = ".\GW202601_$timestamp.ROM"

if (Test-Path $sourceRom) {
    Copy-Item -Path $sourceRom -Destination $destRom -Force
    Copy-Item -Path $sourceRom -Destination $destRomTimestamp -Force
    
    Write-Host "==========================================================" -ForegroundColor Green
    Write-Host "  ROM File Copied Successfully!" -ForegroundColor Green
    Write-Host "  Latest ROM: $destRom" -ForegroundColor Green
    Write-Host "  Backup ROM: $destRomTimestamp" -ForegroundColor Green
    Write-Host "==========================================================" -ForegroundColor Green
} else {
    Write-Host "ERROR: Could not find build output ROM file ($sourceRom)!" -ForegroundColor Red
}
