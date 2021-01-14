@  TEST: LDMH / STMH

.global main
.text

main:
    mov r0, #0
    mov r1, #1
    mov r2, #2
    mov r3, #3
    LDRH r0, [r0,#8]
    STRH r0, [r0,#4]
    swi 0x123456