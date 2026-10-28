$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/build.ps1"
$kit=$env:MSX_KIT
if(!$kit){$kit='C:\Users\Andrea\Documents\Codex\2026-10-27\cr\outputs\eagle-nest-msx'}
$test=Join-Path $root 'build/audio-test'
New-Item -ItemType Directory -Force $test | Out-Null
# Test ROMs contain no commercial firmware: only an OPLL signature or blank wave data.
$fm=New-Object byte[] 65536
for($i=0;$i -lt $fm.Length;$i++){$fm[$i]=255}
[Text.Encoding]::ASCII.GetBytes('PAC2OPLL').CopyTo($fm,0x18)
[IO.File]::WriteAllBytes("$test/fmpac-stub.rom",$fm)
[IO.File]::WriteAllBytes("$test/blank-wave.rom",(New-Object byte[] 2097152))
$fmPath=(Join-Path $test 'fmpac-stub.rom').Replace('\','/')
$wavePath=(Join-Path $test 'blank-wave.rom').Replace('\','/')
@"
<?xml version="1.0"?><!DOCTYPE msxconfig SYSTEM "msxconfig2.dtd">
<msxconfig><info><name>FMPAC audio test</name></info><devices><primary slot="any"><secondary slot="any"><FMPAC id="FMPAC test"><io base="0x7c" num="2" type="O"/><mem base="0x4000" size="0x4000"/><sound><volume>13000</volume></sound><rom><filename>$fmPath</filename></rom><sramname>test.pac</sramname></FMPAC></secondary></primary></devices></msxconfig>
"@ | Set-Content "$test/fmpac-test.xml"
@"
<?xml version="1.0"?><!DOCTYPE msxconfig SYSTEM "msxconfig2.dtd">
<msxconfig><info><name>MoonSound FM test</name></info><devices><MoonSound id="MoonSound FM test"><io base="0x7e" num="2"/><io base="0xc4" num="4"/><rom><filename>$wavePath</filename></rom><sound><volume>17000</volume></sound><sampleram>640</sampleram></MoonSound></devices></msxconfig>
"@ | Set-Content "$test/opl4-test.xml"
$template=Get-Content "$PSScriptRoot/verify-audio.tcl" -Raw
$map=Get-Content "$root/build/beholder.map" -Raw
foreach($name in @('backend','clock','enabled','loops')){
 $match=[regex]::Match($map,'(?m)^\s*([0-9A-F]+)\s+_music_'+$name+'\s')
 if(!$match.Success){throw "Simbolo audio non trovato: $name"}
 $template=$template.Replace('@'+$name.ToUpper()+'@','0x'+$match.Groups[1].Value)
}
$template | Set-Content "$test/verify.tcl"
$env:BEHOLDER_ROOT=$root.Replace('\','/')
$env:OPENMSX_SYSTEM_DATA=Join-Path $kit 'tools/openmsx/share'
$jobs=@()
foreach($mode in @('psg','scc','fmpac','opl4')){
 $env:BEHOLDER_AUDIO_MODE=$mode
 $env:BEHOLDER_AUDIO_EXPECTED=@{psg=0;scc=1;fmpac=2;opl4=3}[$mode]
 $env:OPENMSX_HOME="$test/$mode-home"
 $env:OPENMSX_USER_DATA="$test/$mode-home/share"
 New-Item -ItemType Directory -Force "$env:OPENMSX_USER_DATA/extensions" | Out-Null
 Copy-Item "$test/fmpac-test.xml" "$env:OPENMSX_USER_DATA/extensions/Beholder_FMPAC.xml" -Force
 Copy-Item "$test/opl4-test.xml" "$env:OPENMSX_USER_DATA/extensions/Beholder_OPL4.xml" -Force
 $argsList=@('-machine','C-BIOS_MSX2_EU','-cart',('"'+$root+'/build/beholder-intro.rom"'),'-romtype','ASCII8','-script',('"'+$test+'/verify.tcl"'))
 if($mode -eq 'scc'){$argsList+=@('-ext','scc')}
 if($mode -eq 'fmpac'){$argsList+=@('-ext','Beholder_FMPAC')}
 if($mode -eq 'opl4'){$argsList+=@('-ext','Beholder_OPL4')}
 $jobs+=@{Mode=$mode;Process=(Start-Process "$kit/tools/openmsx/openmsx.exe" -ArgumentList $argsList -WindowStyle Hidden -PassThru -RedirectStandardError "$test/$mode-stderr.txt")}
}
foreach($job in $jobs){
 if(!$job.Process.WaitForExit(55000)){$job.Process.Kill();throw "Timeout audio $($job.Mode)"}
 $report="$test/$($job.Mode)-result.txt"
 if(!(Test-Path $report) -or (Get-Content $report) -ne 'failures=0'){
  Get-Content "$test/$($job.Mode)-stderr.txt"
  Get-Content "$test/$($job.Mode)-checks.txt" -ErrorAction SilentlyContinue
  throw "Verifica audio fallita: $($job.Mode)"
 }
 Get-Content "$test/$($job.Mode)-checks.txt"
}




