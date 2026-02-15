<#
Github Workflow script

Usage:
  pwsh ./scripts/package_pk4.ps1 -DllName gamex86_64.so -OsName linux
  pwsh ./scripts/package_pk4.ps1 -DllName gamex86.dll -OsName windows -RemoveBinaryConf
#>

param(
  [Parameter(Mandatory=$true)][string]$PkgName,
  [Parameter(Mandatory=$true)][string]$DllName,
  [Parameter(Mandatory=$true)][ValidateSet('linux','windows','macosx','freebsd')][string]$OsName,
  [string]$Repo = $env:GITHUB_WORKSPACE,
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

New-Item -ItemType Directory -Path $OutBase -Force | Out-Null
New-Item -ItemType Directory -Path $OutLibs -Force | Out-Null

Copy-Item -Path (Join-Path $SrcBase '*') -Destination $OutBase -Recurse -Force -Exclude $Dll, 'binary.conf'

if (Test-Path $SrcLibs) {
  Copy-Item -Path (Join-Path $SrcLibs '*') -Destination $OutLibs -Recurse -Force
}

$docs = @(".github/Changelog.md", ".github/Configuration.md", ".github/README.md")
foreach ($d in $docs) {
  $src = Join-Path $Repo $d
  if (Test-Path $src) {
    Copy-Item -Path $src -Destination $OutRoot -Force
  }
}

$osIdMap = @{
  'windows' = 0
  'macosx'  = 1
  'linux'   = 2
  'freebsd' = 2
}
$binaryConfPath = Join-Path $OutBase 'binary.conf'
if (-not (Test-Path $binaryConfPath)) {
  $id = $osIdMap[$OsName]
  $binaryConfContent = @"
// add flags for supported operating systems in this pak
// one id per line
// name the file binary.conf and place it in the game pak
// 0 windows
// 1 macosx
// 2 linux/freebsd
$id
"@
  $binaryConfContent.Trim() | Out-File -FilePath $binaryConfPath -Encoding utf8 -Force
}

$OutDllPath = Join-Path $OutBase $Dll
if (-not (Test-Path $OutDllPath)) {
  Write-Error "Expected DLL not found at $OutDllPath"
  exit 1
}

Push-Location $OutBase
try {
  if (Test-Path $PkgPath) { Remove-Item $PkgPath -Force }

  if ($OsName -eq 'windows') {
    Write-Host "Using Compress-Archive to create $PkgName"
    Compress-Archive -Path @("binary.conf", $Dll) -DestinationPath $PkgPath -Force
  } else {
    if (-not (Get-Command zip -ErrorAction SilentlyContinue)) {
      Write-Error "zip is not available on this runner."
      exit 1
    }
    Write-Host "Using zip to create $PkgName"
    & zip -r -q -FS (Split-Path -Leaf $PkgPath) binary.conf $Dll
  }

  if (Test-Path $PkgPath) {
    Write-Host "Package created: $PkgPath"
    Remove-Item -Path "binary.conf" -Force -ErrorAction SilentlyContinue
    Remove-Item -Path $Dll -Force -ErrorAction SilentlyContinue
    $extraArtifacts = @("*.ilk","*.pdb","*.lib","*.exp")
    foreach ($pattern in $extraArtifacts) {
      Get-ChildItem -Path $OutBase -Filter $pattern -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
    }

    if ($RemoveBinaryConf) {
      $srcBinary = Join-Path $SrcBase "binary.conf"
      if (Test-Path $srcBinary) {
        Remove-Item -Path $srcBinary -Force -ErrorAction SilentlyContinue
      }
    }
  } else {
    Write-Error "Package was not created at $PkgPath"
    exit 1
  }
} finally {
  Pop-Location
}

Get-ChildItem -Path $OutBase -Recurse | ForEach-Object { Write-Host $_.FullName }
