set throttle false
set pause_on_lost_focus false
set pause false
set root $env(BEHOLDER_ROOT)
set heroes @HEROES@
set phase @PHASE@
set scene @SCENE@
set front_ready @FRONTREADY@
set maximum @MAXIMUM@
set failed 0
set log [open "$root/build/front-checks.txt" w]
proc byte {at} {debug read memory $at}
proc hero {n at} {global heroes;byte [expr {$heroes+$n*23+$at}]}
proc ensure {condition label} {global failed log; if {![uplevel 1 [list expr $condition]]} {incr failed;puts $log "FAIL $label"} else {puts $log "PASS $label"};flush $log}
proc press {row mask} {keymatrixdown $row $mask;after time 0.15 [list keymatrixup $row $mask]}
proc capture {name} {global root;set f [open "$root/build/$name-vram.bin" wb];puts -nonewline $f [debug read_block VRAM 0 27136];close $f;set f [open "$root/build/$name-palette.bin" wb];puts -nonewline $f [debug read_block {VDP palette} 0 32];close $f}
after time 7 {ensure {[byte $phase]==0 && [byte $scene]==0 && [byte $front_ready]==1} {original logo intro ready};capture intro-logo}
after time 9 {press 8 1}
after time 12 {ensure {[byte $phase]==1} {skip intro to title menu};capture menu;press 7 128}
after time 15 {ensure {[byte $phase]==2 && [byte $front_ready]==1} {open character creation};capture creator;press 3 4}
after time 18 {ensure {[hero 0 19]==1} {select another original portrait};press 4 128}
after time 21 {ensure {[hero 0 15]==1} {select elf race};press 3 1}
after time 24 {ensure {[hero 0 16]==3} {cycle legal classes for elf};press 3 8}
after time 27 {ensure {[hero 0 17]==1} {change character sex};press 3 16}
after time 30 {ensure {[hero 0 18]==2} {change alignment};press 3 2}
after time 33 {ensure {[hero 0 9]==18} {modify strength within race bounds};press 4 8}
after time 35 {press 7 32}
after time 35.6 {press 7 32}
after time 36.2 {press 7 32}
after time 36.8 {press 7 32}
after time 37.4 {press 2 64}
after time 38 {press 4 2}
after time 38.6 {press 3 2}
after time 39.2 {press 3 4}
after time 39.8 {press 4 128}
after time 40.4 {press 7 128}
after time 43 {
 ensure {[binary encode hex [debug read_block memory $heroes 6]] eq "414c44455200"} {edit and retain custom name ALDER}
 ensure {[hero 0 21]==6} {mage class and constitution determine hit points}
 capture creator-custom;press 7 128
}
after time 46 {press 7 128}
after time 49 {press 7 128}
after time 52 {press 7 128}
after time 55 {ensure {[byte $phase]==3} {confirm all four heroes and review party};capture party-review;press 0 2}
after time 58 {ensure {[byte $phase]==2} {edit a confirmed hero};press 5 2}
after time 61 {
 set lower {3 7 6 9 3 8};set upper {18 19 17 18 18 18};set ok 1
 for {set i 0} {$i<6} {incr i} {set v [hero 0 [expr {9+$i}]];if {$v<[lindex $lower $i] || $v>[lindex $upper $i]} {set ok 0}}
 ensure {$ok} {reroll respects original race and class bounds}
 set con [hero 0 11];set bonus [expr {$con>=15?min(2,$con-14):($con<=6?-1:0)}];set selected_hp [expr {4+$bonus}]
 press 7 128
}
after time 64 {press 7 128}
after time 68 {
 ensure {[byte $phase]==4 && [byte 0xC00A]==1} {enter dungeon after party confirmation}
 ensure {[hero 0 20]==$selected_hp && [hero 0 21]==$selected_hp} {selected class and constitution transfer into gameplay}
 ensure {[byte 0xC007]==[byte $maximum] && [byte $maximum]==$selected_hp+24} {individual hit points form the party total}
 ensure {[hero 0 19]==1 && [hero 0 15]==1 && [hero 0 16]==3} {selected portrait race and class survive entry}
 capture selected-party;press 4 32
}
after time 73 {ensure {[byte $phase]==5} {open individual party sheets in dungeon};capture party-sheet;press 7 4}
after time 76 {ensure {[byte $phase]==4} {close sheets without resetting game};press 6 64}
after time 80 {ensure {[byte $phase]==0} {F2 returns to frontend};press 8 1}
after time 83 {ensure {[byte $phase]==1} {frontend title menu restored};press 5 16}
after time 86 {press 7 128}
after time 89 {ensure {[byte $phase]==0 && [byte $scene]==0} {replay introduction from title menu}}
after time 99 {ensure {[byte $phase]==0 && [byte $scene]==1} {title scene advances automatically};capture intro-title}
after time 110 {ensure {[byte $phase]==0 && [byte $scene]==2} {Waterdeep intro scene};capture intro-city}
after time 124 {ensure {[byte $phase]==0 && [byte $scene]==3} {lords intro scene};capture intro-lords}
after time 137 {ensure {[byte $phase]==0 && [byte $scene]==4} {sewer intro scene};capture intro-sewer}
after time 150 {ensure {[byte $phase]==0 && [byte $scene]==5} {blocked exit intro scene};capture intro-blocked}
after time 166 {
 ensure {[byte $phase]==1} {full introduction returns to menu automatically}
 set f [open "$root/build/front-result.txt" w];puts $f "failures=$failed";close $f;close $log;exit
}

