// system.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "shm_common.h"

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

void reset_hunter(struct SystemData *sys) {
    char username[32];
    printf("Masukkan username untuk reset: ");
    scanf("%s", username);

    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *h = &sys->hunters[i];
        if (strcmp(h->username, username) == 0) {
            h->level = 1;
            h->exp = 0;
            h->atk = 10;
            h->hp = 100;
            h->def = 5;
            h->banned = 0;
            printf("Hunter berhasil di-reset!\n");
            return;
        }
    }
    printf("Hunter tidak ditemukan!\n");
}

void ban_hunter(struct SystemData *sys) {
    char username[32];
    printf("Masukkan username yang ingin di-ban: ");
    scanf("%s", username);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].banned = 1;
            printf("Hunter berhasil di-ban!\n");
            return;
        }
    }
    printf("Hunter tidak ditemukan!\n");
}

char *dungeon_names[] = {
    "Double Dungeon", "Demon Castle", "Pyramid Dungeon", "Red Gate Dungeon",
    "Hunters Guild Dungeon", "Busan A-Rank Dungeon", "Insects Dungeon",
    "Goblins Dungeon", "D-Rank Dungeon", "Gwanak Mountain Dungeon",
    "Hapjeong Subway Station Dungeon"
};

void generate_dungeon(struct SystemData *sys) {
    if (sys->num_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon penuh!\n");
        return;
    }

    srand(time(NULL));
    struct Dungeon *d = &sys->dungeons[sys->num_dungeons];
    strcpy(d->name, dungeon_names[rand() % 11]);
    d->min_level = 1 + rand() % 5;
    d->reward_atk = 100 + rand() % 51;
    d->reward_hp = 50 + rand() % 51;
    d->reward_def = 25 + rand() % 26;
    d->reward_exp = 150 + rand() % 151;

    sys->num_dungeons++;

    printf("Dungeon generated!\nName: %s\nMinimum Level: %d\n", d->name, d->min_level);
}

void show_hunters(struct SystemData *sys) {
    printf("\n=== HUNTER INFO ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *h = &sys->hunters[i];
        printf("Name: %s\tLevel: %d\tEXP: %d\tATK: %d\tHP: %d\tDEF: %d\tStatus: %s\n",
            h->username, h->level, h->exp, h->atk, h->hp, h->def,
            h->banned ? "BANNED" : "Active");
    }
}

void show_dungeons(struct SystemData *sys) {
    printf("\n=== DUNGEON INFO ===\n");
    if (sys->num_dungeons == 0) {
        printf("Belum ada dungeon yang dibuat.\n");
        return;
    }

    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon *d = &sys->dungeons[i];
        printf("%d. %s\n", i + 1, d->name);
        printf("   - Minimum Level: %d\n", d->min_level);
        printf("   - Reward ATK: %d\n", d->reward_atk);
        printf("   - Reward HP : %d\n", d->reward_hp);
        printf("   - Reward DEF: %d\n", d->reward_def);
        printf("   - Reward EXP: %d\n\n", d->reward_exp);
    }
}


int main() {
    key_t system_key = get_system_key();
    int system_shmid = shmget(system_key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (system_shmid == -1) {
        perror("shmget");
        return 1;
    }

    struct SystemData *sys = shmat(system_shmid, NULL, 0);
    if (sys == (void *) -1) {
        perror("shmat");
        return 1;
    }

    if (sys->init != 1) {
        sys->num_hunters = 0;
        sys->init = 1;
    }

    while (1) {
        printf("\n=== SYSTEM MENU ===\n");
        printf("1. Hunter Info\n");
        printf("2. Dungeon Info (todo)\n");
        printf("3. Generate Dungeon \n");
        printf("4. Ban Hunter\n");
        printf("5. Reset Hunter\n");
        printf("6. Exit\n");
        printf("Choice: ");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                show_all_hunters(sys);
                break;
            case 2:
                show_dungeons(sys);
                break;
            case 3:
                generate_dungeon(sys);
                break;
            case 4:
                ban_hunter(sys);
                break;
            case 5:
                reset_hunter(sys);
                break;
            case 6:
                printf("Keluar dari sistem.\n");
                shmdt(sys);
                return 0;
            default:
                printf("Pilihan tidak valid.\n");
        }
    }

    shmdt(sys);
    return 0;
}

