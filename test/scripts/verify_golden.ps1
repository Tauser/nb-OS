param(
  [Parameter(Mandatory=$true)][ValidateSet('idle','touch','voice','stress')] [string]$Scenario,
  [Parameter(Mandatory=$true)] [string]$LogFile
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $LogFile)) {
  throw "Log file not found: $LogFile"
}

$base = Join-Path $PSScriptRoot "..\golden\$Scenario.baseline.txt"
if (-not (Test-Path $base)) {
  throw "Baseline not found: $base"
}

$logLines = Get-Content $LogFile
$expected = Get-Content $base | Where-Object { $_.Trim() -ne '' -and -not $_.Trim().StartsWith('#') }

$cursor = 0
$missing = @()

foreach ($token in $expected) {
  $found = $false
  for ($i = $cursor; $i -lt $logLines.Count; $i++) {
    if ($logLines[$i] -like "*$token*") {
      $found = $true
      $cursor = $i + 1
      break
    }
  }

  if (-not $found) {
    $missing += $token
  }
}

if ($missing.Count -gt 0) {
  Write-Host "[REGRESSION] FAIL scenario=$Scenario" -ForegroundColor Red
  $missing | ForEach-Object { Write-Host "  missing: $_" -ForegroundColor Red }
  exit 2
}

Write-Host "[REGRESSION] PASS scenario=$Scenario" -ForegroundColor Green
exit 0
