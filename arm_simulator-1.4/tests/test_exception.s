@ TEST: EXCEPTIONS

.global main
.text

main:
    mov r2, #2
    ldrsb r2,[r3,r4]
    swi 0x123456