param([ValidateSet('FMPAC','OPL4')][string]$Audio)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$audioDir=Join-Path $root 'build/audio-runtime'
$extensions=Join-Path $env:OPENMSX_USER_DATA 'extensions'
New-Item -ItemType Directory -Force $audioDir,$extensions | Out-Null
if($Audio -eq 'FMPAC'){
 $rom=Join-Path $audioDir 'opll-signature.rom'
 if(!(Test-Path -LiteralPath $rom)){
  $data=New-Object byte[] 65536
  for($i=0;$i -lt $data.Length;$i++){$data[$i]=255}
  [Text.Encoding]::ASCII.GetBytes('PAC2OPLL').CopyTo($data,0x18)
  [IO.File]::WriteAllBytes($rom,$data)
 }
 $file=$rom.Replace('\','/')
 @"
<?xml version="1.0"?><!DOCTYPE msxconfig SYSTEM "msxconfig2.dtd">
<msxconfig><info><name>Beholder FM-PAC</name></info><devices><primary slot="any"><secondary slot="any"><FMPAC id="Beholder FM-PAC"><io base="0x7c" num="2" type="O"/><mem base="0x4000" size="0x4000"/><sound><volume>13000</volume></sound><rom><filename>$file</filename></rom><sramname>beholder.pac</sramname></FMPAC></secondary></primary></devices></msxconfig>
"@ | Set-Content "$extensions/Beholder_FMPAC.xml"
}else{
 $rom=Join-Path $audioDir 'blank-wave.rom'
 if(!(Test-Path -LiteralPath $rom)){[IO.File]::WriteAllBytes($rom,(New-Object byte[] 2097152))}
 $file=$rom.Replace('\','/')
 @"
<?xml version="1.0"?><!DOCTYPE msxconfig SYSTEM "msxconfig2.dtd">
<msxconfig><info><name>Beholder OPL4 FM</name></info><devices><MoonSound id="Beholder OPL4"><io base="0x7e" num="2"/><io base="0xc4" num="4"/><rom><filename>$file</filename></rom><sound><volume>17000</volume></sound><sampleram>640</sampleram></MoonSound></devices></msxconfig>
"@ | Set-Content "$extensions/Beholder_OPL4.xml"
}

