#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#define LINELEN 1000
#define JOBSLEN 1000
#define JOBQLEN 100


int get_line(char *s, int n)
{
    int i, c;
    for (i = 0; i < n - 1 && (c = getchar()) != '\n'; ++i)
    {
        if (c == EOF)
            return -1;
        s[i] = c;
    }
    s[i] = '\0';
    return i;
}


char *left_strip(char *s)
{
    int i;

    i = 0;
	
    while (s[i] == ' ' ||
            s[i] == '\t' ||
            s[i] == '\n' ||
            s[i] == '\r' ||
            s[i] == '\x0b' ||
            s[i] == '\x0c')
        ++i;

    return s + i;
}


char *duplicate(char *s)
{
    int i, c;
    char *copy;

    i = -1;
    copy = malloc(sizeof(char) * strlen(s));
    while ((c = s[++i]) != '\0')
        copy[i] = c;
    copy[i] = '\0';

    return copy;
}
