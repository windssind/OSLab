global my_print
section .text

my_print:
    push rbp
    
    mov rdx, rsi
    mov rsi, rdi

    mov eax, 1    ; 系统调用号为 1，表示写入到标准输出
    mov edi, 1    ; 文件描述符为 1，表示标准输出

    ; 进行系统调用
    syscall

    pop rbp
    ret

