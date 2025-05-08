# LAPORAN PRAKTIKUM SISTEM OPERASI MODUL 3 KELOMPOK IT01

  |       Nama        |     NRP    |
  |-------------------|------------|
  | Ahmad Idza Anafin | 5027241017 |
  | Ivan Syarifuddin  | 5027241045 |
  | Diva Aulia Rosa   | 5027241003 |


# Soal 1
  Membuat implementasi RPC di C menggunakan socket untuk program decode file to image dari input file hexstring.
  ![image](https://github.com/user-attachments/assets/68322035-060e-4d53-a9d3-8fe41d5891bb)
## Server
#### Full Code
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #include <time.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    
    #define PORT 6969
    #define BUFFER_SIZE 6969
    
    
    void daemonize() {
      pid_t pid, sid;
    
      pid = fork();
    
      if (pid < 0)
        exit(EXIT_FAILURE);
      if (pid > 0)
        exit(EXIT_SUCCESS);
    
      umask(0);
    
      sid = setsid();
      if (sid < 0)
        exit(EXIT_FAILURE);
    
      if ((chdir("/home/idzoyy/sisop-it24/Sisop-3-2025-IT01/soal_1/server/")) < 0)
        exit(EXIT_FAILURE);
    
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
    }
    
    void sanitize_filename(char *dest, const char *src, size_t max_len) {
      size_t j = 0;
      for (size_t i = 0; src[i] && j < max_len - 1; i++) {
          if (src[i] == '.' && src[i+1] == '.') {
              i++;
              continue;
          }
          if (src[i] == '/' || src[i] == '\\') continue;
          dest[j++] = src[i];
      }
      dest[j] = '\0';
    }
    
    void parse_buffer(char *buffer, char *command, char *data) {
      char temp[BUFFER_SIZE];
      strncpy(temp, buffer, BUFFER_SIZE);
      temp[BUFFER_SIZE - 1] = '\0';
    
      char *token = strtok(temp, ":");
      if (token != NULL) {
          strcpy(command, token);
    
          token = strtok(NULL, ":");
          if (token != NULL) {
              strcpy(data, token);
          } else {
              strcpy(data, "");
          }
    
      } else {
          strcpy(command, "");
          strcpy(data, "");
      }
    
      command[strcspn(command, "\r\n")] = 0;
      data[strcspn(data, "\r\n")] = 0;
    }
    
    char* readfile(const char* filename, size_t* size_out) {
      FILE* file = fopen(filename, "rb");
      if (!file) return NULL;
    
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      if (size < 0) {
          fclose(file);
          return NULL;
      }
      rewind(file);
    
      unsigned char* buffer = malloc(size);
      if (!buffer) {
          fclose(file);
          return NULL;
      }
    
      size_t read_size = fread(buffer, 1, size, file);
      fclose(file);
    
      if (read_size != (size_t)size) {
          free(buffer);
          return NULL;
      }
    
      char* hex_output = malloc(size * 2 + 1);
      if (!hex_output) {
          free(buffer);
          return NULL;
      }
    
      for (size_t i = 0; i < size; ++i) {
          sprintf(hex_output + i * 2, "%02X", buffer[i]);
      }
    
      hex_output[size * 2] = '\0';
      free(buffer);
    
      if (size_out) *size_out = size * 2;
      return hex_output;
    }
    
    void write_log(const char *source, const char *action, const char *info) {
      FILE *log_file = fopen("server.log", "a");
      if (!log_file) return;
    
      time_t now = time(NULL);
      struct tm *t = localtime(&now);
      char timestamp[32];
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
      fprintf(log_file, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
      fclose(log_file);
    }
    
    
    void handle_ping(int client_fd) {
        write(client_fd, "pong", 4);
    }
    
    
    void handle_download(int client_fd, const char *filename) {
      char path[256];
      char safe[256];
      sanitize_filename(safe, filename, sizeof(safe));
      snprintf(path, sizeof(path), "database/%s", safe);
    
      size_t file_size;
      char* file_content = readfile(path,&file_size);
    
      if (file_content == NULL) {
        write(client_fd,"File not found", 15);
        return;
      }
    
      write(client_fd, file_content, file_size);
      write_log("Client", "DOWNLOAD", filename);
      write_log("Server", "UPLOAD", filename);
      free(file_content);
    
    }
    
    
    void handle_send(int client_fd, char *hex_data) {
      time_t now = time(NULL);
      char filename[256];
      char path[256];
      snprintf(filename, sizeof(filename), "%ld.jpeg", now);
      snprintf(path, sizeof(path), "database/%s", filename);
    
    
      int length = strlen(hex_data);
      int start = 0;
      int end = length - 1;
      char *reversedStr = malloc(length + 1);
      strcpy(reversedStr, hex_data);
    
      while (start < end) {
          char temp = reversedStr[start];
          reversedStr[start] = reversedStr[end];
          reversedStr[end] = temp;
    
          start++;
          end--;
      }
    
      size_t byteArraySize = length / 2;
      unsigned char *byteArray = malloc(byteArraySize);
    
      for (size_t i = 0; i < byteArraySize; i++) {
          sscanf(reversedStr + 2 * i, "%2hhx", &byteArray[i]);
      }
    
      FILE *file = fopen(path, "wb");
      if (file == NULL) {
          write(client_fd,"Failed to send file", 17);
          free(reversedStr);
          free(byteArray);
          return;
      }
    
      fwrite(byteArray, sizeof(unsigned char), byteArraySize, file);
      fclose(file);
      free(reversedStr);
      free(byteArray);
      write_log("Client", "DECODE", "Text data");
      write_log("Server", "SAVE", filename);
      char resp[256];
      snprintf(resp, sizeof(resp), "Text decoded and saved as %s\n", filename);
      write(client_fd, resp, strlen(resp));
    
    }
    
    void handle_invalid(int client_fd) {
        write(client_fd, "Invalid command", 15);
    
    }
    
    
    void run_rpc_server() {
      int server_fd, client_fd;
      struct sockaddr_in server_addr, client_addr;
      socklen_t addr_len;
      char buffer[BUFFER_SIZE];
      char command[BUFFER_SIZE];
      char data[BUFFER_SIZE];
    
      server_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (server_fd == -1) {
        exit(EXIT_FAILURE);
      }
    
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = INADDR_ANY;
      server_addr.sin_port = htons(PORT);
    
      if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        exit(EXIT_FAILURE);
      }
    
      if (listen(server_fd, 5) < 0) {
        close(server_fd);
        exit(EXIT_FAILURE);
      }
    
      while (1) {
        addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            continue;
        }
    
        memset(buffer, 0, BUFFER_SIZE);
        read(client_fd, buffer, BUFFER_SIZE);
    
        memset(command, 0, BUFFER_SIZE);
        memset(data, 0, BUFFER_SIZE);
        parse_buffer(buffer, command, data);
    
        if (strcmp(command, "ping") == 0) {
          handle_ping(client_fd);
    
        } else if (strcmp(command, "download") == 0) {
          handle_download(client_fd, data);
    
        } else if (strcmp(command, "send") == 0) {
          handle_send(client_fd, data);
    
        } else if (strcmp(command, "exit") == 0) {
          close(client_fd);
          write_log("Client", "EXIT", "Client Requested exit");
    
        } else {
          handle_invalid(client_fd);
        }
    
        close(client_fd);
      }
    
      close(server_fd);
    }
    
    int main() {
      daemonize();
    
      if (mkdir("database", 0755) == -1 && errno != EEXIST) {
        perror("mkdir failed");
        exit(EXIT_FAILURE);
      }
    
      FILE *log_file = fopen("server.log", "a");
      if (!log_file) {
          perror("Failed to create/open server.log");
          exit(EXIT_FAILURE);
      }
      fclose(log_file);
    
      run_rpc_server();
      return 0;
    }
#### Fungsi
Program server disini berfungsi untuk memproses request dari user sesuai dengan command yang dikirim user. Format request "command:data" sehingga server akan mengerti harus memproses apa dan apa yang diproses. Server berjalan secara daemon ketika dijalankan dan akan menggenerate log setiap user melakukan request.

- RPC socket server
  
  fungsi utama agar program dapat berkomunikasi dengan client dan menangani request
  ```
  void run_rpc_server() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char data[BUFFER_SIZE];
  
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
      exit(EXIT_FAILURE);
    }
  
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
  
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      close(server_fd);
      exit(EXIT_FAILURE);
    }
  
    if (listen(server_fd, 5) < 0) {
      close(server_fd);
      exit(EXIT_FAILURE);
    }
  
    while (1) {
      addr_len = sizeof(client_addr);
      client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
      if (client_fd < 0) {
          continue;
      }
  
      memset(buffer, 0, BUFFER_SIZE);
      read(client_fd, buffer, BUFFER_SIZE);
  
      memset(command, 0, BUFFER_SIZE);
      memset(data, 0, BUFFER_SIZE);
      parse_buffer(buffer, command, data);
  
      if (strcmp(command, "ping") == 0) {
        handle_ping(client_fd);
  
      } else if (strcmp(command, "download") == 0) {
        handle_download(client_fd, data);
  
      } else if (strcmp(command, "send") == 0) {
        handle_send(client_fd, data);
  
      } else if (strcmp(command, "exit") == 0) {
        close(client_fd);
        write_log("Client", "EXIT", "Client Requested exit");
  
      } else {
        handle_invalid(client_fd);
      }
  
      close(client_fd);
    }
  
    close(server_fd);
  }
  ``` 
- Daemon
  ```
  void daemonize() {
      pid_t pid, sid;
    
      pid = fork();
    
      if (pid < 0)
        exit(EXIT_FAILURE);
      if (pid > 0)
        exit(EXIT_SUCCESS);
    
      umask(0);
    
      sid = setsid();
      if (sid < 0)
        exit(EXIT_FAILURE);
    
      if ((chdir("/home/idzoyy/sisop-it24/Sisop-3-2025-IT01/soal_1/server/")) < 0)
        exit(EXIT_FAILURE);
    
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
    }
  ```
- Logging
  
  akan mencatat setiap aktivitas client/server pada server.log
  ```
  void write_log(const char *source, const char *action, const char *info) {
    FILE *log_file = fopen("server.log", "a");
    if (!log_file) return;
  
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
  
    fprintf(log_file, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
    fclose(log_file);
  }
  ```
  ![image](https://github.com/user-attachments/assets/3b827b69-76db-4092-bf3b-02e57ff3b940)

- Decode
  
  Decode disini karena input diberikan dari soal diberikan hex yang ter-reverse, maka server akan memproses dengan membalik/reverse text kemudian decode hex ke bytearray kemudian disimpan ke file pada directory database/namafile.jpeg
  ```
  void handle_send(int client_fd, char *hex_data) {
    time_t now = time(NULL);
    char filename[256];
    char path[256];
    snprintf(filename, sizeof(filename), "%ld.jpeg", now);
    snprintf(path, sizeof(path), "database/%s", filename);
  
  
    int length = strlen(hex_data);
    int start = 0;
    int end = length - 1;
    char *reversedStr = malloc(length + 1);
    strcpy(reversedStr, hex_data);
  
    while (start < end) {
        char temp = reversedStr[start];
        reversedStr[start] = reversedStr[end];
        reversedStr[end] = temp;
  
        start++;
        end--;
    }
  
    size_t byteArraySize = length / 2;
    unsigned char *byteArray = malloc(byteArraySize);
  
    for (size_t i = 0; i < byteArraySize; i++) {
        sscanf(reversedStr + 2 * i, "%2hhx", &byteArray[i]);
    }
  
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        write(client_fd,"Failed to send file", 17);
        free(reversedStr);
        free(byteArray);
        return;
    }
  
    fwrite(byteArray, sizeof(unsigned char), byteArraySize, file);
    fclose(file);
    free(reversedStr);
    free(byteArray);
    write_log("Client", "DECODE", "Text data");
    write_log("Server", "SAVE", filename);
    char resp[256];
    snprintf(resp, sizeof(resp), "Text decoded and saved as %s\n", filename);
    write(client_fd, resp, strlen(resp));
  
  }
  ```
  ![image](https://github.com/user-attachments/assets/82f19eb9-61de-4455-9674-4538b6c390bf)

- Send file
  
  Untuk send file disini hanya membaca file kemudian mengirimkan kepada client
  ```
  void handle_download(int client_fd, const char *filename) {
    char path[256];
    char safe[256];
    sanitize_filename(safe, filename, sizeof(safe));
    snprintf(path, sizeof(path), "database/%s", safe);
  
    size_t file_size;
    char* file_content = readfile(path,&file_size);
  
    if (file_content == NULL) {
      write(client_fd,"File not found", 15);
      return;
    }
  
    write(client_fd, file_content, file_size);
    write_log("Client", "DOWNLOAD", filename);
    write_log("Server", "UPLOAD", filename);
    free(file_content);
  
  }
  ```
## Client
#### Full Code
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
    
    #define PORT 6969
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
#### Fungsi
Program client disini berfungsi sebagai interface user untuk terhubung ke server dan bisa melakukan pengiriman request untuk tujuan decoding. Terdapat 4 fungsionalitas untuk masing masing jenis request yaitu ping, send image, download image dan exit. 
![image](https://github.com/user-attachments/assets/0f738fd8-7337-4359-9d6a-65d0cab5de5a)

- RPC socket client
  
  fungsi utama untuk berkomunikasi dengan server
  ```
  #define PORT 6969
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
  ``` 
- Ping
  
  akan mengirim request dengan format "ping:null", dengan tujuan command ping ini sebagai test connection
  ```
  send_message(sockfd, "ping:null");
  receive_message(sockfd, buffer);
  ```
  ![image](https://github.com/user-attachments/assets/1cbbb013-9966-4e42-b6c0-87e36f921abc)

- send image
  
  akan mengirim request dengan format "decrypt:data", dimana data adalah isi file yang dibaca
  ```
  snprintf(message, sizeof(message), "send:%s", file_content);
  send_message(sockfd, message);
  receive_message(sockfd, buffer);
  ```
  ![image](https://github.com/user-attachments/assets/2b34e085-2cc3-45fc-b0f3-0910f4db1eaa)

- download image
  
  akan mengirim request dengan format "download:filename", dimana filename adalah dari input user yang ditujukan ke server sebagai file yang akan didownload oleh client
  ```
  snprintf(message, sizeof(message), "download:%s", filename);
  send_message(sockfd, message);
  receive_message(sockfd, buffer);
  ```
  ![image](https://github.com/user-attachments/assets/91035ddb-4a3e-4c16-ae4d-e518dae3ade2)

- exit
  
  akan mengirim request dengan format "exit:null", command ini sebenarnya hanya keluar dari interface tetapi keperluan server log maka perlu mengirim request ke server
  ```
  send_message(sockfd, "exit:null");
  ```
  ![image](https://github.com/user-attachments/assets/cef6b943-e4c2-425c-9b1c-404fa5921555)

  
# Soal 2 
Membuat 2 program untuk pengiriman paket dengan 2 jenis, yaitu paket reguler dan exspres.
 Untuk Paket exspres disimpan dalam delivery_agent.c dan akan dikirim otomatis ketika program dijalankan serta mencatatnya dalam file log (delivery.log).
  Berikut adalah kode programnya :
```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h> 

typedef struct {
    char nama[50];
    char alamat[100];
    char tipe[10];
    char status[30]; 
} Order;

#define SHM_KEY 12345

typedef struct {
    char agent_name[10];
    int shmid;
} AgentArgs;

// fungsi timestamp
void get_timestamp(char *timestamp) {
    time_t timer;
    struct tm *tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(timestamp, 26, "%d/%m/%Y %H:%M:%S", tm_info);
}
// fungsi thread agen
void *process_express_orders(void *arg) {
    AgentArgs *args = (AgentArgs *)arg;
    char *agent_name = args->agent_name;
    int shmid = args->shmid;
    Order *orders;
    int *num_orders;

    orders = (Order *)shmat(shmid, NULL, 0);
    if (orders == (void *)-1) {
        perror("Agent gagal attach");
        pthread_exit(NULL);
    }

    num_orders = (int *)orders;
    orders = (Order *)(num_orders + 1);

    while (1) {
        int total_orders = *num_orders;
        for (int i = 0; i < total_orders; i++) {
            if (strcmp(orders[i].tipe, "Express") == 0 && strcmp(orders[i].status, "Pending") == 0) {
                char delivering_status[30];
                sprintf(delivering_status, "Delivering by %s", agent_name);
                strcpy(orders[i].status, delivering_status);
                sleep(1); 
                FILE *log_file = fopen("delivery.log", "a");
                if (log_file != NULL) {
                    char timestamp[26];
                    get_timestamp(timestamp);
                    fprintf(log_file, "[%s] [%s] Express package delivered to %s in %s\n",
                            timestamp, agent_name, orders[i].nama, orders[i].alamat);
                    fclose(log_file);
                    sprintf(orders[i].status, "Delivered by %s", agent_name);
                } else {
                    perror("Gagal membuka file log");
                }
            }
        }
        sleep(2);
    }
    if (shmdt(num_orders) == -1) {
        perror("Agent gagal detach");
    }
    pthread_exit(NULL);
}

int main() {
    int shmid;
    pthread_t agent_threads[3];
    AgentArgs agent_args[3];
    char agent_names[3][10] = {"AGENT A", "AGENT B", "AGENT C"};
    int i;

    shmid = shmget(SHM_KEY, 0, 0666);
    if (shmid == -1) {
        perror("delivery_agent gagal mengakses");
        return 1;
    }

    for (i = 0; i < 3; i++) {
        strcpy(agent_args[i].agent_name, agent_names[i]);
        agent_args[i].shmid = shmid;
        if (pthread_create(&agent_threads[i], NULL, process_express_orders, &agent_args[i]) != 0) {
            perror("Gagal membuat thread agen");
            return 1;
        }
    }

    while (1) {
        sleep(3600); 
    }
    for (i = 0; i < 3; i++) {
        pthread_join(agent_threads[i], NULL);
    }

    return 0;
}
```
Apabila Kode itu dijalankan dengan
```
./delivery_agent &
```
Maka Paket akan dikirim dengan dokumentasi sebagai berikut:
- Kondisi List sebelum program dijalankan

   ![Screenshot 2025-05-03 162344](https://github.com/user-attachments/assets/44b61da8-28a9-4002-b57e-b81b8d419905)

- Kondisi List setelah program paket exspres dijalankan

  ![Screenshot 2025-05-03 162438](https://github.com/user-attachments/assets/6c54ae8a-f3af-4065-bc28-9442817642a6)

- Kondisi Log saat paket exspres sudah diantar

  ![Screenshot 2025-05-03 162555](https://github.com/user-attachments/assets/ecd759b2-79c5-4081-82c4-000f81035eec)

Kemudian untuk program paket reguler akan dikirim manual oleh user, dan didalam program ini juga bisa untuk mengecek status pengiriman, dan menampilkan list seperti yang diatas tadi. Dan setiap pengiriman ini juga akan dicatat dalam file log yang sama yaitu delivery.log. Untuk kode programnya sebagai berikut :

```
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
```
Setelah kode program itu dijalan kan dengan ./dispatcher maka akan berbagai pilihan yang bisa dilakukan oleh user yaitu mengirim paket, cek status paket, menampilkan list. Dengan dokumentasi sebagai berikut :

- Mengirim beberapa paket secara manual dan mengecek di file log

   ![Screenshot 2025-05-03 162815](https://github.com/user-attachments/assets/a68d0a59-7e63-47a0-88b4-799a63dce0a6)

- Mencoba fitur cek status

  ![Screenshot 2025-05-03 163025](https://github.com/user-attachments/assets/4c0b561f-5df9-44ca-83ce-32f598ace228)

- Menampilkan List yang beberapa paket reguler sudah berubah statusnya

  ![Screenshot 2025-05-03 163103](https://github.com/user-attachments/assets/8a4eebc4-3db7-4e5e-8b20-7905aad31d86)



# Soal 3

## Dungeon
```
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
```

## Player
```
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

```

##### Connection rpc
- Dungeon
```
    srand(time(NULL));
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    printf("Dungeon server listening on port 9000...\n");

```
- Client
```
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Gagal terhubung ke server");
        return 1;
    }
```
#### Sightseeing
main menu
![image](https://github.com/user-attachments/assets/519f035d-0316-4dd4-9284-addbd9c730d4)

#### Status Check
![image](https://github.com/user-attachments/assets/91ca6d7a-3224-4212-bb8b-bea26dded761)

Di player ketika menginputkan "1" akan memunculkan Stat

#### Weapon Shop
![image](https://github.com/user-attachments/assets/b6babb51-28e6-4ade-a5d5-728837275389)

Saat menginputkan "3" akan memunculkan opsi senjata yang ada dari shop.h

## shop.h
```
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

```

#### Handy Inventory
![image](https://github.com/user-attachments/assets/b9e65001-7e71-4756-8895-b85bd5908fd9)

Saat menginputkan "2" akan menampilkan opsi weapon yang dimiliki

#### Battle Mode
![image](https://github.com/user-attachments/assets/98389118-e94a-4db9-818d-5d659711db31)

musuh akan muncul dengan Hp random antara 50-200 point, melakukan attack dengan memasukkan text attack, dan exit untuk menutup battle mode

![image](https://github.com/user-attachments/assets/53875f59-de9c-4e22-b365-00692c291064)

saat musuh berhasil dikalahkan akan memberikan gold dan kembali ke main menu

![image](https://github.com/user-attachments/assets/1d181fa0-cfc9-4617-80fc-c1bc322c931c)

saat equip senjata maka stats akan terupdate menyesuaikan dengan senjata yang kita pakai dan berapa musuh yang sudah kita kalahkan

# Soal 4
Membuat program hunter dan system yang mana menggunakan shared memory, 

#### login hunter
![image](https://github.com/user-attachments/assets/6d1dd812-9607-4cfe-833d-3e54cf3ca1b5)


#### hunter info di system
![image](https://github.com/user-attachments/assets/2c66854c-a4e6-42fd-90eb-6e7908475e20)


#### dungeon info
![image](https://github.com/user-attachments/assets/73016a6b-3809-4440-8399-b2368516e522)
![image](https://github.com/user-attachments/assets/7ef7b646-06a2-4bec-82e5-08b3020bbc42)

#### raid
![image](https://github.com/user-attachments/assets/abf559fa-05c6-47d9-840c-b6f10fa79749)

#### battle
![image](https://github.com/user-attachments/assets/f61945ff-20df-432a-8e12-480948cf3a8a)

#### ban/unban
![image](https://github.com/user-attachments/assets/12e56261-87ef-4f6e-8d1c-93027b64286e)


