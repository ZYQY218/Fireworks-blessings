param(
    [string]$Name = ('BirthdayWish-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
)

$ErrorActionPreference = 'Stop'
if ($Name -notmatch '^[A-Za-z0-9][A-Za-z0-9_-]*$') {
    throw 'Name may contain only letters, digits, hyphens, and underscores.'
}
$projectRoot = Split-Path -Parent $PSScriptRoot
$outputRoot = Join-Path $projectRoot 'upload'
$destination = Join-Path $outputRoot $Name
$archive = Join-Path $outputRoot ($Name + '.zip')
if ((Test-Path -LiteralPath $destination) -or (Test-Path -LiteralPath $archive)) {
    throw 'Output already exists. Choose a new Name; existing files are never overwritten.'
}

# Explicit allowlist: never copy old fireworks, local backups, or compiler output.
$files = @(
    '.gitignore', '.gitattributes', '.editorconfig', 'README.md', 'raylib.local.props.example',
    'firework.cpp', 'wish.sln', 'wish.vcxproj', 'wish.vcxproj.filters',
    'docs/preview.png', 'scripts/Export-Upload.ps1',
    'picture/bg.png', 'picture/fireworks-generated-v1.png', 'picture/launch-generated-v1.png'
)
foreach ($folder in @('font', 'sound')) {
    foreach ($item in (Get-ChildItem -LiteralPath (Join-Path $projectRoot $folder) -File -Recurse)) {
        $files += $item.FullName.Substring($projectRoot.Length + 1)
    }
}
foreach ($relative in $files) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $relative) -PathType Leaf)) {
        throw "Missing file: $relative"
    }
}
New-Item -ItemType Directory -Path $destination | Out-Null
foreach ($relative in $files) {
    $target = Join-Path $destination $relative
    $parent = Split-Path -Parent $target
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $projectRoot $relative) -Destination $target
}
Compress-Archive -LiteralPath $destination -DestinationPath $archive -CompressionLevel Optimal
Write-Output "Upload source folder: $destination"
Write-Output "Upload source archive: $archive"
