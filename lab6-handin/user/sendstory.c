#include <inc/lib.h>
#include <inc/string.h>

#define MAX_PKT_SIZE 1518

void umain(int argc, char **argv)
{
    const char *story[] = {
        "Once upon a time in JOS land,",
        "there was a tiny kernel with big dreams.",
        "It wanted to talk to the outside world...",
        "But it needed a driver to do that!",
        "Luckily, YOU came along and wrote one.",
        "The end."};
    int i;
    for (i = 0; i < sizeof(story) / sizeof(story[0]); i++)
    {
        const char *line = story[i];
        size_t len = strlen(line);

        int res = sys_e1000_transmit((void *)line, len);
        if (res < 0)
            cprintf("Failed to send packet %d: %e\n", i, res);
        else
            cprintf("Sent packet %d: \"%s\"\n", i, line);
    }
}
