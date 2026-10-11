param(
    [switch]$Verify,
    [switch]$ConfigureOnly,
    [ValidateRange(1,64)][int]$Jobs = 4,
    [string]$Python = "python"
)
$ErrorActionPreference = 'Stop'
$buildArguments = @((Join-Path $PSScriptRoot 'tools\build.py'), '--jobs', "$Jobs")
if ($Verify) { $buildArguments += '--verify' }
if ($ConfigureOnly) { $buildArguments += '--configure-only' }
& $Python @buildArguments
if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
