#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_HUNTER 50
#define MAX_DUNGEON 50

typedef struct {
    int id;
    char name[32];
    char password[32];
    char key[32];
    int level;
    int exp;
    int atk, hp, def;
    int banned;   // 0 = no, 1 = yes
    int notify;   // 0 = off, 1 = on
    int active;   // 0 = inactive, 1 = active (registered)
    int status;   // 0 = logout, 1 = login
} Hunter;

typedef struct {
    int id;
    char name[32];
    char key[32];
    int min_level;
    int atk_reward, hp_reward, def_reward, exp_reward;
    int active; // 0 = not exists, 1 = exists
} Dungeon;

Hunter *hunters;
Dungeon *dungeons;
int shmid_hunter, shmid_dungeon;

void init_shared_memory() {
    key_t key_hunter = ftok("system.c", 'H');
    key_t key_dungeon = ftok("system.c", 'D');

    shmid_hunter = shmget(key_hunter, sizeof(Hunter) * MAX_HUNTER, IPC_CREAT | 0666);
    shmid_dungeon = shmget(key_dungeon, sizeof(Dungeon) * MAX_DUNGEON, IPC_CREAT | 0666);

    hunters = shmat(shmid_hunter, NULL, 0);
    dungeons = shmat(shmid_dungeon, NULL, 0);
}

void list_hunters() {
    printf("\n--- Daftar Hunter ---\n");
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (hunters[i].active) {
            printf("Name: %s | Key: %s | Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d | %s\n",
                hunters[i].name, hunters[i].key, hunters[i].level, hunters[i].exp,
                hunters[i].atk, hunters[i].hp, hunters[i].def,
                hunters[i].banned ? "BANNED" : "ACTIVE");
        }
    }
}

void list_dungeons() {
    printf("\n--- Daftar Dungeon ---\n");
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (dungeons[i].active) {
            printf("Name: %s | Key: %s | MinLevel: %d | ATK: %d | HP: %d | DEF: %d | EXP: %d\n",
                dungeons[i].name, dungeons[i].key, dungeons[i].min_level,
                dungeons[i].atk_reward, dungeons[i].hp_reward,
                dungeons[i].def_reward, dungeons[i].exp_reward);
        }
    }
}

void generate_dungeon() {
    srand(time(NULL));
    char names[][10] = {"Cave", "Ruins", "Tower", "Forest", "Hell"};
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (!dungeons[i].active) {
            dungeons[i].id = i + 1;
            snprintf(dungeons[i].name, 32, "%s_%d", names[rand() % 5], rand() % 100);
            snprintf(dungeons[i].key, 32, "DG%03d", rand() % 1000);
            dungeons[i].min_level = rand() % 5 + 1;
            dungeons[i].atk_reward = rand() % 51 + 100;
            dungeons[i].hp_reward = rand() % 51 + 50;
            dungeons[i].def_reward = rand() % 26 + 25;
            dungeons[i].exp_reward = rand() % 151 + 150;
            dungeons[i].active = 1;
            printf("Dungeon %s dibuat!\n", dungeons[i].name);
            break;
        }
    }
}

void ban_unban_hunter() {
    char key[32];
    printf("Masukkan key hunter: ");
    scanf("%s", key);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (hunters[i].active && strcmp(hunters[i].key, key) == 0) {
            hunters[i].banned = !hunters[i].banned;
            printf("Hunter %s sekarang %s.\n", hunters[i].name,
                   hunters[i].banned ? "BANNED" : "UNBANNED");
            return;
        }
    }
    printf("Hunter tidak ditemukan.\n");
}

void reset_hunter() {
    char key[32];
    printf("Key hunter yang akan di-reset: ");
    scanf("%s", key);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (hunters[i].active && strcmp(hunters[i].key, key) == 0) {
            hunters[i].level = 1;
            hunters[i].exp = 0;
            hunters[i].atk = 10;
            hunters[i].hp = 100;
            hunters[i].def = 5;
            printf("Hunter %s di-reset ke stats awal.\n", hunters[i].name);
            return;
        }
    }
}

void shutdown() {
    shmdt(hunters);
    shmdt(dungeons);
    shmctl(shmid_hunter, IPC_RMID, NULL);
    shmctl(shmid_dungeon, IPC_RMID, NULL);
    printf("Sistem dimatikan dan shared memory dihapus.\n");
    exit(0);
}

int main() {
    init_shared_memory();
    int choice;
    while (1) {
        printf("\n=== ADMIN MENU ===\n");
        printf("1. Lihat semua hunter\n");
        printf("2. Lihat semua dungeon\n");
        printf("3. Tambah dungeon\n");
        printf("4. Ban/unban hunter\n");
        printf("5. Reset hunter\n");
        printf("6. Matikan sistem\n");
        printf("Pilih: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: list_hunters(); break;
            case 2: list_dungeons(); break;
            case 3: generate_dungeon(); break;
            case 4: ban_unban_hunter(); break;
            case 5: reset_hunter(); break;
            case 6: shutdown(); break;
            default: printf("Pilihan salah!\n");
        }
    }
    return 0;
}
