#ifndef SHOP_H
#define SHOP_H

#define SHOP_SIZE 5
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

static Weapon shop_list[SHOP_SIZE] = {
    {"DUll Blade", 10, 0, "None"},
    {"Chaos Dagger", 20, 50, "Burn"},
    {"irithyll Axe", 25, 70, "Slow"},
    {"dark Blade", 30, 100, "Crit Boost"},
    {"Divine Hammer", 35, 150, "Heal"}
};

static int buy_weapon(const char* name, PlayerStats *player, Weapon *inventory, int *inv_count) {
    for (int i = 0; i < SHOP_SIZE; i++) {
        if (strcmp(shop_list[i].name, name) == 0) {
            if (player->gold >= shop_list[i].price) {
                player->gold -= shop_list[i].price;
                inventory[*inv_count] = shop_list[i];
                (*inv_count)++;
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}

#endif
