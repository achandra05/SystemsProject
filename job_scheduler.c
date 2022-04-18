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

char *current_datetime_str()
{
    time_t t = time(NULL);
	int i, c;
    char *copy;
	char *timestring=ctime(&t);

    i = -1;
    copy = malloc(sizeof(char) * strlen(timestring));
    while ((c = timestring[++i]) != '\0' && c != '\n')
        copy[i] = c;
    copy[i] = '\0';
	return copy;
}

char **get_args(char *line)
{
    char *copy = malloc(sizeof(char) * (strlen(line) + 1));
    strcpy(copy, line);

    char *arg;
    char **args = malloc(sizeof(char *));
    int i = 0;
    while ((arg = strtok(copy, " \t")) != NULL)
    {
        args[i] = malloc(sizeof(char) * (strlen(arg) + 1));
        strcpy(args[i], arg);
        args = realloc(args, sizeof(char *) * (++i + 1));
        copy = NULL;
    }
    args[i] = NULL;
    return args;
}


int open_log(char *fn)
{
    int fd;
    if ((fd = open(fn, O_CREAT | O_APPEND | O_WRONLY, 0755)) == -1)
    {
        fprintf(stderr, "Error: failed to open \"%s\"\n", fn);
        perror("open");
        exit(EXIT_FAILURE);
    }
    return fd;
}















typedef struct job
{
    int jid;        
    pthread_t tid;  
    char *cmd;      
    char fnout[10]; 
    char fnerr[10]; 
    char *stat;     
    int estat;      
    char *start;    
    char *stop;    
} job;


job create_job(char *cmd, int jid)
{
    job j;
    j.jid = jid;
    j.cmd = duplicate(cmd);
    j.stat = "waiting";
    j.estat = -1;
    j.start = j.stop = NULL;
    sprintf(j.fnout, "%d.out", j.jid);
    sprintf(j.fnerr, "%d.err", j.jid);
    return j;
}