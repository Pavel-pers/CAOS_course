.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global ArgSumShort
ArgSumShort:
    mov rax, rdi
    mov rcx, qword ptr [rsi]
    add rax, rcx
    mov rcx, qword ptr [rdx]
    mov rcx, qword ptr [rcx]
    add rax, rcx
    ret

.global ArgSumLong
ArgSumLong:
    mov rax, r9
    mov rdx, qword ptr [rsp + 8]
    mov rdx, qword ptr [rdx]
    add rax, rdx
    mov rcx, qword ptr [rsp + 16]
    mov rcx, qword ptr [rcx]
    mov rcx, qword ptr [rcx]
    add rax, rcx
    ret
