.global main
.text
main:

   mov r0, #0x1000
   mov r0, #0x1100
   ldr r2, [r0]
   str r2, [r1]
.data
   var1: .word 3
   var2: .word 4

fin:
