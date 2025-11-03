.intel_syntax noprefix
.section .note.GNU-stack

.section .text

.global IncRef
IncRef:
    cmp rdi, 0
    je  IncRef_done          
    inc qword ptr [rdi + 8]
IncRef_done:
    ret

.global DecRef
DecRef:
    cmp rdi, 0
    je  DecRef_ret
    push rbx                 

DecRef_loop:
    dec qword ptr [rdi + 8]  
    jne DecRef_done          
    mov rbx, qword ptr [rdi] 
    mov rax, rdi             
    mov rdi, rax
    call free                
    mov rdi, rbx
    test rdi, rdi
    jne DecRef_loop          

DecRef_done:
    pop rbx
DecRef_ret:
    ret

.global Push
Push:
    push rbx                 
    mov rbx, rdi             
    mov rdx, rsi             
    mov edi, 24              
    call malloc
    test rax, rax
    je  Push_return_null     
    mov qword ptr [rax], rbx   
    mov qword ptr [rax + 8], 1     
    mov qword ptr [rax + 16], rdx  
    test rbx, rbx
    je Push_done
    mov rdi, rbx
    call IncRef

Push_done:
    pop rbx
    ret

Push_return_null:
    pop rbx
    ret
