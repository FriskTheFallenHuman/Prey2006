<#
Package PK4 helper script (zip-only, os-aware)

Usage:
  pwsh ./scripts/package_pk4.ps1 -DllName gamex86_64.so -OsName linux
  pwsh ./scripts/package_pk4.ps1 -DllName gamex86.dll -OsName windows -RemoveBinaryConf

Behavior:
  - Uses output/<osname>/ and output/<osname>/base as canonical locations.
  - Copies repo/base and repo/libs into the output folder excluding the DLL and binary.conf.
  - Creates binary.conf in output/<osname>/base if missing.
  - Requires zip to be available; will exit with error if zip is missing.
  - Creates game01.pk4 in output/<osname>/base and removes binary.conf after success.
#>

param(
  [Parameter(Mandatory=$true)][string]$DllName,
  [Parameter(Mandatory=$true)][ValidateSet('linux','windows','macosx','freebsd')][string]$OsName,
  [string]$Repo = $env:GITHUB_WORKSPACE,
  [string]$PkgName = "game01.pk4",
  [switch]$RemoveBinaryConf
)

if (-not $Repo) {
  Write-Error "Repository root not provided and GITHUB_WORKSPACE is empty."
  exit 1
}

$OutRoot = Join-Path $Repo ("output" + [IO.Path]::DirectorySeparatorChar + $OsName)
$OutBase = Join-Path $OutRoot "base"
$OutLibs = Join-Path $OutRoot "libs"
$SrcBase = Join-Path $Repo "base"
$SrcLibs = Join-Path $Repo "libs"
$PkgPath = Join-Path $OutBase $PkgName
$Dll = $DllName

Write-Host "Repo: $Repo"
Write-Host "OS name: $OsName"
Write-Host "Output root: $OutRoot"
Write-Host "Output base: $OutBase"
Write-Host "Package: $PkgPath"
Write-Host "DLL: $Dll"

# Prepare directories
New-Item -ItemType Directory -Path $OutBase -Force | Out-Null
New-Item -ItemType Directory -Path $OutLibs -Force | Out-Null

# Validate source base
if (-not (Test-Path $SrcBase)) {
  Write-Error "Source base directory not found: $SrcBase"
  exit 1
}

# Copy base contents excluding DLL and binary.conf
Write-Host "Copying base contents to $OutBase excluding $Dll and binary.conf"
Copy-Item -Path (Join-Path $SrcBase '*') -Destination $OutBase -Recurse -Force -Exclude $Dll, 'binary.conf'

# Copy libs if present
if (Test-Path $SrcLibs) {
  Write-Host "Copying libs contents to $OutLibs"
  Copy-Item -Path (Join-Path $SrcLibs '*') -Destination $OutLibs -Recurse -Force
} else {
  Write-Host "No libs directory at $SrcLibs, skipping libs copy"
}

# Copy docs if present
$docs = @(".github/Changelog.md", ".github/Configuration.md", ".github/README.md")
foreach ($d in $docs) {
  $src = Join-Path $Repo $d
  if (Test-Path $src) {
    Copy-Item -Path $src -Destination $OutRoot -Force
  }
}

# Create binary.conf in packaging directory if missing
$osIdMap = @{
  'windows' = 0
  'macosx'  = 1
  'linux'   = 2
  'freebsd' = 2
}

$binaryConfPath = Join-Path $OutBase 'binary.conf'
if (-not (Test-Path $binaryConfPath)) {
  $id = $osIdMap[$OsName]
  if ($null -eq $id) {
    Write-Warning "Unknown OsName '$OsName'; defaulting binary.conf id to 2 (linux/freebsd)"
    $id = 2
  }

  $binaryConfContent = @"
// add flags for supported operating systems in this pak
// one id per line
// name the file binary.conf and place it in the game pak
// 0 windows
// 1 macosx
// 2 linux/freebsd
$id
"@

  Write-Host "Creating binary.conf at $binaryConfPath with id $id"
  $binaryConfContent.Trim() | Out-File -FilePath $binaryConfPath -Encoding utf8 -Force
} else {
  Write-Host "binary.conf already exists at $binaryConfPath; leaving it in place"
}

$OutDllPath = Join-Path $OutBase $Dll
if (-not (Test-Path $OutDllPath)) {
  Write-Error "Expected DLL not found at canonical location: $OutDllPath"
  Write-Error "Place the built binary at output/$OsName/base/$Dll before running this script."
  exit 1
} else {
  Write-Host "DLL found at $OutDllPath"
}

if (-not (Get-Command zip -ErrorAction SilentlyContinue)) {
  Write-Error "zip is not available on this runner. Install zip or enable a fallback."
  exit 1
}

Push-Location $OutBase
try {
  if (Test-Path $PkgPath) {
    Remove-Item $PkgPath -Force
  }

  Write-Host "Using zip to create package $PkgName"
  & zip -r -q -FS (Split-Path -Leaf $PkgPath) binary.conf $Dll

  if (Test-Path $PkgPath) {
    Write-Host "Package created: $PkgPath"
    if (Test-Path "binary.conf") {
      Remove-Item -Path "binary.conf" -Force -ErrorAction SilentlyContinue
      Write-Host "Removed binary.conf from packaging directory"
    }
    if ($RemoveBinaryConf) {
      $srcBinary = Join-Path $SrcBase "binary.conf"
      if (Test-Path $srcBinary) {
        Remove-Item -Path $srcBinary -Force -ErrorAction SilentlyContinue
        Write-Host "Removed binary.conf from source base: $srcBinary"
      }
    }
  } else {
    Write-Error "Package was not created at $PkgPath"
    exit 1
  }
} finally {
  Pop-Location
}

Write-Host "Packaging step completed. Final contents of ${OutBase}:"
Get-ChildItem -Path $OutBase -Recurse | ForEach-Object { Write-Host $_.FullName }
