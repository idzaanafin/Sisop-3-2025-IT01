#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <time.h>
#include <unistd.h> 

// struktur Order 
typedef struct {
    char nama[50];
    char alamat[100];
    char tipe[10];
    char status[30]; 
} Order;

// key shared memory 
#define SHM_KEY 12345

// timestamp
void get_timestamp(char *timestamp) {
    time_t timer;
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(timestamp, 26, "%d/%m/%Y %H:%M:%S", tm_info);
}

int main(int argc, char *argv[]) {
    FILE *fp;
    char line[256];
    const char *filename = "delivery_order.csv";
    int shmid;
    Order *orders;
    int *num_orders;
    int order_count = 0;
    int initialized = 0;
    int header_found = 0;
    size_t shm_size;

    // akses shared memory
    shmid = shmget(SHM_KEY, 0, 0666);

    // inisialisasi shared memory 
    if (shmid == -1 && argc == 1) {
        fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("Gagal membuka file CSV");
            return 1;
        }
        while (fgets(line, sizeof(line), fp) != NULL) {
            line[strcspn(line, "\n")] = 0;
            if (strstr(line, "nama,alamat,jenis") != NULL) {
                header_found = 1;
            } else if (header_found) {
                order_count++;
            }
        }
        fclose(fp);

        shm_size = sizeof(int) + order_count * sizeof(Order);
        shmid = shmget(SHM_KEY, shm_size, IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("Gagal membuat shared memory");
            return 1;
        }

        orders = (Order *)shmat(shmid, NULL, 0);
        if (orders == (void *)-1) {
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        }
        num_orders = (int *)orders;
        Order *order_data = (Order *)(num_orders + 1);

        fp = fopen(filename, "r");
        if (fp == NULL) {
            perror("Gagal membuka kembali file CSV");
            shmdt(num_orders);
            shmctl(shmid, IPC_RMID, NULL);
            return 1;
        }
        char nama[50], alamat[100], tipe[10];
        int i = 0;
        header_found = 0;
        while (fgets(line, sizeof(line), fp) != NULL && i < order_count) {
            line[strcspn(line, "\n")] = 0;
            if (!header_found && strstr(line, "nama,alamat,jenis") != NULL) {
                header_found = 1;
                continue;
            }
            if (header_found) {
                char *line_copy = strdup(line);
                char *token = strtok(line_copy, ",");
                if (token != NULL) strncpy(order_data[i].nama, token, sizeof(order_data[i].nama) - 1);
                token = strtok(NULL, ",");
                if (token != NULL) strncpy(order_data[i].alamat, token, sizeof(order_data[i].alamat) - 1);
                token = strtok(NULL, ",");
                if (token != NULL) {
                    strncpy(order_data[i].tipe, token, sizeof(order_data[i].tipe) - 1);
                    order_data[i].tipe[sizeof(order_data[i].tipe) - 1] = '\0';
                }
                strcpy(order_data[i].status, "Pending");
                free(line_copy);
                i++;
            }
        }
        fclose(fp);
        *num_orders = order_count;
        initialized = 1;
    } else if (shmid != -1) {
        // jika shared memory sudah ada, attach doang
        orders = (Order *)shmat(shmid, NULL, 0);
        if (orders == (void *)-1) {
            perror("Gagal attach shared memory");
            return 1;
        }
        num_orders = (int *)orders;
        initialized = 1;
    } else {
        fprintf(stderr, "Shared memory belum diinisialisasi.\n");
        return 1;
    }

    if (!initialized) {
        fprintf(stderr, "Gagal inisialisasi atau attach shared memory.\n");
        return 1;
    }
    Order *order_data = (Order *)(num_orders + 1);

    // perintah dari user
    if (argc > 1) {
        if (strcmp(argv[1], "-deliver") == 0) {
            if (argc == 3) {
                char *target_nama = argv[2];
                int delivered = 0;
                for (int i = 0; i < *num_orders; i++) {
                    if (strcmp(order_data[i].nama, target_nama) == 0 && strcmp(order_data[i].tipe, "Reguler") == 0 && strcmp(order_data[i].status, "Pending") == 0) {
                        char timestamp[26];
                        get_timestamp(timestamp);
                        FILE *log_file = fopen("delivery.log", "a");
                        if (log_file != NULL) {
                            fprintf(log_file, "[%s] [AGENT ocaastudy] Reguler package delivered to %s in %s\n",
                                    timestamp, order_data[i].nama, order_data[i].alamat);
                            fclose(log_file);
                            sprintf(order_data[i].status, "Delivered (Reguler)");
                            printf("Paket untuk %s berhasil dikirim (Reguler).\n", target_nama);
                            delivered = 1;
                            break;
                        } else {
                            perror("Gagal membuka file log");
                            break;
                        }
                    }
                }
                if (!delivered) {
                    printf("Tidak ada paket reguler dengan nama '%s' yang belum dikirim.\n", target_nama);
                }
            } else {
                printf("Penggunaan: %s -deliver [Nama]\n", argv[0]);
            }
        } else if (strcmp(argv[1], "-status") == 0) {
            if (argc == 3) {
                char *target_nama = argv[2];
                int found = 0;
                for (int i = 0; i < *num_orders; i++) {
                    if (strcmp(order_data[i].nama, target_nama) == 0) {
                        printf("Status untuk %s: %s\n", target_nama, order_data[i].status);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    printf("Tidak tersedia paket dengan nama '%s'.\n", target_nama);
                }
            } else {
                printf("Penggunaan: %s -status [Nama]\n", argv[0]);
            }
        }else if (strcmp(argv[1], "-list") == 0) {
    	printf("--------------------------------------------------\n");
    	printf("| %-20s | %-25s | %-10s |\n", "Nama", "Status", "Jenis");
    	printf("--------------------------------------------------\n");
    	for (int i = 0; i < *num_orders; i++) {
        	printf("| %-20s | %-25s | %-10s |\n", order_data[i].nama, order_data[i].status, order_data[i].tipe);
   	 }
    	printf("--------------------------------------------------\n");

        } else {
            printf("Gunakan -deliver, -status, atau -list.\n");
        }
    } else {
        printf("Penggunaan: %s [perintah]\n", argv[0]);
	printf("Perintah yang tersedia:\n");
	printf(" -deliver <Nama> : Mencatat pengiriman paket reguler dengan nama tertentu.\n"); 
	printf(" -status <Nama> : Menampilkan status paket dengan nama tertentu.\n"); 
	printf(" -list : Menampilkan daftar semua paket.\n"); 
	printf("\nContoh Penggunaan:\n"); 
	printf(" %s -deliver Budi\n", argv[0]); 
	printf(" %s -status Ayu\n", argv[0]); 
	printf(" %s -list\n", argv[0]);
    }

    // detach shared memory
    if (shmdt(num_orders) == -1) {
        perror("Gagal detach");
        return 1;
    }

    return 0;
}



