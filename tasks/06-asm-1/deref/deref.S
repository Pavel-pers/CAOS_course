.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global Deref
Deref:
    mov rax, rdi
    test rsi, rsi
    je .return_value

.loop:
    mov rax, qword ptr [rax]
    dec rsi
    jne .loop

.return_value:
    ret
