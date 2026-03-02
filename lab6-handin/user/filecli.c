#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/fd.h>

int errno;

void send_file(const char *server_ip, const char *filename)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(25002);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cprintf("❌ Connection failed to %s\n", server_ip);
        close(sock);
        return;
    }

    cprintf("🌐 Connected to server %s\n", server_ip);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "PUT %s\n", filename);
    write(sock, cmd, strlen(cmd));

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        cprintf("❌ Failed to open local file %s\n", filename);
        close(sock);
        return;
    }

    char buf[1024];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
    {
        write(sock, buf, n);
    }

    cprintf("✅ File %s sent successfully.\n", filename);
    close(fd);
    close(sock);
}

void get_file(const char *server_ip, const char *filename)
{
    cprintf("🌐 Connecting to server %s to get file %s...\n", server_ip, filename);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(25002);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cprintf("❌ Connection failed to %s\n", server_ip);
        close(sock);
        return;
    }

    cprintf("🌐 Connected to server %s\n", server_ip);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "GET %s\n", filename);
    write(sock, cmd, strlen(cmd));

    char localfile[128];
    snprintf(localfile, sizeof(localfile), "downloaded_%s", filename);
    int fd = open(localfile, O_CREAT | O_WRONLY);
    if (fd < 0)
    {
        cprintf("❌ Failed to create local file %s\n", localfile);
        close(sock);
        return;
    }

    char buf[1024];
    int n;
    while ((n = read(sock, buf, sizeof(buf))) > 0)
    {
        write(fd, buf, n);
    }

    cprintf("✅ File %s received and saved as %s.\n", filename, localfile);
    close(fd);
    close(sock);
}

void get_list(const char *server_ip)
{
    cprintf("🌐 Connecting to server %s to get file list...\n", server_ip);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(25002);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        cprintf("❌ Connection failed to %s\n", server_ip);
        close(sock);
        return;
    }

    cprintf("🌐 Connected to server %s\n", server_ip);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "LIST\n");
    write(sock, cmd, strlen(cmd));

    cprintf("📜 File list from server:\n");
    char buf[1024];
    int n;
    while ((n = read(sock, buf, sizeof(buf) - 1)) > 0)
    {
        buf[n] = '\0';
        cprintf("%s", buf);
    }

    cprintf("📜 End of list.\n");
    close(sock);
}

void umain(int argc, char **argv)
{
    lwip_socket_init();

    if (argc < 3)
    {
        cprintf("Usage: filecli <PUT|GET|LIST> <server_ip> [filename]\n");
        return;
    }

    if (strcmp(argv[1], "PUT") == 0)
    {
        if (argc < 4)
        {
            cprintf("❌ PUT requires filename\n");
            return;
        }
        send_file(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "GET") == 0)
    {
        if (argc < 4)
        {
            cprintf("❌ GET requires filename\n");
            return;
        }
        get_file(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "LIST") == 0)
    {
        get_list(argv[2]);
    }
    else
    {
        cprintf("❌ Unknown command: %s\n", argv[1]);
    }
}
