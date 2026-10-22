# Puts oxipng.exe and optipng.exe, at pinned versions, next to lightscreen.exe in a deploy folder, with their licence
# files under licenses\<tool>\ (docs/decisions/2026-10-21-optipng-and-oxipng-both-selectable.md).
# Each exe is taken from its upstream release archive and must match the SHA-256 recorded here; the hashes are those of
# the owner's own copies of the same versions, so a download that differs from them fails the build. To bump a tool,
# change its version, URLs and hash together.
param([Parameter(Mandatory = $true)][string]$Destination)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$tools = @(
    @{
        Name    = 'oxipng'
        Version = '10.2.1'
        Sha256  = '35ae3980ab831af64f4aecc98f69b81bfa146fa750a61d17ebbb12520128cbc8'
        Urls    = @(
            'https://github.com/oxipng/oxipng/releases/download/v10.2.1/oxipng-10.2.1-x86_64-pc-windows-msvc.zip'
        )
    },
    @{
        Name    = 'optipng'
        Version = '7.9.1'
        Sha256  = 'de908e5ba742c4132f7628087ca40fe8588a681c9992ae136e1ceee83dcd8139'
        Urls    = @(
            'https://downloads.sourceforge.net/project/optipng/OptiPNG/optipng-7.9.1/optipng-7.9.1-win64.zip',
            'https://sourceforge.net/projects/optipng/files/OptiPNG/optipng-7.9.1/optipng-7.9.1-win64.zip/download'
        )
    }
)

$work = Join-Path ([System.IO.Path]::GetTempPath()) ('optimizers-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $work | Out-Null

foreach ($tool in $tools) {
    $name = $tool.Name
    $found = $null

    foreach ($url in $tool.Urls) {
        $zip = Join-Path $work "${name}.zip"
        $dir = Join-Path $work $name
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $zip, $dir

        try {
            Invoke-WebRequest -Uri $url -OutFile $zip -UserAgent 'Wget' -MaximumRedirection 10
            Expand-Archive -Path $zip -DestinationPath $dir
        } catch {
            Write-Warning "${name} $($tool.Version): $url failed: $($_.Exception.Message)"
            continue
        }

        $exe = Get-ChildItem -Path $dir -Recurse -Filter "${name}.exe" | Select-Object -First 1

        if (-not $exe) {
            Write-Warning "${name}: no ${name}.exe in $url; the archive holds:"
            Get-ChildItem -Path $dir -Recurse | ForEach-Object { Write-Warning "  $($_.FullName.Substring($dir.Length))" }
            continue
        }

        $hash = (Get-FileHash -Algorithm SHA256 $exe.FullName).Hash.ToLowerInvariant()

        if ($hash -ne $tool.Sha256) {
            throw "${name} $($tool.Version) from $url has SHA-256 $hash, expected $($tool.Sha256)"
        }

        $found = $exe
        break
    }

    if (-not $found) {
        throw "${name} $($tool.Version): no download worked"
    }

    Copy-Item $found.FullName -Destination $Destination

    $licenses = Join-Path $Destination "licenses\${name}"
    New-Item -ItemType Directory -Force -Path $licenses | Out-Null
    $texts = Get-ChildItem -Path (Join-Path $work $name) -Recurse -File |
        Where-Object { $_.Name -match '^(LICEN[CS]E|COPYING)' }

    if (-not $texts) {
        throw "${name} $($tool.Version): no licence file in the release archive"
    }

    $texts | ForEach-Object { Copy-Item $_.FullName -Destination $licenses }
    Write-Host "${name} $($tool.Version): $($found.Name) verified ($($tool.Sha256)), licences: $(($texts | ForEach-Object Name) -join ', ')"
}

Remove-Item -Recurse -Force $work
