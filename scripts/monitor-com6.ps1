$repoRoot = Split-Path -Parent $PSScriptRoot
$path = Join-Path $repoRoot 'logs\cam-serial-log.txt'
$logDir = Split-Path -Parent $path
if (-not (Test-Path -LiteralPath $logDir)) {
  New-Item -ItemType Directory -Path $logDir | Out-Null
}
Remove-Item $path -ErrorAction SilentlyContinue
Add-Content -Path $path -Value ((Get-Date -Format 'HH:mm:ss') + ' monitor start')
try {
  $port = New-Object System.IO.Ports.SerialPort 'COM6',115200,'None',8,'one'
  $port.ReadTimeout = 250
  $port.DtrEnable = $false
  $port.RtsEnable = $false
  $port.Open()
  Add-Content -Path $path -Value ((Get-Date -Format 'HH:mm:ss') + ' COM6 open ok')
  $start = Get-Date
  while ((Get-Date) -lt $start.AddMinutes(2)) {
    try {
      $line = $port.ReadLine()
      if ($line) {
        Add-Content -Path $path -Value ((Get-Date -Format 'HH:mm:ss') + ' ' + $line.TrimEnd())
      }
    } catch {}
  }
  $port.Close()
  Add-Content -Path $path -Value ((Get-Date -Format 'HH:mm:ss') + ' COM6 closed')
} catch {
  Add-Content -Path $path -Value ((Get-Date -Format 'HH:mm:ss') + ' ERROR ' + $_.Exception.Message)
}
