; ASCII8 RLE asset stream -> linear VRAM, SDCC stack ABI.
.area _CODE
.globl _VideoWait
_VideoWait::
 ; Poll CE with interrupts enabled between status reads. Audio keeps ticking.
007$:
 di
 ld a,#2
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 in a,(#0x99)
 push af
 xor a
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 ei
 pop af
 and #1
 jr nz,007$
 ret
_stream::
 push ix
 ld ix,#0
 add ix,sp
 di
 ld a,#2
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
001$:
 in a,(#0x99)
 and #1
 jr nz,001$
 xor a
 out (#0x99),a
 ld a,#0x8f
 out (#0x99),a
 ld a,7(ix)
 out (#0x99),a
 ld a,#0x8e
 out (#0x99),a
 xor a
 out (#0x99),a
 ld a,#0x40
 out (#0x99),a
 ld a,4(ix)
 ld (0x7800),a
 ld (stream_bank),a
 ld l,5(ix)
 ld h,6(ix)
 set 7,h
 set 5,h
 ld e,8(ix)
 ld d,9(ix)
 ei
002$:
 ld a,d
 or e
 jr z,006$
 call stream_get
 ld c,a
 and #0x7f
 inc a
 ld b,a
 bit 7,c
 jr z,004$
 call stream_get
003$:
 out (#0x98),a
 dec de
 djnz 003$
 jr 002$
004$:
 call stream_get
 out (#0x98),a
 dec de
 djnz 004$
 jr 002$
006$:
 di
 xor a
 out (#0x99),a
 ld a,#0x8e
 out (#0x99),a
 ei
 pop ix
 ret
stream_get:
 ld a,(hl)
 inc hl
 bit 6,h
 ret z
 push af
 di
 ld a,(stream_bank)
 inc a
 ld (stream_bank),a
 ld (0x7800),a
 ld hl,#0xa000
 ei
 pop af
 ret
.area _DATA
.globl _stream_bank
stream_bank:
_stream_bank:: .ds 1


