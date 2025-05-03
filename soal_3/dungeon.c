#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h> 
#include "shop.h"



#define MAX_WEAPONS 10
int enemy_hp = 0;

int get_random(int min, int max) {
    return rand() % (max - min + 1) + min;
}

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


extern Weapon shop_list[];
extern int buy_weapon(const char*, PlayerStats*, Weapon*, int*);

// player
PlayerStats player = {100, "None", 5, 0, "None"};
Weapon inventory[MAX_WEAPONS];
int inv_count = 0;

void handle_client(int client_fd) {
    CommandType cmd;
    while (recv(client_fd, &cmd, sizeof(cmd), 0) > 0) {
        switch (cmd) {
            case CMD_STATS:
                send(client_fd, &player, sizeof(player), 0);
                break;

            case CMD_VIEW_INVENTORY:
                send(client_fd, &inv_count, sizeof(int), 0);
                send(client_fd, inventory, sizeof(Weapon) * inv_count, 0);
                break;

            case CMD_LIST_WEAPONS:
                send(client_fd, shop_list, sizeof(Weapon) * 5, 0);
                break;

            case CMD_BUY_WEAPON: {
                char weapon_name[32];
                recv(client_fd, weapon_name, sizeof(weapon_name), 0);
                int result = buy_weapon(weapon_name, &player, inventory, &inv_count);
                send(client_fd, &result, sizeof(int), 0);
                break;
            }

            case CMD_EQUIP_WEAPON: {
                char weapon_name[32];
                recv(client_fd, weapon_name, sizeof(weapon_name), 0);
                int found = 0;
                for (int i = 0; i < inv_count; i++) {
                    if (strcmp(inventory[i].name, weapon_name) == 0) {
                        strcpy(player.current_weapon, inventory[i].name);
                        player.base_damage = inventory[i].damage;
                        strcpy(player.passive, inventory[i].passive);
                        found = 1;
                        break;
                    }
                }
                int result = found ? 1 : 0;
                send(client_fd, &result, sizeof(int), 0);
                break;
            }
            case CMD_BATTLE_START:
             enemy_hp = get_random(50, 200);
             send(client_fd, &enemy_hp, sizeof(enemy_hp), 0);
            break;
            
            case CMD_BATTLE_ATTACK: {
                int damage = player.base_damage + get_random(0, 10);
                int critical = get_random(1, 5) == 1;
                if (critical) damage *= 2;
            
                if (strcmp(player.passive, "Burn") == 0 && get_random(1, 4) == 1) {
                    damage += 10;
                }
            
                if (strcmp(player.passive, "Heal") == 0 && get_random(1, 4) == 1) {
                    int bonus = get_random(10, 30);
                    player.gold += bonus;
                }
            
                enemy_hp -= damage;
                if (enemy_hp < 0) enemy_hp = 0;
            
                int reward = 0;
                if (enemy_hp == 0) {
                    reward = get_random(20, 100);
                    player.gold += reward;
                    player.kills++;
                    enemy_hp = get_random(50, 200); //  respawn
                }
            
                int result[3] = {damage, enemy_hp, reward};
                send(client_fd, result, sizeof(result),0);
                break;
            }
            

            case CMD_EXIT:
            printf("Player disconnected.\n");
            close(client_fd);
             return;

        }
    }
}




int main() {
    srand(time(NULL));
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    printf("Dungeon server listening on port 9000...\n");

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        printf("Player connected.\n");
        if (fork() == 0) {
            handle_client(client_fd);
            exit(0);
        }
        close(client_fd);
    }
}
