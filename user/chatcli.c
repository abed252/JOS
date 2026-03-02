// user/chatcli.c
#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <inc/fd.h>
#define IPADDR "172.17.0.1"

int errno;
void send_message(int sock)
{
    int buffer[256];
    char *message = NULL;
    while(1){
    message = readline("write a message: ");
    if (message == NULL || *message == '\0')
    {
        cprintf("❌ [client] Empty input, please type a message\n");
        continue;
    }
    if (write(sock, message, strlen(message)) < 0)
    {
        cprintf("❌ [client] Error sending message\n");
    }
    else
    {
        cprintf("✍️ [client] Sent message: %s\n", message);
    }
    }
}
void receive_message(int sock)
{
    char buffer[256];
    while (1)
    {
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            cprintf("📥 [client] Received response: %s\n", buffer);
        }
        else if (n < 0)
        {
            cprintf("❌ [client] Error receiving message\n");
            break;
        }
    }
}
void umain(int argc, char **argv)
{
    binaryname = "chatcli";
    lwip_socket_init();
    cprintf("📡 [client] Connecting to server...\n");

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        panic("❌ socket failed");

    cprintf("🔌 [client] Socket created: fd = %d\n", sock);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(25002);
    addr.sin_addr.s_addr = inet_addr(IPADDR);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        panic("❌ connect failed");

    cprintf("📍 [client] Connected to server at %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    int pid = fork();
    if(pid == 0) receive_message(sock);
    else send_message(sock);
    close(sock);
    cprintf("👋 [client] Disconnected from server\n");
}
