#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// ANSI color codes
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define RESET   "\033[0m"
#define clear_screen() system("clear")

#define PORT 8080
#define LOCALHOST "127.0.0.1"
#define BUFFER_SIZE 6969

int create_connection() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror(RED "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, LOCALHOST, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror(RED "Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

char* readfile(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, size, file) != size) {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

void send_message(int sockfd, const char *message) {
    write(sockfd, message, strlen(message));
}

void receive_message(int sockfd, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
}

void show_menu() {
    printf(CYAN);
    printf(" _______________________________________ \n");
    printf("|                                       |\n");
    printf("|     ██╗████████╗     ██████╗  ██╗     |\n");
    printf("|     ██║╚══██╔══╝    ██╔═══██╗███║     |\n");
    printf("|     ██║   ██║       ██║   ██║╚██║     |\n");
    printf("|     ██║   ██║       ██║   ██║ ██║     |\n");
    printf("|     ██║   ██║       ╚██████╔╝ ██║     |\n");
    printf("|     ╚═╝   ╚═╝        ╚═════╝  ╚═╝     |\n");
    printf("|         Image Decoder Service         |\n");
    printf("|_______________________________________|\n");
    printf(RESET);

    printf(YELLOW "Menu:\n" RESET);
    printf(GREEN " [0] " RESET "Ping server\n");
    printf(GREEN " [1] " RESET "Send image decode request\n");
    printf(GREEN " [2] " RESET "Send download decoded image request\n");
    printf(GREEN " [3] " RESET "Exit\n");
    printf(CYAN "---------------------------------------\n\n" RESET);
}

void handle_ping() {
    int sockfd = create_connection();
    char buffer[BUFFER_SIZE];

    send_message(sockfd, "ping:null");
    receive_message(sockfd, buffer);
    clear_screen();
    show_menu();
    printf("Server Response: %s%s\n\n%s", GREEN, buffer, RESET);

    close(sockfd);
}

void handle_send() {
    int sockfd = create_connection();
    char filename[256];
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    printf("Enter filename to send ➤ ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) return;
    filename[strcspn(filename, "\n")] = 0;

    if (strlen(filename) == 0) {
        clear_screen();
        show_menu();
        printf(RED "Filename cannot be empty.\n\n" RESET);
        close(sockfd);
        return;
    }

    char* file_content = readfile(filename);
    if (!file_content) {
        clear_screen();
        show_menu();
        printf(RED "Failed to read file or file does not exist.\n\n" RESET);
        close(sockfd);
        return;
    }

    snprintf(message, sizeof(message), "send:%s", file_content);
    send_message(sockfd, message);
    receive_message(sockfd, buffer);
    clear_screen();
    show_menu();
    printf("Server Response: %s%s\n\n%s", GREEN, buffer, RESET);

    close(sockfd);
    free(file_content);
}

void handle_download() {
    int sockfd = create_connection();
    char filename[256];
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];

    printf("Enter filename to download ➤ ");
    if (fgets(filename, sizeof(filename), stdin) == NULL) return;
    filename[strcspn(filename, "\n")] = 0;

    if (strlen(filename) == 0) {
        clear_screen();
        show_menu();
        printf(RED "Filename cannot be empty.\n\n" RESET);
        close(sockfd);
        return;
    }

    snprintf(message, sizeof(message), "download:%s", filename);
    send_message(sockfd, message);
    receive_message(sockfd, buffer);
    clear_screen();
    show_menu();

    if (strcmp(buffer, "File not found") == 0) {
        printf("Server Response: %s%s%s\n\n", RED, buffer, RESET);
        close(sockfd);
        return;
    }

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to create file");
        close(sockfd);
        return;
    }

    size_t byteArraySize = strlen(buffer) / 2;
    unsigned char *byteArray = malloc(byteArraySize);
    for (size_t i = 0; i < byteArraySize; i++) {
        sscanf(buffer + 2 * i, "%2hhx", &byteArray[i]);
    }

    fwrite(byteArray, sizeof(unsigned char), byteArraySize, file);
    fclose(file);
    free(byteArray);
    close(sockfd);

    printf("%sDownloaded and saved to %s%s\n\n", GREEN, filename, RESET);
}

void handle_exit() {
    int sockfd = create_connection();
    send_message(sockfd, "exit:null");
    close(sockfd);
    printf(YELLOW "Exiting...\n" RESET);
    exit(EXIT_SUCCESS);
}

int main() {
    clear_screen();
    show_menu();

    char choice;

    while (1) {
        printf("Enter your choice ➤ ");
        choice = getchar();
        getchar(); 

        switch (choice) {
            case '0':
                handle_ping();
                break;
            case '1':
                handle_send();
                break;
            case '2':
                handle_download();
                break;
            case '3':
                handle_exit();
                break;
            default:
                printf(RED "Invalid choice, please try again.\n" RESET);
        }
    }

    return 0;
}
