@ TEST: DATA_PROCESSING_4
.global main
.text

main:
	mov r0, #1
	mov r1, #3
	cmp r0, r1
    mvnlt r4, #15
end:
    swi 0x123456

.data