param()
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$zig = Get-Command zig -CommandType Application -ErrorAction Stop
if (([string](& $zig.Source version)).Trim() -ne "0.15.2") { throw "Expected Zig 0.15.2" }
$outputDirectory = Join-Path $PSScriptRoot "dist\tests"
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$output = Join-Path $outputDirectory "voice_volume_compat_test.exe"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$sdk = Join-Path $root "resources\patches\vivoxsdk_x64_v5.dll"
$proxy = Join-Path $root "resources\patches\vivoxsdk_x64.dll"
$runtimeDirectory = Join-Path $outputDirectory "voice-volume-runtime"
New-Item -ItemType Directory -Path $runtimeDirectory -Force | Out-Null
Copy-Item -LiteralPath $sdk -Destination $runtimeDirectory -Force
Copy-Item -LiteralPath $proxy -Destination $runtimeDirectory -Force
$sdk = Join-Path $runtimeDirectory "vivoxsdk_x64_v5.dll"
$proxy = Join-Path $runtimeDirectory "vivoxsdk_x64.dll"
& $zig.Source cc -target x86_64-windows-gnu -O2 -Wall -Wextra -Werror -o $output (Join-Path $PSScriptRoot "tests\voice_volume_compat_test.c")
if ($LASTEXITCODE -ne 0) { throw "Voice volume test build failed" }
& $output $sdk $proxy
if ($LASTEXITCODE -ne 0) { throw "Voice volume compatibility test failed ($LASTEXITCODE)" }
