#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

#define PROXY_PORT 33060
#define DB_PORT 3306
#define DB_HOST "target_database"

void to_upper(char *str, int len) {
    for (int i = 0; i < len; i++) str[i] = toupper(str[i]);
}

void handle_client(int client_sock) {
    int db_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in db_addr = {0};
    db_addr.sin_family = AF_INET;
    db_addr.sin_port = htons(DB_PORT);
    inet_pton(AF_INET, "172.18.0.2", &db_addr.sin_addr);
    
    if (connect(db_sock, (struct sockaddr *)&db_addr, sizeof(db_addr)) < 0) {
        perror("[-] Failed to connect to Target Database");
        close(client_sock);
        return;
    }
    printf("Connected to backend database.\n");

    fd_set read_fds;
    int max_fd = (client_sock > db_sock) ? client_sock : db_sock;
    char buffer[4096];
    char check_buffer[4096];

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_sock, &read_fds);
        FD_SET(db_sock, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) break;
        if (FD_ISSET(client_sock, &read_fds)) {
            int bytes = read(client_sock, buffer, sizeof(buffer) - 1);
            if (bytes <= 0) break;
            
            buffer[bytes] = '\0';
            memcpy(check_buffer, buffer, bytes + 1);
            to_upper(check_buffer, bytes);

            if (strstr(check_buffer, "DROP") != NULL || strstr(check_buffer, "OR 1=1") != NULL) {
                printf("\n[!] BLOCKED: Malicious query intercepted!\n");
                break; 
            }
            write(db_sock, buffer, bytes);
        }
        if (FD_ISSET(db_sock, &read_fds)) {
            int bytes = read(db_sock, buffer, sizeof(buffer));
            if (bytes <= 0) break;
            write(client_sock, buffer, bytes);
        }
    }
    close(client_sock);
    close(db_sock);
    printf("Connection closed.\n");
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PROXY_PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    printf("[*] C Zero-Trust Proxy listening on port %d\n", PROXY_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock >= 0) {
            printf("\nIncoming SQL connection accepted.\n");
            handle_client(client_sock);
        }
    }
    return 0;
}
