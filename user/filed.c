#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/fd.h>
int errno;

struct file{
    char *name;
    int size;
};
#define MAX_FILES 100
char* files[MAX_FILES];
int file_count = 0;

void handle_put(int sock, char *filename)
{
    char buf[1024];
    int fd = open(filename, O_CREAT | O_WRONLY);
    if (fd < 0)
    {
        cprintf("❌ Error opening file %s\n", filename);
        return;
    }
    cprintf("📥 Writing to file: %s\n", filename);

    int n;
    while ((n = read(sock, buf, sizeof(buf))) > 0)
    {
        write(fd, buf, n);
    }

    file_count++;
    cprintf("✅ File %s received and saved. and file_count = %d\n", filename, file_count);
    files[file_count] = filename;

    close(fd);
}

void handle_get(int sock, char *filename)
{
    char buf[1024];
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        cprintf("❌ Error opening file %s for reading\n", filename);
        return;
    }
    cprintf("📤 Sending file: %s\n", filename);

    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        write(sock, buf, n);
    }

    cprintf("✅ File %s sent to client.\n", filename);
    close(fd);
}
void handle_list(int sock)
{
    cprintf("📜 Listing files:\n");
    int i;
    char* line = "Files: \n";
    write(sock, line, strlen(line));
    for (i = 0; i < file_count; i++)
    {
        line = files[i];
        write(sock, line, strlen(line));
        write(sock, "\n", 1);
    }
    write(sock, "List complete.\n", 15);
}

void umain(int argc, char **argv)
{
    int serversock, clientsock;
    struct sockaddr_in server, client;

    lwip_socket_init();
    serversock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(80); // pick any port
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(serversock, (struct sockaddr *)&server, sizeof(server));
    listen(serversock, 5);
    cprintf("🚀 [server] Listening on port 80...\n");

    while (1)
    {
        socklen_t client_len = sizeof(client);
        clientsock = accept(serversock, (struct sockaddr *)&client, &client_len);
        if (clientsock < 0)
            continue;
        cprintf("👋 Client connected.\n");

        // Read command line
        char line[128];
        int i = 0;
        char c;
        while (i < sizeof(line) - 1 && read(clientsock, &c, 1) == 1 && c != '\n')
        {
            line[i++] = c;
        }
        line[i] = '\0';

        // Expect: PUT filename OR GET filename
        if (strncmp(line, "PUT ", 4) == 0)
        {
            char filename[64];
            strncpy(filename, line + 4, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
            handle_put(clientsock, filename);
        }
        else if (strncmp(line, "GET ", 4) == 0)
        {
            char filename[64];
            strncpy(filename, line + 4, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
            handle_get(clientsock, filename);
        }
        else if (strncmp(line, "LIST", 4) == 0)
        {
            handle_list(clientsock);
        }
        else if (strncmp(line, "EXIT", 4) == 0)
        {
            cprintf("👋 Client requested exit.\n");
            break;
        }
        else
        {
            cprintf("❌ Unknown command: %s\n", line);
        }

        close(clientsock);
        cprintf("🔌 Client disconnected.\n");
        break;
    }
}
