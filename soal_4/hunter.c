// hunter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "shm_common.h"
#include <string.h>
#define MAX_HUNTER 10
#define NAME_LEN 32

typedef struct {
    int active;
    char name[NAME_LEN];
    int level;
    int hp;
    char location[NAME_LEN];
    char activity[NAME_LEN];
} Hunter;

typedef struct {
    Hunter hunters[MAX_HUNTER];
} SharedMemory;


void hunter_menu(struct Hunter *hunter) {
    while (1) {
        printf("\n=== HUNTER SYSTEM ===\n");
        printf("=== %s's MENU ===\n", hunter->username);
        printf("1. List Dungeon\n");
        printf("2. Raid\n");
        printf("3. Battle\n");
        printf("4. Toggle Notification\n");
        printf("5. Exit\n");
        printf("Choice: ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            printf("Belum ada dungeon (placeholder).\n");
        } else if (choice == 2) {
            printf("Raid dimulai... (placeholder).\n");
        } else if (choice == 3) {
            printf("Battle mulai... (placeholder).\n");
        } else if (choice == 4) {
            printf("Notifikasi: (placeholder toggle).\n");
        } else if (choice == 5) {
            printf("Logout dari hunter.\n");
            break;
        } else {
            printf("Pilihan tidak valid.\n");
        }
    }
}

void show_all_hunters(struct SystemData *sys) {
    printf("\n=== HUNTER INFO ===\n");
    if (sys->num_hunters == 0) {
        printf("Belum ada hunter terdaftar.\n");
        return;
    }

    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *h = &sys->hunters[i];
        printf("Name: %s\t", h->username);
        printf("Level: %d\tEXP: %d\t", h->level, h->exp);
        printf("ATK: %d\tHP: %d\tDEF: %d\t", h->atk, h->hp, h->def);
        printf("Status: %s\n", h->banned ? "BANNED" : "Active");
    }
}

int main() {
    key_t system_key = get_system_key();
    int system_shmid = shmget(system_key, sizeof(struct SystemData), 0666);
    if (system_shmid == -1) {
        perror("shmget system");
        return 1;
    }

    struct SystemData *sys = shmat(system_shmid, NULL, 0);
    if (sys == (void *) -1) {
        perror("shmat system");
        return 1;
    }

    while (1) {
        printf("=== HUNTER MENU ===\n");
        printf("1. Register\n2. Login\n3. Exit\nChoice: ");
        int choice;
        scanf("%d", &choice);

        if (choice == 1) {
            if (sys->num_hunters >= MAX_HUNTERS) {
                printf("Hunter penuh!\n");
                continue;
            }

            struct Hunter *h = &sys->hunters[sys->num_hunters];

            printf("Username: ");
            scanf("%s", h->username);

            // Cek duplikat username
            int duplicate = 0;
            for (int i = 0; i < sys->num_hunters; i++) {
                if (strcmp(sys->hunters[i].username, h->username) == 0) {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate) {
                printf("Username sudah terdaftar!\n");
                continue;
            }

            // Set atribut awal
            h->level = 1;
            h->exp = 0;
            h->atk = 10;
            h->hp = 100;
            h->def = 5;
            h->banned = 0;
            h->shm_key = ftok(".", 100 + sys->num_hunters); // key unik untuk hunter

            // Buat shared memory hunter sendiri
            int hunter_shmid = shmget(h->shm_key, sizeof(struct Hunter), IPC_CREAT | 0666);
            if (hunter_shmid == -1) {
                perror("shmget hunter");
                continue;
            }

            struct Hunter *private_hunter = shmat(hunter_shmid, NULL, 0);
            if (private_hunter == (void *) -1) {
                perror("shmat hunter");
                continue;
            }

            memcpy(private_hunter, h, sizeof(struct Hunter)); // salin data

            shmdt(private_hunter);

            sys->num_hunters++;

            printf("Registration success!\n");
        } else if (choice == 2) {
            char username[32];
            printf("Username: ");
            scanf("%s", username);

            int found = -1;
            for (int i = 0; i < sys->num_hunters; i++) {
                if (strcmp(sys->hunters[i].username, username) == 0) {
                    found = i;
                    break;
                }
            }

            if (found == -1) {
                printf("Username tidak ditemukan!\n");
                continue;
            }

            struct Hunter *h = &sys->hunters[found];

            int hunter_shmid = shmget(h->shm_key, sizeof(struct Hunter), 0666);
            if (hunter_shmid == -1) {
                perror("shmget login");
                continue;
            }

            struct Hunter *private_hunter = shmat(hunter_shmid, NULL, 0);
            if (private_hunter == (void *) -1) {
                perror("shmat login");
                continue;
            }

            printf("Login success!\n");
            hunter_menu(private_hunter);
            shmdt(private_hunter);
        } else if (choice == 3) {
            printf("Keluar...\n");
            break;
        } else {
            printf("Pilihan tidak valid.\n");
        }
    }

    shmdt(sys);
    return 0;
}
