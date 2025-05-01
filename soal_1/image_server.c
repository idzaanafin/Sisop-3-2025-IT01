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
