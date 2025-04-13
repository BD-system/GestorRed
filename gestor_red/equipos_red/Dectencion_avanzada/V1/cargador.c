#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    // Abrir el binario
    FILE *f = fopen("SO.bin", "rb");
    if (!f) {
        perror("No se pudo abrir SO.bin");
        return 1;
    }

    // Leer contenido a buffer
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    rewind(f);

    unsigned char *buffer = malloc(len);
    fread(buffer, 1, len, f);
    fclose(f);

    // Reservar memoria ejecutable
    void *exec = mmap(NULL, len, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (exec == MAP_FAILED) {
        perror("mmap falló");
        return 1;
    }

    memcpy(exec, buffer, len);
    free(buffer);

    // Ejecutar
    ((void(*)())exec)();

    return 0;
}
