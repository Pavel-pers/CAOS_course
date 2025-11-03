.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global TreeSum
TreeSum:
    test rdi, rdi
    je .return_zero
    push rbx
    push r12
    mov rbx, rdi
    mov r12, qword ptr [rbx]
    mov rdi, qword ptr [rbx + 8]
    call TreeSum
    add r12, rax
    mov rdi, qword ptr [rbx + 16]
    call TreeSum
    add rax, r12
    pop r12
    pop rbx
    ret

.return_zero:
    xor eax, eax
    ret
