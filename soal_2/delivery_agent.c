#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h> 

typedef struct {
    char nama[50];
    char alamat[100];
    char tipe[10];
    char status[30]; 
} Order;

#define SHM_KEY 12345

typedef struct {
    char agent_name[10];
    int shmid;
} AgentArgs;

// fungsi timestamp
void get_timestamp(char *timestamp) {
    time_t timer;
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(timestamp, 26, "%d/%m/%Y %H:%M:%S", tm_info);
}
// fungsi thread agen
void *process_express_orders(void *arg) {
    AgentArgs *args = (AgentArgs *)arg;
    char *agent_name = args->agent_name;
    int shmid = args->shmid;
    Order *orders;
    int *num_orders;

    orders = (Order *)shmat(shmid, NULL, 0);
    if (orders == (void *)-1) {
        perror("Agent gagal attach");
        pthread_exit(NULL);
    }

    num_orders = (int *)orders;
    orders = (Order *)(num_orders + 1);

    while (1) {
        int total_orders = *num_orders;
        for (int i = 0; i < total_orders; i++) {
            if (strcmp(orders[i].tipe, "Express") == 0 && strcmp(orders[i].status, "Pending") == 0) {
                char delivering_status[30];
                sprintf(delivering_status, "Delivering by %s", agent_name);
                strcpy(orders[i].status, delivering_status);
                sleep(1); 
                FILE *log_file = fopen("delivery.log", "a");
                if (log_file != NULL) {
                    char timestamp[26];
                    get_timestamp(timestamp);
                    fprintf(log_file, "[%s] [%s] Express package delivered to %s in %s\n",
                            timestamp, agent_name, orders[i].nama, orders[i].alamat);
                    fclose(log_file);
                    sprintf(orders[i].status, "Delivered by %s", agent_name);
                } else {
                    perror("Gagal membuka file log");
                }
            }
        }
        sleep(2);
    }
    if (shmdt(num_orders) == -1) {
        perror("Agent gagal detach");
    }
    pthread_exit(NULL);
}

int main() {
    int shmid;
    pthread_t agent_threads[3];
    AgentArgs agent_args[3];
    char agent_names[3][10] = {"AGENT A", "AGENT B", "AGENT C"};
    int i;

    shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        perror("delivery_agent gagal mengakses");
        return 1;
    }

    for (i = 0; i < 3; i++) {
        strcpy(agent_args[i].agent_name, agent_names[i]);
        agent_args[i].shmid = shmid;
        if (pthread_create(&agent_threads[i], NULL, process_express_orders, &agent_args[i]) != 0) {
            perror("Gagal membuat thread agen");
            return 1;
        }
    }

    while (1) {
        sleep(3600); 
    }
    for (i = 0; i < 3; i++) {
        pthread_join(agent_threads[i], NULL);
    }

    return 0;
}

