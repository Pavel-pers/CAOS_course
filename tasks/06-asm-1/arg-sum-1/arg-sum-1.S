.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global ArgSum
ArgSum:
    mov rax, rdi
    add rax, rsi
    add rax, rdx
    add rax, rcx
    ret
