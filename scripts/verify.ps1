$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/build.ps1"
$kit=$env:MSX_KIT
if(!$kit){$kit='C:\Users\Andrea\Documents\Codex\2026-10-27\cr\outputs\eagle-nest-msx'}
$env:BEHOLDER_ROOT=$root.Replace('\','/')
$env:OPENMSX_SYSTEM_DATA=Join-Path $kit 'tools/openmsx/share'
$env:OPENMSX_USER_DATA=Join-Path $root 'build/emulator/share'
$env:OPENMSX_HOME=Join-Path $root 'build/emulator'
$report=Join-Path $root 'build/verification.txt'
if(Test-Path $report){Remove-Item -LiteralPath $report}
$tcl=Get-Content "$PSScriptRoot/verify.tcl" -Raw
$map=Get-Content "$root/build/beholder.map" -Raw
foreach($entry in @(@('heroes','HEROES'),@('front_phase','PHASE'))){
 $match=[regex]::Match($map,'(?m)^\s*([0-9A-F]+)\s+_'+$entry[0]+'\s')
 if(!$match.Success){throw "Simbolo non trovato: $($entry[0])"}
 $tcl=$tcl.Replace('@'+$entry[1]+'@','0x'+$match.Groups[1].Value)
}
$tcl | Set-Content "$root/build/verify-game.tcl"
$argsList=@('-machine','C-BIOS_MSX2_EU','-cart',('"'+$root+'/build/beholder-intro.rom"'),'-romtype','ASCII8','-script',('"'+$root+'/build/verify-game.tcl"'))
$process=Start-Process "$kit/tools/openmsx/openmsx.exe" -ArgumentList $argsList -WindowStyle Hidden -PassThru -RedirectStandardError "$root/build/verification.log"
if(!$process.WaitForExit(45000)){$process.Kill();throw 'Timeout emulatore'}
Get-Content "$root/build/verification.log"
if(!(Test-Path $report) -or (Get-Content $report) -ne 'failures=0'){throw 'Verifica MSX2 fallita'}
# Decode the actual SCREEN 5 VRAM and VDP palette, independently of the hidden window renderer.
Add-Type -AssemblyName System.Drawing
foreach($prefix in @('','combat-','door-','map-')){
 $v=[IO.File]::ReadAllBytes("$root/build/${prefix}vram.bin")
 $pal=[IO.File]::ReadAllBytes("$root/build/${prefix}palette.bin")
 $bmp=[Drawing.Bitmap]::new(256,212)
 try {
  for($y=0;$y -lt 212;$y++){for($x=0;$x -lt 256;$x++){
   $b=$v[$y*128+[int][Math]::Floor($x/2)]
   $n=if($x%2 -eq 0){$b -shr 4}else{$b -band 15}
   $r=($pal[$n*2] -shr 4) -band 7;$g=$pal[$n*2+1] -band 7;$blue=$pal[$n*2] -band 7
   $bmp.SetPixel($x,$y,[Drawing.Color]::FromArgb([int]($r*255/7),[int]($g*255/7),[int]($blue*255/7)))
  }}
  $bmp.Save("$root/${prefix}preview.png",[Drawing.Imaging.ImageFormat]::Png)
 } finally {$bmp.Dispose()}
}
Write-Host 'Verifica completata: 24 controlli, quattro viste VRAM esportate.'




