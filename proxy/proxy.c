#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define PROXY_PORT 33060
#define DB_PORT 3306
#define DB_HOST "target_database"
#define RULES_FILE "/app/config/rules.txt"

void to_upper(char *str, int len) {
    for (int i = 0; i < len; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

void send_log(const char* status, const char* ip, const char* query) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct hostent *host = gethostbyname("backend");
    if (host == NULL) return;
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5005);
    memcpy(&servaddr.sin_addr, host->h_addr_list[0], host->h_length);
    
    char json[2048];
    char safe_query[1024];
    int j = 0;
    for(int i = 0; query[i] != '\0' && i < 1023; i++) {
        if(query[i] == '"' || query[i] == '\\' || query[i] < 32) continue;
        safe_query[j++] = query[i];
    }
    safe_query[j] = '\0';

    snprintf(json, sizeof(json), "{\"status\":\"%s\", \"ip\":\"%s\", \"query\":\"%s\"}", status, ip, safe_query);
    sendto(sock, json, strlen(json), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
    close(sock);
}

int check_threats(const char *buffer) {
    FILE *file = fopen(RULES_FILE, "r");
    if (!file) return 0; 
    char rule[256];
    while (fgets(rule, sizeof(rule), file)) {
        rule[strcspn(rule, "\r\n")] = 0; 
        if (strlen(rule) > 0 && strstr(buffer, rule) != NULL) {
            fclose(file);
            return 1; 
        }
    }
    fclose(file);
    return 0; // Safe
}

int main() {
    int server_sock, client_sock, db_sock;
    struct sockaddr_in server_addr, client_addr, db_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[4096];
    char check_buffer[4096];

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PROXY_PORT);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 10);
    
    printf("[*] Zero-Trust Proxy listening on port %d\n", PROXY_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        char *client_ip = inet_ntoa(client_addr.sin_addr);
        
        struct hostent *db_host = gethostbyname(DB_HOST);
        db_sock = socket(AF_INET, SOCK_STREAM, 0);
        
        memset(&db_addr, 0, sizeof(db_addr));
        db_addr.sin_family = AF_INET;
        db_addr.sin_port = htons(DB_PORT);
        memcpy(&db_addr.sin_addr, db_host->h_addr_list[0], db_host->h_length);
        
        connect(db_sock, (struct sockaddr*)&db_addr, sizeof(db_addr));

        while (1) {
            int bytes = read(client_sock, buffer, sizeof(buffer) - 1);
            if (bytes <= 0) break;
            
            buffer[bytes] = '\0';
            memcpy(check_buffer, buffer, bytes + 1);
            to_upper(check_buffer, bytes);

            // TCP Packets under 5 bytes are usually just handshake/auth pings, ignore in logs
            if (bytes > 5) {
                if (check_threats(check_buffer)) {
                    printf("[!] BLOCKED IP: %s\n", client_ip);
                    send_log("BLOCKED", client_ip, check_buffer);
                    break; // Sever TCP connection
                } else {
                    send_log("ALLOWED", client_ip, check_buffer);
                }
            }
            write(db_sock, buffer, bytes);
            bytes = read(db_sock, buffer, sizeof(buffer));
            if (bytes > 0) write(client_sock, buffer, bytes);
        }
        close(client_sock);
        close(db_sock);
    }
    return 0;
}
