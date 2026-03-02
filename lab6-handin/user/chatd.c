// user/chatd.c
#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <inc/string.h>
#include <inc/stdio.h>
#include <inc/fd.h>
int errno;

#define MAX_CLIENTS 8
static int clients[MAX_CLIENTS];


int num_clients = 0;

void child_send_recive(int sock)
{
    char buffer[256];
    while (1)
    {
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
            buffer[n] = '\0';
            cprintf("📥 [client] Received response: %s\n", buffer);
            // Echo back to the server
            int i;
            for(i = 0 ; i < num_clients; i++)
            {
                if(clients[i] < 0) continue; // Skip invalid sockets
                if(clients[i] != sock) // Don't send back to the same client
                {
                    if (write(clients[i], buffer, n) < 0)
                    {
                        cprintf("❌ [client] Error sending message to client %d\n", clients[i]);
                    }
                    else
                    {
                        cprintf("✍️ [client] Sent message to client %d: %s\n", clients[i], buffer);
                    }
                }
            }
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
    binaryname = "chatd";
    lwip_socket_init();
    cprintf("📡 [server] Starting chat server...\n");

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
        panic("❌ socket failed");
    
    cprintf("🔌 [server] Socket created: fd = %d\n", sock);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        panic("❌ bind failed");

    cprintf("📍 [server] Bound to port 80\n");

    if (listen(sock, 5) < 0)
        panic("❌ listen failed");

    cprintf("👂 [server] Listening for connections...\n");
    char buf[512];
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int pid = 0;
    int h;
    int cs = -1;
    for(h = 0; h < MAX_CLIENTS; h++)
    {
        clients[h] = -1; // Initialize all client sockets to -1
    }
    while (num_clients < 3)
    {
        cs = accept(sock, (struct sockaddr *)&client, &len);
        if (cs >= 0)
        {
            clients[num_clients++] = cs;
            cprintf("✅ [server] New client connected! Total: %d\n", num_clients);
          //  pid = fork();
           // cprintf("👥 [server] Forked a new process for client %d\n", cs);
           // if(pid == 0) break;
        }
    }
    cprintf("🎉 [server] Maximum clients reached (%d). Starting chat...\n", num_clients);
    int i =0;
    for(i = 0; i < num_clients; i++){
        pid = fork();
        if(pid == 0 ){cs = clients[i]; break;}
    }
    i =0;
    for(i = 0; i < num_clients; i++){
        if(clients[i] < 0) continue; // Skip invalid sockets
        cprintf("👥 [server] Client %d connected with socket fd %d\n", i, clients[i]);
    }
    int index = 0;
    //child is the one who read from socket and send signals to parent / parent is the one who write to sockets
   if (pid == 0) {
       child_send_recive(cs);
   } else {
       while (1);
       cprintf("👋 [server] Child process finished. Closing server...\n");
   }
}
