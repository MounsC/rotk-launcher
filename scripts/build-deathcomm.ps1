param([string]$DotnetPath = "dotnet")
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root "native\RotkDeathcomm\RotkDeathcomm.csproj"
$output = Join-Path $root "resources\deathcomm"
& $DotnetPath publish $project -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:EnableCompressionInSingleFile=true -o $output
if ($LASTEXITCODE -ne 0) { throw "Deathcomm helper build failed." }
Write-Host "Built deathcomm helper: $output\RotkDeathcomm.exe"
