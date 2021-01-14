@ TEST: DATA_PROCESSING_5
.global main
.text

main:
	mov r0, #1
	adc r0, r0, #0
	mov r2, #1
	mov r2, r2, LSL #30
	mov r3, #3
	mov r3, r3, LSL #30
	cmn r2, r3
	adc r0, r0, #0
	mov r1, #1
	eor r1, r0, r1
	eor r1, r0, r1
end:
    swi 0x123456

.data