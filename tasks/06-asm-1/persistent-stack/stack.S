.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.extern malloc
.extern free

.global IncRef
IncRef:
    test rdi, rdi
    je .IncRef_done
    inc qword ptr [rdi + 8]
.IncRef_done:
    ret

.global DecRef
DecRef:
    test rdi, rdi
    je .DecRef_return

    push rbx
    sub rsp, 8
.DecRef_loop:
    dec qword ptr [rdi + 8]
    jne .DecRef_exit

    mov rbx, qword ptr [rdi]
    call free

    mov rdi, rbx
    test rdi, rdi
    jne .DecRef_loop

.DecRef_exit:
    add rsp, 8
    pop rbx
    ret

.DecRef_return:
    ret

.global Push
Push:
    push rdi
    push rsi

    mov edi, 24
    call malloc
    test rax, rax
    je .Push_return_null

    mov rdx, qword ptr [rsp + 8]
    mov rcx, qword ptr [rsp]

    mov qword ptr [rax], rdx
    mov qword ptr [rax + 8], 1
    mov qword ptr [rax + 16], rcx

    test rdx, rdx
    je .Push_done

    mov r8, rax
    mov rdi, rdx
    call IncRef
    mov rax, r8

.Push_done:
    add rsp, 16
    ret

.Push_return_null:
    add rsp, 16
    ret
