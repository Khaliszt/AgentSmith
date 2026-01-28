# C:\FarfadetsCorp\AgentSmith\scripts\download_xterm.ps1

$webDir = "$PSScriptRoot\..\resources\web"
New-Item -ItemType Directory -Force -Path $webDir | Out-Null

$files = @{
    "https://cdn.jsdelivr.net/npm/xterm@5.3.0/lib/xterm.min.js" = "xterm.min.js"
    "https://cdn.jsdelivr.net/npm/xterm@5.3.0/css/xterm.css" = "xterm.css"
    "https://cdn.jsdelivr.net/npm/@xterm/addon-fit@0.10.0/lib/addon-fit.min.js" = "xterm-addon-fit.min.js"
    "https://cdn.jsdelivr.net/npm/@xterm/addon-web-links@0.11.0/lib/addon-web-links.min.js" = "xterm-addon-web-links.min.js"
}

foreach ($url in $files.Keys) {
    $dest = Join-Path $webDir $files[$url]
    Write-Host "Downloading $($files[$url])..."
    Invoke-WebRequest -Uri $url -OutFile $dest
}

Write-Host "xterm.js resources downloaded to $webDir"
