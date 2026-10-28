set heroes @HEROES@
set front_phase @PHASE@
set pause_on_lost_focus false
set pause false
set throttle false
set failed 0
set root $env(BEHOLDER_ROOT)
set log [open "$root/build/checks.txt" w]
proc press {row mask} {keymatrixdown $row $mask;after time 0.25 [list keymatrixup $row $mask]}
proc byte {offset} {debug read memory [expr {0xC000+$offset}]}
proc ensure {condition label} {
 global failed log
 if {![uplevel 1 [list expr $condition]]} {incr failed;puts $log "FAIL $label state=[binary encode hex [debug read_block memory 0xC000 11]]"} else {puts $log "PASS $label"}
}
proc capture {prefix} {
 global root
 set f [open "$root/build/${prefix}vram.bin" wb];puts -nonewline $f [debug read_block VRAM 0 27136];close $f
 set f [open "$root/build/${prefix}palette.bin" wb];puts -nonewline $f [debug read_block {VDP palette} 0 32];close $f
}
after time 22 {
 ensure {[byte 0]==1 && [byte 1]==10 && [byte 6]==12 && [byte 7]==36 && [byte 10]==1} {initial party and ready frame}
 ensure {[debug read memory 0xFCAF]==5} {SCREEN 5 active}
 capture "";press 5 16
}
after time 25 {ensure {[byte 1]==9} {forward step};press 5 16}
after time 28 {ensure {[byte 1]==9} {kobold blocks movement};capture combat-;press 8 1}
after time 31 {ensure {[byte 6]==8 && [byte 7]==30} {attack and retaliation};press 8 1}
after time 34 {ensure {[byte 6]==4 && [byte 7]==24} {second hit};press 3 32}
after time 37 {ensure {[byte 7]==36 && [byte 8]==1} {healing consumes potion};press 8 1}
after time 40 {ensure {[byte 6]==0 && [byte 7]==36} {defeat kobold};press 5 16}
after time 43 {ensure {[byte 1]==8} {walk through defeated enemy};press 5 16}
after time 46 {ensure {[byte 1]==7 && [byte 5]==1} {collect key};press 5 16}
after time 49 {ensure {[byte 1]==7} {wall collision};press 3 4}
after time 52 {ensure {[byte 2]==1} {turn right};press 4 4}
after time 55 {
 ensure {[byte 3]==1} {map toggle};capture map-
 debug write memory 0xC000 5;debug write memory 0xC001 1;debug write memory 0xC005 0
 press 4 4
}
after time 58 {ensure {[byte 3]==0} {close map};capture door-;press 3 8}
after time 61 {ensure {[byte 4]==0} {locked without key};debug write memory 0xC005 1;press 3 8}
after time 64 {ensure {[byte 4]==1} {unlock door};press 5 16}
after time 67 {
 ensure {[byte 0]==6} {cross open door}
 debug write memory 0xC000 9;debug write memory 0xC001 1;press 5 16
}
after time 70 {ensure {[byte 0]==10 && [byte 9]==1} {victory at exit};press 2 64}
after time 73 {ensure {[byte 0]==10} {victory stops movement};press 4 128}
after time 76 {
 ensure {[byte 0]==1 && [byte 1]==10 && [byte 6]==12 && [byte 7]==36 && [byte 8]==2 && [byte 9]==0} {restart resets all state}
 debug write memory 0xC001 9;debug write memory 0xC007 6
 for {set i 0} {$i<4} {incr i} {debug write memory [expr {$heroes+$i*23+20}] 0}
 debug write memory [expr {$heroes+20}] 6;press 8 1
}
after time 79 {ensure {[byte 7]==0} {party death};press 5 16}
after time 82 {ensure {[byte 1]==9} {dead party cannot move};press 4 128}
after time 85 {
 ensure {[byte 1]==10 && [byte 7]==36 && [byte 6]==12} {restart after death}
 set f [open "$root/build/verification.txt" w];puts $f "failures=$failed";close $f
 flush $log
}

# End-to-end route after restart: no party position or inventory fixtures.
set route {w hit hit hit w w e w w q w w w w e w w w q w f w e w w w w}
set at 88
foreach key $route {
 set pair [dict get {w {5 16} q {4 64} e {3 4} f {3 8} hit {8 1}} $key]
 after time $at [list press {*}$pair]
 incr at 2
}
after time $at {
 ensure {[byte 0]==10 && [byte 1]==1 && [byte 9]==1 && [byte 4]==1 && [byte 5]==1 && [byte 6]==0} {complete walkthrough without position fixtures}
 set f [open "$root/build/verification.txt" w];puts $f "failures=$failed";close $f
 close $log;exit
}

after time 8 {press 8 1}
after time 10 {press 5 1}
after time 12 {press 7 128}
after time 16 {press 7 128}




