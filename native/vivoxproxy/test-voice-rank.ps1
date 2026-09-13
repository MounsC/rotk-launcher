param([string]$OutputPath = "")
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$zig = Get-Command zig -CommandType Application -ErrorAction Stop
if (([string](& $zig.Source version)).Trim() -ne "0.15.2") { throw "Expected Zig 0.15.2" }
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $PSScriptRoot "dist\tests\voice_rank_patch_test.exe"
}
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
& $zig.Source cc -target x86_64-windows-gnu -O2 -Wall -Wextra -Werror -o $output (Join-Path $PSScriptRoot "tests\voice_rank_patch_test.c")
if ($LASTEXITCODE -ne 0) { throw "Voice rank test compilation failed" }
& $output "$output.patched-code.bin"
if ($LASTEXITCODE -ne 0) { throw "Voice rank patch tests failed" }
