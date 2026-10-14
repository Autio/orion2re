param(
    [Parameter(Mandatory = $true)] [string] $GameDataDirectory,
    [string] $RuntimeDirectory,
    [string] $GameExecutable,
    [int] $Port = 8766,
    [switch] $PrepareOnly,
    [switch] $WatchBattles
)

$ErrorActionPreference = 'Stop'
$repository = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (!$RuntimeDirectory) {
    $RuntimeDirectory = Join-Path (Split-Path $repository -Parent) 'spectator-runtime'
}
if (!$GameExecutable) {
    $GameExecutable = Join-Path $repository 'out/build/stream/Debug/orion2re.exe'
}
$source = (Resolve-Path -LiteralPath $GameDataDirectory).Path
$executable = (Resolve-Path -LiteralPath $GameExecutable).Path
$baseArchive = Join-Path $repository 'out/build/stream/data/base.zip'
foreach ($name in @('FONTS.LBX', 'DIPLOMAT.LBX', 'MAINMENU.LBX')) {
    if (!(Test-Path -LiteralPath (Join-Path $source $name))) {
        throw "Missing $name in $source. Select the directory containing the original LBX files."
    }
}
if (!(Test-Path -LiteralPath $baseArchive)) {
    throw 'Build the game first; out/build/stream/data/base.zip is missing.'
}
if ($Port -lt 1024 -or $Port -gt 65535) { throw 'Choose a port between 1024 and 65535.' }
$runtime = [System.IO.Path]::GetFullPath($RuntimeDirectory)
if ($runtime.TrimEnd('\') -eq $source.TrimEnd('\')) {
    throw 'Use a separate runtime directory to preserve the original installation.'
}
$destinationExe = Join-Path $runtime 'orion2re.exe'
$runningGame = Get-Process orion2re -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $destinationExe }
if ($runningGame) { throw 'Close the existing spectator game before preparing this runtime again.' }
New-Item -ItemType Directory -Force -Path $runtime, (Join-Path $runtime 'data'), (Join-Path $runtime 'stream') | Out-Null
Get-ChildItem -LiteralPath $source -File | Where-Object { $_.Extension -in '.LBX', '.SMK' } | ForEach-Object {
    $destination = Join-Path $runtime $_.Name
    $existing = Get-Item -LiteralPath $destination -ErrorAction SilentlyContinue
    if (!$existing -or $existing.Length -ne $_.Length -or $existing.LastWriteTimeUtc -ne $_.LastWriteTimeUtc) {
        Copy-Item -LiteralPath $_.FullName -Destination $destination
    }
}
Copy-Item -LiteralPath $executable -Destination $destinationExe
Copy-Item -LiteralPath $baseArchive -Destination (Join-Path $runtime 'data/base.zip')
$configPath = Join-Path $runtime 'data/orion2re.cfg'
if (!(Test-Path -LiteralPath $configPath)) {
    Copy-Item -LiteralPath (Join-Path $repository 'out/build/stream/data/orion2re.cfg') -Destination $configPath
}
[System.IO.File]::WriteAllText((Join-Path $runtime 'ORIONCD.INI'), '.\', [System.Text.Encoding]::ASCII)
if ($PrepareOnly) {
    Write-Output "Prepared spectator runtime: $runtime"
    return
}

$python = (Get-Command python -ErrorAction Stop).Source
$journal = Join-Path $runtime 'stream/journal.jsonl'
$control = Join-Path $runtime 'stream/control.txt'
$serverArguments = @('-u', ('"' + (Join-Path $PSScriptRoot 'server.py') + '"'),
    '--journal', ('"' + $journal + '"'), '--control', ('"' + $control + '"'), '--port', "$Port")
$server = Start-Process -FilePath $python -ArgumentList $serverArguments -WorkingDirectory $runtime -WindowStyle Hidden `
    -RedirectStandardOutput (Join-Path $runtime 'stream/server.out.log') `
    -RedirectStandardError (Join-Path $runtime 'stream/server.err.log') -PassThru
Start-Sleep -Milliseconds 750
if ($server.HasExited) {
    throw "Stream server failed to start. Check $runtime\stream\server.err.log (the port may already be in use)."
}
$oldJournal = $env:ORION2RE_JOURNAL
$oldControl = $env:ORION2RE_STREAM_CONTROL
$oldWatchBattles = $env:ORION2RE_WATCH_BATTLES
try {
    $env:ORION2RE_JOURNAL = $journal
    $env:ORION2RE_STREAM_CONTROL = $control
    $env:ORION2RE_WATCH_BATTLES = if ($WatchBattles) { '1' } else { '0' }
    $game = Start-Process -FilePath $destinationExe -WorkingDirectory $runtime -PassThru
} catch {
    Stop-Process -Id $server.Id -ErrorAction SilentlyContinue
    throw
} finally {
    $env:ORION2RE_JOURNAL = $oldJournal
    $env:ORION2RE_STREAM_CONTROL = $oldControl
    $env:ORION2RE_WATCH_BATTLES = $oldWatchBattles
}
Write-Output "Game PID: $($game.Id); stream server PID: $($server.Id)"
Write-Output "Dashboard: http://127.0.0.1:$Port/"
Write-Output "OBS source: http://127.0.0.1:$Port/?overlay=1&empire=0"
Write-Output 'Start a single-player game, activate ALLAI on the galaxy screen, then use Run simulation in the dashboard.'
