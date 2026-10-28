set throttle false
set pause_on_lost_focus false
set pause false
set root $env(BEHOLDER_ROOT)
set audio_mode $env(BEHOLDER_AUDIO_MODE)
set backend @BACKEND@
set clock @CLOCK@
set enabled @ENABLED@
set loops @LOOPS@
set expected $env(BEHOLDER_AUDIO_EXPECTED)
set failed 0
set log [open "$root/build/audio-test/${audio_mode}-checks.txt" w]
proc word {a} {expr {[debug read memory $a]+256*[debug read memory [expr {$a+1}]]}}
proc ensure {condition label} {global failed log;if {![uplevel 1 [list expr $condition]]} {incr failed;puts $log "FAIL $label"} else {puts $log "PASS $label"}}
proc press {row mask} {keymatrixdown $row $mask;after time 0.25 [list keymatrixup $row $mask]}
after time 22 {
 ensure {[debug read memory $backend]==$expected} {detect sound expansion or fall back to PSG}
 ensure {[word $clock]>200} {timer plays music while waiting for input}
 set initial [word $clock]
}
after time 72 {record start -audioonly "$root/music-${audio_mode}.wav"}
after time 92 {record stop;press 4 4;set before [word $clock]}
after time 93 {press 4 4}
after time 94 {press 3 4}
after time 95 {press 4 64}
after time 96 {press 5 16}
after time 97 {
 puts $log "render clock delta=[expr {[word $clock]-$before}]"
 ensure {[word $clock]-$before>=240 && [word $clock]-$before<=255} {music keeps tempo during map and banked graphics loading}
 record start -audioonly "$root/build/audio-test/effects-${audio_mode}.wav";press 8 1
}
after time 98 {press 8 1}
after time 99 {press 3 32}
after time 100 {press 8 1}
after time 101 {press 5 16}
after time 102 {press 5 16}
after time 104 {record stop}
after time 123 {
 ensure {[debug read memory $loops]>=1} {original score completes and loops}
 ensure {[word $clock]-$initial>=4980 && [word $clock]-$initial<=5080} {PAL playback maintains 50 Hz}
 press 5 4
}
after time 125 {ensure {[debug read memory $enabled]==0} {music mute};press 5 4}
after time 127 {ensure {[debug read memory $enabled]==1} {music resume};press 2 128}
after time 129 {ensure {[debug read memory $backend]==0} {switch to PSG without restarting game};press 2 128}
after time 131 {
 ensure {[debug read memory $backend]==$expected} {restore best available backend}
 set f [open "$root/build/audio-test/${audio_mode}-result.txt" w];puts $f "failures=$failed";close $f
 close $log;exit
}



after time 8 {press 8 1}
after time 10 {press 5 1}
after time 12 {press 7 128}
after time 16 {press 7 128}



