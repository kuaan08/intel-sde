param(
    [string]$Configuration = "Release",
    [string]$Output = "dist\\sde-avx-lab-windows-x64.zip"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $root "dist\\sde-avx-lab-windows-x64"
Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item (Join-Path $root "avx-runtime\\build-ci\\$Configuration\\avx_runtime.exe") $stage
Copy-Item (Join-Path $root "avx1-sse-poc\\build-ci\\$Configuration\\avx1_sse_poc.exe") $stage
Copy-Item (Join-Path $root "sde-avx-lab-app\\bin\\SDE AVX Lab.exe") $stage
Copy-Item (Join-Path $root "README.md") $stage
Copy-Item (Join-Path $root "LICENSE") $stage
Copy-Item (Join-Path $root "THIRD-PARTY-NOTICES.md") $stage
$manifest = Join-Path $stage "SHA256SUMS.txt"
Get-ChildItem $stage -File | Where-Object { $_.Name -ne "SHA256SUMS.txt" } | ForEach-Object {
    "{0}  {1}" -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $_.Name
} | Set-Content $manifest
$outputPath = if ([IO.Path]::IsPathRooted($Output)) { $Output } else { Join-Path $root $Output }
New-Item -ItemType Directory -Force (Split-Path $outputPath) | Out-Null
Remove-Item $outputPath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $outputPath
Write-Host "Created $outputPath"
