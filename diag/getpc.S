.text
.align 2
.global __get_PC
.type __get_PC, %function
__get_PC:
    grs r0, __get_PC
    jmp r15
