#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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
    key_t kh = ftok("system.c", 'H');
    key_t kd = ftok("system.c", 'D');
    shmid_hunter = shmget(kh, sizeof(Hunter) * MAX_HUNTER, 0666);
    shmid_dungeon = shmget(kd, sizeof(Dungeon) * MAX_DUNGEON, 0666);
    if (shmid_hunter == -1 || shmid_dungeon == -1) {
        puts("Sistem belum dijalankan.");
        exit(1);
    }
    hunters = shmat(shmid_hunter, NULL, 0);
    dungeons = shmat(shmid_dungeon, NULL, 0);
}

void register_hunter() {
    char name[32], pass[32];
    printf("Nama: "); scanf("%s", name);
    printf("Password: "); scanf("%s", pass);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (strcmp(hunters[i].name, name) == 0) {
            puts("Nama sudah terdaftar.");
            return;
        }
    }
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (hunters[i].id == 0) {
            hunters[i].id = i + 1;
            strcpy(hunters[i].name, name);
            strcpy(hunters[i].password, pass);
            hunters[i].level = 1;
            hunters[i].exp = 0;
            hunters[i].atk = 10;
            hunters[i].hp = 100;
            hunters[i].def = 5;
            hunters[i].banned = 0;
            hunters[i].notif_on = 0;
            hunters[i].status = 0;
            puts("Registrasi berhasil!");
            return;
        }
    }
    puts("Slot penuh.");
}

int login() {
    char name[32], pass[32];
    printf("Nama: "); scanf("%s", name);
    printf("Password: "); scanf("%s", pass);
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (strcmp(hunters[i].name, name) == 0 && strcmp(hunters[i].password, pass) == 0) {
            if (hunters[i].banned) {
                puts("Akun dibanned.");
                return -1;
            }
            hunters[i].status = 1;
            return i;
        }
    }
    puts("Login gagal.");
    return -1;
}

void show_status(Hunter h) {
    printf("Nama: %s | Level: %d | EXP: %d | ATK: %d | HP: %d | DEF: %d\n",
           h.name, h.level, h.exp, h.atk, h.hp, h.def);
}

void raid_dungeon(int idx) {
    int did;
    puts("\n== Dungeon Tersedia ==");
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (dungeons[i].active && dungeons[i].level_min <= hunters[idx].level) {
            printf("[%d] %s (MinLv %d) +%d EXP\n", dungeons[i].id, dungeons[i].name,
                   dungeons[i].level_min, dungeons[i].exp_reward);
        }
    }
    printf("Pilih ID dungeon: ");
    scanf("%d", &did);
    for (int i = 0; i < MAX_DUNGEON; i++) {
        if (dungeons[i].id == did && dungeons[i].active) {
            if (hunters[idx].level < dungeons[i].level_min) {
                puts("Level Anda belum cukup.");
                return;
            }
            puts("Raid berhasil!");
            hunters[idx].atk += dungeons[i].atk_reward;
            hunters[idx].hp += dungeons[i].hp_reward;
            hunters[idx].def += dungeons[i].def_reward;
            hunters[idx].exp += dungeons[i].exp_reward;
            if (hunters[idx].exp >= hunters[idx].level * 100) {
                hunters[idx].level++;
                puts("Level up!");
            }
            return;
        }
    }
    puts("Dungeon tidak valid.");
}

void battle(int idx) {
    int tid;
    puts("== Daftar Lawan ==");
    for (int i = 0; i < MAX_HUNTER; i++) {
        if (i != idx && hunters[i].id > 0) {
            printf("[%d] %s (Lv %d)\n", i, hunters[i].name, hunters[i].level);
        }
    }
    printf("Pilih ID lawan: ");
    scanf("%d", &tid);
    if (tid < 0 || tid >= MAX_HUNTER || tid == idx || hunters[tid].id == 0) {
        puts("Lawan tidak valid.");
        return;
    }
    int my_power = hunters[idx].atk + hunters[idx].def + hunters[idx].hp;
    int enemy_power = hunters[tid].atk + hunters[tid].def + hunters[tid].hp;
    if (my_power >= enemy_power) {
        puts("Kamu menang!");
        hunters[idx].exp += 50;
    } else {
        puts("Kamu kalah...");
    }
}

void main_menu(int idx) {
    while (1) {
        puts("\n== MENU HUNTER ==");
        puts("1. Lihat Status");
        puts("2. Raid Dungeon");
        puts("3. Battle Hunter");
        puts("4. Logout");
        printf("Pilih: ");
        int c;
        scanf("%d", &c);
        if (c == 1) show_status(hunters[idx]);
        else if (c == 2) raid_dungeon(idx);
        else if (c == 3) battle(idx);
        else if (c == 4) return;
        else puts("Pilihan salah.");
    }
}

int main() {
    init_shared_memory();
    while (1) {
        puts("\n== MENU AWAL ==");
        puts("1. Login");
        puts("2. Register");
        puts("3. Keluar");
        int c;
        printf("Pilih: ");
        scanf("%d", &c);
        if (c == 1) {
            int idx = login();
            if (idx >= 0) main_menu(idx);
        } else if (c == 2) register_hunter();
        else if (c == 3) break;
    }
    return 0;
}
