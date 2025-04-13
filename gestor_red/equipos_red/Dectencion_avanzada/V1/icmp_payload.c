#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define MAX_PACKET_SIZE 1500
#define ICMP_FILTER_ID 1234

// ===============================
// FUNCIONES AUXILIARES
// ===============================

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

// ===============================
// MAIN
// ===============================

int main() {
    pthread_t listener;
    if (pthread_create(&listener, NULL, listener_thread, NULL) != 0) {
        perror("[ERROR] No se pudo crear el hilo de escucha");
        return 1;
    }

    printf("[INFO] Receptor ICMP escuchando...\n");
    printf("[COMANDO] Para enviar: send <IP> <archivo>\n\n");

    char linea[256];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(linea, sizeof(linea), stdin)) break;
        linea[strcspn(linea, "\n")] = 0;

        if (strncmp(linea, "send ", 5) == 0) {
            char ip[64], archivo[128];
            if (sscanf(linea + 5, "%63s %127s", ip, archivo) == 2) {
                char *payload = NULL;
                int tam = 0;
                if (cargar_payload(archivo, &payload, &tam) == 0) {
                    enviar_icmp(ip, payload, tam);
                    free(payload);
                }
            } else {
                printf("[ERROR] Uso: send <IP> <archivo>\n");
            }
        } else if (strcmp(linea, "exit") == 0) {
            break;
        } else {
            printf("[INFO] Comando no reconocido. Usa: send <IP> <archivo> o exit\n");
        }
    }

    return 0;
}
