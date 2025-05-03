#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_WEAPONS 10

typedef struct {
    char name[32];
    int damage;
    int price;
    char passive[64];
} Weapon;

typedef struct {
    int gold;
    char current_weapon[32];
    int base_damage;
    int kills;
    char passive[64];
} PlayerStats;

typedef enum {
    CMD_STATS,
    CMD_VIEW_INVENTORY,
    CMD_LIST_WEAPONS,
    CMD_BUY_WEAPON,
    CMD_EQUIP_WEAPON,
    CMD_BATTLE_START,
    CMD_BATTLE_ATTACK,
    CMD_EXIT
} CommandType;

void show_menu(int sockfd) {
    int choice;
    while (1) {
        printf("=== Dungeon Menu ===\n");
        printf("1. Show Stats\n2. View Inventory\n3. Weapon Shop\n4. Equip Weapon\n5. Battle Mode\n6. Exit\nChoice: ");

        scanf("%d", &choice);

        CommandType cmd;
        switch (choice) {
            case 1:
                cmd = CMD_STATS;
                send(sockfd, &cmd, sizeof(cmd), 0);
                PlayerStats stats;
                recv(sockfd, &stats, sizeof(stats), 0);
                printf("\n === Player Stats === \n");
                printf("Gold          : %d \n", stats.gold);
                printf("Weapon        : %s \n", stats.current_weapon);
                printf("Base Damage   : %d \n", stats.base_damage);
                printf("Enemies Killed: %d \n", stats.kills);
                printf("Passive       : %s \n", stats.passive);
                break;

            case 2:
                cmd = CMD_VIEW_INVENTORY;
                send(sockfd, &cmd, sizeof(cmd), 0);
                int count;
                recv(sockfd, &count, sizeof(int), 0);
                Weapon weapons[MAX_WEAPONS];
                recv(sockfd, weapons, sizeof(Weapon) * count, 0);
                printf("\nInventory:\n");
                for (int i = 0; i < count; i++) {
                    printf("- %s (DMG: %d, Passive: %s)\n", weapons[i].name, weapons[i].damage, weapons[i].passive);
                }
                break;

            case 3: {
                cmd = CMD_LIST_WEAPONS;
                send(sockfd, &cmd, sizeof(cmd),0);
                Weapon shop[5];
                recv(sockfd, shop, sizeof(shop), 0);
            
                printf("\nShop Weapons:\n");
                for (int i = 0; i < 5; i++) {
                    printf("%d. %s - %dG (DMG: %d, Passive: %s)\n",
                        i+1, shop[i].name, shop[i].price, shop[i].damage, shop[i].passive);
                }
            
                int pilih;
                printf("Pilih senjata untuk dibeli (0 untuk batal): ");
                scanf("%d", &pilih);
                getchar();
            
                if (pilih < 1 || pilih > 5) break;
            
                cmd = CMD_BUY_WEAPON;
                send(sockfd, &cmd, sizeof(cmd), 0);
                send(sockfd, shop[pilih - 1].name, sizeof(shop[pilih - 1].name), 0);
            
                int result;
                recv(sockfd, &result, sizeof(result), 0);
                if (result)
                    printf("Pembelian berhasil!\n");
                else
                    printf("Gagal membeli senjata (mungkin gold tidak cukup).\n");
            
                break;
            }
            

            case 4: {
                cmd = CMD_VIEW_INVENTORY;
                send(sockfd, &cmd, sizeof(cmd),0);
                int count;
                recv(sockfd, &count, sizeof(int), 0);
                Weapon weapons[MAX_WEAPONS];
                recv(sockfd, weapons, sizeof(Weapon) * count, 0);

                if (count == 0) {
                    printf("Inventory kosong!\n");
                    break;
                }

                printf("Pilih senjata yang ingin dipakai: \n");
                for (int i = 0; i < count; i++) {
                    printf("%d. %s (DMG: %d, Passive: %s) \n",
                        i + 1, weapons[i].name, weapons[i].damage, weapons[i].passive);
                }

                int pilih;
                printf("Pilihan (angka): ");
                scanf("%d", &pilih);
                getchar();

                if (pilih < 1 || pilih > count) {
                    printf("Pilihan tidak valid.\n");
                    break;
                }

                cmd = CMD_EQUIP_WEAPON;
                send(sockfd, &cmd, sizeof(cmd), 0);
                send(sockfd, weapons[pilih - 1].name, sizeof(weapons[pilih - 1].name), 0);

                int result;
                recv(sockfd, &result, sizeof(int), 0);
                if (result)
                    printf("Senjata %s telah dipakai!\n", weapons[pilih - 1].name);
                else
                    printf("Gagal memakai senjata.\n");

                break;
            }

            case 5: {
                cmd = CMD_BATTLE_START;
                send(sockfd, &cmd, sizeof(cmd),0);
                int hp;
                recv(sockfd, &hp, sizeof(hp), 0);
            
                printf("\n=== BATTLE START ===\n");
                while (hp > 0) {
                    printf("Enemy HP: [%d] ", hp);
                    for (int i = 0; i < hp / 10; i++) printf("|");
                    printf("\nCommand (attack/exit): ");
            
                    char input[16];
                    scanf("%s", input);
                    if (strcmp(input, "exit") == 0) break;
            
                    cmd = CMD_BATTLE_ATTACK;
                    send(sockfd, &cmd, sizeof(cmd),0);
                    int result[3]; // damage, hp, reward
                    recv(sockfd, result, sizeof(result), 0);
            
                    printf("You dealt %d damage!\n", result[0]);
            
                    if (result[0] > 50) printf("CRITICAL HIT!\n");
                    if (result[2] > 0) {
                        printf("Enemy defeated! You earned %d gold!\n", result[2]);
                        break;
                    }
            
                    hp = result[1];
                }
                break;
            }
            case 6:
                cmd = CMD_EXIT;
            send(sockfd, &cmd, sizeof(cmd),0);
            close(sockfd);
            printf("Keluar dari game.\n");
            return;

            

            default:
                printf("Invalid option!\n");
        }
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Gagal terhubung ke server");
        return 1;
    }

    show_menu(sockfd);
    return 0;
}
