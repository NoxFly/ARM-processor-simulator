@ TEST: DATA_PROCESSING_1
.global main
.text

main:
	mov r0, #1
	mov r0, r0, LSL #30
	mov r1, #3
	mov r1, r1, LSL #30
    addS r1, r0, r1
	movCS r0, #5
	mov r0, #5
end:
    swi 0x123456

.data