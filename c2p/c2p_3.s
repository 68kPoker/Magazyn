����                                        
	move.l	#$01010101,d4
	move.l	#$f0ccaa00,d5
	move.l	#$0f0f0f0f,d7

	lea	c2p(pc),a0
	movea.l	a0,a1
	adda.l	#eoc2p-c2p,a1
	jsr	routine
	rts

c2p:	ds.b	81920
eoc2p:
routine:

.start:
	clr.l	d2

	move.l	(a0)+,d0
	and.l	d7,d0
	moveq	#4-1,d1
.next:
	rol.l	#8,d0

	move.l	d5,d3
	bclr	#3,d0
	sne	d3
	ror.l	#8,d3

	lsr.l	d0,d3
	and.l	d4,d3

	lsl.l	#1,d2
	or.l	d3,d2

	dbf	d1,.next

	move.l	(a0)+,d0
	and.l	d7,d0
	moveq	#4-1,d1

.next2:
	rol.l	#8,d0

	move.l	d5,d3
	bclr	#3,d0
	sne	d3
	ror.l	#8,d3

	lsr.l	d0,d3
	and.l	d4,d3

	lsl.l	#1,d2
	or.l	d3,d2

	dbf	d1,.next2

	cmpa.l	a0,a1
	bgt.s	.start

	rts
