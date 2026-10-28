$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$kit=$env:MSX_KIT
if(!$kit){$kit='C:\Users\Andrea\Documents\Codex\2026-10-27\cr\outputs\eagle-nest-msx'}
$fusion=Join-Path $kit 'vendor/fusion-c/Working Folder/fusion-c'
$cc=Join-Path $kit 'tools/sdcc/bin/sdcc.exe'
if(!(Test-Path $cc)){throw 'Impostare MSX_KIT alla cartella della toolchain Fusion-C/SDCC.'}
$nodeCandidates=@($env:NODE_EXE)
$nodeCommand=Get-Command node.exe -ErrorAction SilentlyContinue
if($nodeCommand){$nodeCandidates += $nodeCommand.Source}
$nodeCandidates += @(
 (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe'),
 (Join-Path $env:ProgramFiles 'nodejs/node.exe')
)
$nodeExe=$nodeCandidates | Where-Object {$_ -and (Test-Path -LiteralPath $_ -PathType Leaf)} | Select-Object -First 1
if(!$nodeExe){throw 'Node.js non trovato. Impostare NODE_EXE al percorso di node.exe. Per giocare alla ROM pronta, usare Gioca.cmd.'}
$env:PATH=(Split-Path $cc)+';'+$env:PATH
New-Item -ItemType Directory -Force "$root/build" | Out-Null
Push-Location "$root/build"
try {
 & $nodeExe "$root/scripts/generate-assets.cjs" | Out-Null
 if($LASTEXITCODE){throw "Errore conversione VGA"}
 & $nodeExe "$root/scripts/import-audio.cjs" | Out-Null
 if($LASTEXITCODE){throw "Errore importazione colonna sonora"}
 & $cc -mz80 --sdcccall 0 --no-optsdcc-in-asm --disable-warning 296 --opt-code-size -I "$fusion/header" -c "$root/src/audio.c" -o audio.rel
 if($LASTEXITCODE){throw "Errore audio"}
 & (Join-Path (Split-Path $cc) "sdasz80.exe") -o audio_irq.rel "$root/src/audio_irq.s"
 if($LASTEXITCODE){throw "Errore interrupt audio"}
 & (Join-Path (Split-Path $cc) "sdasz80.exe") -o video.rel "$root/src/video.s"
 if($LASTEXITCODE){throw "Errore caricatore video"}
 & (Join-Path (Split-Path $cc) 'sdasz80.exe') -o crt0.rel "$root/src/crt0.s"
 if($LASTEXITCODE){throw 'Errore startup ROM'}
 & (Join-Path (Split-Path $cc) 'sdasz80.exe') -o input.rel "$root/src/input.s"
 if($LASTEXITCODE){throw 'Errore input'}
 & $cc -mz80 --sdcccall 0 --no-optsdcc-in-asm --disable-warning 296 --opt-code-size -c "$root/src/math.c" -o math.rel
 if($LASTEXITCODE){throw 'Errore runtime aritmetico'}
 & $cc -mz80 --sdcccall 0 --no-optsdcc-in-asm --disable-warning 296 --disable-warning 360 --disable-warning 357 --opt-code-size -I "$fusion/header" -c "$root/src/front.c" -o front.rel
 if($LASTEXITCODE){throw "Errore menu e personaggi"}
 $objects=@('input.rel','math.rel','video.rel','front.rel','audio.rel','audio_irq.rel')
 foreach($module in @('setcolors','cls','vdp_graph2plus','keysound','readmsxtype','print')){
  & $cc -mz80 --sdcccall 0 --no-optsdcc-in-asm --disable-warning 296 -c "$fusion/source/lib/$module.c" -o "$module.rel"
  if($LASTEXITCODE){throw "Errore Fusion-C: $module"}
  $objects += "$module.rel"
 }
 & $cc -mz80 --sdcccall 0 --no-optsdcc-in-asm --disable-warning 296 --disable-warning 357 --disable-warning 360 --no-std-crt0 --code-loc 0x4070 -Wl-b_HOME=0x9c00 --data-loc 0xC000 --opt-code-size -I "$fusion/header" -L "$fusion/lib" crt0.rel @objects "$root/src/main.c" fusion.lib -o beholder.ihx
 if($LASTEXITCODE){throw 'Compilazione fallita'}
 $assetInfo=Get-Content "$root/assets/generated/manifest.json" -Raw | ConvertFrom-Json
 $rom=New-Object byte[] $assetInfo.romBytes
 for($i=0;$i -lt $rom.Length;$i++){$rom[$i]=255}
 foreach($record in Get-Content beholder.ihx){
  if(!$record.StartsWith(':')){continue}
  $n=[Convert]::ToInt32($record.Substring(1,2),16)
  $a=[Convert]::ToInt32($record.Substring(3,4),16)
  $type=[Convert]::ToInt32($record.Substring(7,2),16)
  $sum=0
  for($j=1;$j -lt $record.Length;$j+=2){$sum += [Convert]::ToInt32($record.Substring($j,2),16)}
  if(($sum -band 255) -ne 0){throw 'Checksum Intel HEX non valido'}
  if($type -eq 0){for($j=0;$j -lt $n;$j++){
   if($a+$j -ge 0xC000 -and $a+$j -lt 0xC100 -and $record.Substring(9+$j*2,2) -eq '00'){continue} # RAM cleared by startup
   if($a+$j -lt 0x4000 -or $a+$j -ge 0xA000){throw 'Codice fuori dai 24KB fissi ASCII8'}
   $rom[$a+$j-0x4000]=[Convert]::ToByte($record.Substring(9+$j*2,2),16)
  }}
 }
 if($rom[0] -ne 65 -or $rom[1] -ne 66){throw 'Header AB mancante'}
 $payload=[IO.File]::ReadAllBytes("$root/assets/generated/assets.bin")
 [Array]::Copy($payload,0,$rom,24576,$payload.Length)
 $music=[IO.File]::ReadAllBytes("$root/assets/generated/music.bin")
 if(24576+$payload.Length -gt 0x1e0000){throw "Sovrapposizione asset grafici e musicali"}
 [Array]::Copy($music,0,$rom,0x1e0000,$music.Length)
 [IO.File]::WriteAllBytes("$root/build/beholder-intro.rom",$rom)
 Write-Host 'ROM MSX2 ASCII8: build/beholder-intro.rom (2 MB)'
}finally{Pop-Location}









