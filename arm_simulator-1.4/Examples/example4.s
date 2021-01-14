.global main
.text

add_value:
    ADD r1, r1, #4
    mov pc, lr

main:
    mov r1, #4
    bl add_value
end:
    swi 0x123456

.data
