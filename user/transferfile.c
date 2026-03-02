#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <inc/stdio.h>
#include <inc/string.h>

#define E_BAD_REQ 1000

struct http_request
{
    int sock;
};


static struct http_request dummy_http_request = { .sock = -1 };
struct http_put_request
{
    int sock;
    char *path;
    char *version;
    int content_length;
    char *body;
};

int http_put_request_parse(struct http_put_request *req, char *raw)
{
    const char *p = raw;

    // Check method is PUT
    if (strncmp(p, "PUT ", 4) != 0)
        return -E_BAD_REQ;
    p += 4;

    // Parse path
    const char *path_start = p;
    while (*p && *p != ' ')
        p++;
    int path_len = p - path_start;
    req->path = malloc(path_len + 1);
    memmove(req->path, path_start, path_len);
    req->path[path_len] = '\0';

    // Skip space
    if (*p == ' ')
        p++;

    // Parse HTTP version
    const char *ver_start = p;
    while (*p && *p != '\r' && *p != '\n')
        p++;
    int ver_len = p - ver_start;
    req->version = malloc(ver_len + 1);
    memmove(req->version, ver_start, ver_len);
    req->version[ver_len] = '\0';


    // Search for Content-Length
    req->content_length = -1;

    // Move to start of headers (skip request line)
    while (*p && *p != '\n')
        p++;
    if (*p == '\n')
        p++;

    // Parse headers line-by-line
    while (*p && !(p[0] == '\r' && p[1] == '\n'))
    {
        // Start of line
        const char *line_start = p;

        // Move to end of line
        while (*p && *p != '\n')
            p++;
        int line_len = p - line_start;

        if (line_len >= 15 && strncmp(line_start, "Content-Length:", 15) == 0)
        {
            const char *val = line_start + 15;
            while (*val == ' ' && val < p)
                val++;

            req->content_length = 0;
            while (*val >= '0' && *val <= '9')
            {
                req->content_length = req->content_length * 10 + (*val - '0');
                val++;
            }
        }

        if (*p == '\n')
            p++;
    }

    // Body starts after "\r\n\r\n"
    char *body_start = 0;
    int i;
    for (i = 0; raw[i] && raw[i + 1] && raw[i + 2] && raw[i + 3]; i++)
    {
        if (raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n')
        {
            body_start = &raw[i + 4];
            break;
        }
    }
    if (!body_start)
        return -E_BAD_REQ;
    req->body = body_start;

    return 0;
}

int send_error(struct http_request *req, int code)
{
    const char *msg = "400 Bad Request\r\n";
    write(req->sock, msg, strlen(msg));
    return 0;
}



void umain(int argc, char **argv)
{
    binaryname = "transferfile";

    cprintf("🚀 [server] Starting transferfile server...\n");

    int serversock, clientsock;
    struct sockaddr_in server, client;

    // Step 1: Create TCP socket
    cprintf("🔌 [server] Creating socket...\n");
    serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serversock < 0)
        panic("❌ [server] socket() failed");

    // Step 2: Prepare address
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(80);                // Port number
    server.sin_addr.s_addr = htonl(INADDR_ANY); // Bind to all interfaces

    cprintf("📍 [server] Binding to port 80...\n");

    // Step 3: Bind and Listen
    if (bind(serversock, (struct sockaddr *)&server, sizeof(server)) < 0)
        panic("❌ [server] bind() failed");

    if (listen(serversock, 5) < 0)
        panic("❌ [server] listen() failed");

    cprintf("👂 [server] Now listening on port 80...\n");

    socklen_t client_len = sizeof(client);
    while(1){
    // Step 4: Accept one client
    cprintf("⏳ [server] Waiting to accept a connection...\n");
    clientsock = accept(serversock, (struct sockaddr *)&client, &client_len);
    if (clientsock < 0)
        panic("❌ [server] accept() failed");

    cprintf("🎉 [server] Client connected!\n");

    // Step 5: Read request
    char buffer[1024];
    int n;

    cprintf("📖 [server] Reading request...\n");
    n = read(clientsock, buffer, sizeof(buffer) - 1);
    if (n < 0)
    {
        cprintf("❌ [server] read() failed\n");
        close(clientsock);
        return;
    }

    buffer[n] = '\0';
    cprintf("📥 [server] Request received:\n%s\n", buffer);

    struct http_put_request put_req;
    memset(&put_req, 0, sizeof(put_req));
    put_req.sock = clientsock;

    int r = http_put_request_parse(&put_req, buffer);
    if (r == -E_BAD_REQ)
    {
        send_error(&dummy_http_request, 400); // or write raw 400 response
        return;
    }

    cprintf("✅ PUT path: %s\n", put_req.path);
    cprintf("📏 Content-Length: %d\n", put_req.content_length);
    cprintf("📦 Body: %.*s\n", put_req.content_length, put_req.body);
    if (strcmp(put_req.body, "exit") == 0)
    {
        cprintf("👋 [server] Exiting server...\n");
        close(clientsock);
        close(serversock);
        return;
    }
    // Step 6: Write to file
    char *filename = put_req.path;

    // Strip leading slash if needed
    if (filename[0] == '/')
        filename++;

    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "/%s", filename);
    int fd = open(fullpath, O_CREAT | O_WRONLY);

    if (fd < 0)
    {
        cprintf("❌ Failed to open file %s\n", filename);
        close(clientsock);
        return;
    }

    int total_written = 0;
    int initial = &buffer[n] - put_req.body;
    if (initial > put_req.content_length)
        initial = put_req.content_length;

    // Step 1: Write the body already inside buffer
    int w = write(fd, put_req.body, initial);
    if (w < 0)
    {
        cprintf("❌ Failed to write initial data\n");
        close(fd);
        close(clientsock);
        return;
    }
    total_written += w;

    // Step 2: If not done, read the rest from the socket and write it
    char chunk[512];
    int remaining = put_req.content_length - total_written;

    while (remaining > 0)
    {
        int to_read = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        int r = read(clientsock, chunk, to_read);
        if (r <= 0)
        {
            cprintf("❌ Socket closed or read failed during file body\n");
            break;
        }
        int w = write(fd, chunk, r);
        if (w != r)
        {
            cprintf("❌ Incomplete chunk write: %d of %d\n", w, r);
            break;
        }
        remaining -= r;
        total_written += r;
    }

    if (total_written == put_req.content_length)
    {
        cprintf("✅ File '%s' written successfully (%d bytes)\n", filename, total_written);
    }
    else
    {
        cprintf("❌ File write incomplete (%d of %d bytes)\n", total_written, put_req.content_length);
    }

    close(fd);

    const char *ok_response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    write(clientsock, ok_response, strlen(ok_response));

    // Step 6: Close connection
    close(clientsock);
    cprintf("📴 [server] Client socket closed.\n");
    }


}
