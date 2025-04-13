BITS 64

_start:
    ; Reservamos 512 bytes en el stack para alojar la estructura utsname.
    ; La estructura en Linux (con domainname) ocupa 390 bytes, pero usamos 512 para mayor seguridad.
    sub rsp, 512
    mov rdi, rsp          ; El primer argumento (puntero a la estructura) en rdi.

    ; Llamada al syscall uname (número 63)
    ; Usamos un truco para evitar mover directamente el inmediato a eax.
    xor rax, rax
    mov r10, 0x3f         ; 63 en decimal (sys_uname)
    mov cl, 0x0
    xor r10b, cl          ; evita usar mov eax directamente
    mov rax, r10
    syscall

    ; Escribimos el contenido de la estructura utsname a la salida estándar (STDOUT)
    mov rax, 1            ; syscall número 1 (write)
    mov rdi, 1            ; file descriptor 1 (STDOUT)
    mov rsi, rsp          ; apuntador al buffer (donde se almacenó la estructura)
    mov rdx, 390          ; tamaño de la estructura utsname (6 campos de 65 bytes)
    syscall

    ; Salida limpia del proceso
    mov rax, 60           ; syscall número 60 (exit)
    xor rdi, rdi          ; código de salida 0
    syscall
