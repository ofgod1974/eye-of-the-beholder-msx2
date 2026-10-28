; Keyboard matrix through the standard MSX PPI. Stack ABI, result in L.
.area _CODE
_matrix::
 ld hl,#2
 add hl,sp
 ld c,(hl)
 di
 in a,(#0xaa)
 and #0xf0
 or c
 out (#0xaa),a
 in a,(#0xa9)
 ld l,a
 ei
 ret
