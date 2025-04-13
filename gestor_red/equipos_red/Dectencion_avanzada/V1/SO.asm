BITS 64

_start:
    sub rsp, 512         ; reservar espacio en stack para uname
    mov rdi, rsp         ; usar stack como buffer
    mov eax, 63          ; syscall uname
    syscall

    mov eax, 1           ; syscall write
    mov edi, 1           ; fd = 1 (stdout)
    mov rsi, rsp         ; buffer en rsp
    mov edx, 195         ; tamaño
    syscall

    mov eax, 60          ; syscall exit
    xor edi, edi
    syscall
