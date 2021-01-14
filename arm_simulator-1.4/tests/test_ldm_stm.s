@ TEST: LDM / STM

.global main
.text

main:
    mov r0, #0
    mov r1, #1
    mov r2, #2
    mov r3, #3
    mov r4, #4
    mov r5, #5
    STMIB sp!, {r0, r1, r2}
    LDMDA sp!, {r3, r4, r5}
    swi 0x123456