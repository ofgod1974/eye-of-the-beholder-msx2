param([ValidateSet('Auto','SCC','FMPAC','OPL4')][string]$Audio='Auto')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$rom=Join-Path $root 'build/beholder-intro.rom'
if(!(Test-Path -LiteralPath $rom)){& "$PSScriptRoot/build.ps1"}
$kit=$env:MSX_KIT
if(!$kit){$kit='C:\Users\Andrea\Documents\Codex\2026-10-27\cr\outputs\eagle-nest-msx'}
$env:OPENMSX_SYSTEM_DATA=Join-Path $kit 'tools/openmsx/share'
$env:OPENMSX_USER_DATA=Join-Path $root 'build/emulator/share'
$env:OPENMSX_HOME=Join-Path $root 'build/emulator'
$emulator=Join-Path $kit 'tools/openmsx/openmsx.exe'
if(!(Test-Path -LiteralPath $emulator)){throw 'openMSX non trovato: impostare MSX_KIT alla cartella della toolchain.'}
$soundArgs=@()
if($Audio -eq 'SCC'){$soundArgs=@('-ext','scc')}
if($Audio -eq 'FMPAC' -or $Audio -eq 'OPL4'){
 & "$PSScriptRoot/prepare-audio.ps1" -Audio $Audio
 $soundArgs=@('-ext',('Beholder_'+$Audio))
}
& "$kit/tools/openmsx/openmsx.exe" -machine C-BIOS_MSX2_EU -cart "$root/build/beholder-intro.rom" -romtype ASCII8 -script "$PSScriptRoot/play.tcl" @soundArgs



