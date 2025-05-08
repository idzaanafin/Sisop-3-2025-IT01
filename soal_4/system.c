#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#define MAX_HUNTER 50
#define MAX_DUNGEON 50

typedef struct {
    int id;
    char name[32];
    char password[32];
    int level, exp;
    int atk, hp, def;
    int banned;
    int notif_on;
    int status;
} Hunter;

typedef struct {
    int id;
    char name[32];
    int level_min;
    int atk_reward, hp_reward, def_reward, exp_reward;
    int active;
} Dungeon;

Hunter *hunters;
Dungeon *dungeons;
int shmid_hunter, shmid_dungeon;

void init_shared_memory() {
    key_t key_h = ftok("system.c", 'H');
    key_t key_d = ftok("system.c", 'D');
    shmid_hunter = shmget(key_h, sizeof(Hunter) * MAX_HUNTER, IPC_CREAT | 0666);
    shmid_dungeon = shmget(key_d, sizeof(Dungeon) * MAX_DUNGEON, IPC_CREAT | 0666);
    hunters = (Hunter *)shmat(shmid_hunter, NULL, 0);
    dungeons = (Dungeon *)shmat(shmid_dungeon, NULL, 0);
}

void list_hunters() {
    puts("\n== Daftar Hunter ==");
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (hunters[i].id > 0) {
            printf("[%d] %s | Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d | %s\n",
                   hunters[i].id, hunters[i].name, hunters[i].level, hunters[i].exp,
                   hunters[i].atk, hunters[i].hp, hunters[i].def,
                   hunters[i].banned ? "BANNED" : "ACTIVE");
        }
    }
}

void list_dungeons() {
    puts("\n== Daftar Dungeon ==");
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (dungeons[i].active) {
            printf("[%d] %s | MinLv: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
                   dungeons[i].id, dungeons[i].name, dungeons[i].level_min,
                   dungeons[i].exp_reward, dungeons[i].atk_reward,
                   dungeons[i].hp_reward, dungeons[i].def_reward);
        }
    }
}

void generate_dungeon() {
    srand(time(NULL));
    char *names[] = {"Cave", "Forest", "Hell", "Ruins", "Tower"};
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (!dungeons[i].active) {
            dungeons[i].id = i + 1;
            snprintf(dungeons[i].name, 32, "%s_%d", names[rand() % 5], rand() % 100);
            dungeons[i].level_min = rand() % 5 + 1;
            dungeons[i].atk_reward = rand() % 20 + 5;
            dungeons[i].hp_reward = rand() % 50 + 50;
            dungeons[i].def_reward = rand() % 10 + 5;
            dungeons[i].exp_reward = rand() % 100 + 50;
            dungeons[i].active = 1;
            printf("Dungeon %s berhasil dibuat!\n", dungeons[i].name);
            break;
        }
    }
}

void toggle_ban() {
    char name[32];
    printf("Masukkan nama hunter: ");
    scanf("%s", name);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (strcmp(hunters[i].name, name) == 0) {
            hunters[i].banned = !hunters[i].banned;
            printf("Status hunter %s: %s\n", name, hunters[i].banned ? "BANNED" : "UNBANNED");
            return;
        }
    }
    puts("Hunter tidak ditemukan.");
}

void reset_hunter() {
    char name[32];
    printf("Nama hunter: ");
    scanf("%s", name);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (strcmp(hunters[i].name, name) == 0) {
            hunters[i].level = 1;
            hunters[i].exp = 0;
            hunters[i].atk = 10;
            hunters[i].hp = 100;
            hunters[i].def = 5;
            puts("Stat berhasil di-reset.");
            return;
        }
    }
    puts("Hunter tidak ditemukan.");
}

void shutdown_system() {
    shmdt(hunters);
    shmdt(dungeons);
    shmctl(shmid_hunter, IPC_RMID, NULL);
    shmctl(shmid_dungeon, IPC_RMID, NULL);
    puts("Sistem dimatikan dan shared memory dihapus.");
    exit(0);
}

int main() {
    init_shared_memory();
    while (1) {
        puts("\n=== MENU ADMIN ===");
        puts("1. List Hunter");
        puts("2. List Dungeon");
        puts("3. Tambah Dungeon");
        puts("4. Ban/Unban Hunter");
        puts("5. Reset Hunter");
        puts("6. Shutdown");
        printf("Pilih: ");
        int c;
        scanf("%d", &c);
        switch (c) {
            case 1: list_hunters(); break;
            case 2: list_dungeons(); break;
            case 3: generate_dungeon(); break;
            case 4: toggle_ban(); break;
            case 5: reset_hunter(); break;
            case 6: shutdown_system(); break;
            default: puts("Pilihan salah.");
        }
    }
}
