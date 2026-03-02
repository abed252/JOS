#include <inc/lib.h>

void umain(int argc, char **argv)
{
    int fd = open("/testuser.txt", O_CREAT | O_WRONLY);
    if (fd < 0)
    {
        cprintf("❌ Failed to open /testuser.txt\n");
        return;
    }

    const char *msg = "Created from user!";
    int n = write(fd, msg, strlen(msg));
    if (n != strlen(msg))
    {
        cprintf("❌ Incomplete write: %d of %d bytes\n", n, strlen(msg));
    }
    else
    {
        cprintf("✅ /testuser.txt written successfully\n");
    }
    
    close(fd);
}
