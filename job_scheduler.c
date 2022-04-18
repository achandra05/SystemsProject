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

void list_jobs(job *jobs, int n, char *mode)
{
    int i;
    if (jobs != NULL && n != 0)
    {
        if (strcmp(mode, "showjobs") == 0)
        {
            for (i = 0; i < n; ++i)
            {
                if (strcmp(jobs[i].stat, "complete") != 0)
                    printf("Job ID: %d\n"
                           "Command: %s\n"
                           "Status: %s\n\n",
                           jobs[i].jid,
                           jobs[i].cmd,
                           jobs[i].stat);
            }
        }
        else if (strcmp(mode, "submithistory") == 0)
        {
            for (i = 0; i < n; ++i)
            {
                if (strcmp(jobs[i].stat, "complete") == 0)
                    printf("Job ID: %d\n"
                           "Thread ID: %ld\n"
                           "Command: %s\n"
                           "Exit Status: %d\n"
                           "Start Time: %s\n"
                           "Stop Time: %s\n\n",
                           jobs[i].jid,
                           jobs[i].tid,
                           jobs[i].cmd,
                           jobs[i].estat,
                           jobs[i].start,
                           jobs[i].stop);
            }
        }
    }
}

















typedef struct queue
{
    int count;    
    int start;    
    int end;      
    int size;     
    job **buffer; 
    
} queue;




queue *queue_init(int n)
{
    queue *q = malloc(sizeof(queue));
    q->size = n;
    q->buffer = malloc(sizeof(job *) * n);
    q->start = 0;
    q->end = 0;
    q->count = 0;

    return q;
}


int queue_insert(queue *q, job *jp)
{
    if ((q == NULL) || (q->count == q->size))
        return -1;

    q->buffer[q->end % q->size] = jp;
    q->end = (q->end + 1) % q->size;
    ++q->count;

    return q->count;
}





job *queue_delete(queue *q)
{
    if ((q == NULL) || (q->count == 0))
        return (job *)-1;

    job *j = q->buffer[q->start];
    q->start = (q->start + 1) % q->size;
    --q->count;

    return j;
}





void queue_destroy(queue *q)
{
    free(q->buffer);
    free(q);
}














int CONCUR;
int NWORKING;
job JOBS[JOBSLEN];
queue *JOBQ;


void *complete_job(void *arg)
{
    job *jp;
    char **args;
    pid_t pid;
    jp = (job *)arg;
    ++NWORKING;
    jp->stat = "working";
    jp->start = current_datetime_str();
    pid = fork();
    if (pid == 0)
    {
        dup2(open_log(jp->fnout), STDOUT_FILENO);
        dup2(open_log(jp->fnerr), STDERR_FILENO);
        args = get_args(jp->cmd);
        execvp(args[0], args);
        fprintf(stderr, "Error: command execution failed for \"%s\"\n", args[0]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        waitpid(pid, &jp->estat, WUNTRACED);
        jp->stat = "complete";
        jp->stop = current_datetime_str();

        if (!WIFEXITED(jp->estat))
            fprintf(stderr, "Child process %d did not terminate normally!\n", pid);
    }
    else
    {
        fprintf(stderr, "Error: process fork failed\n");
        perror("fork");
        exit(EXIT_FAILURE);
    }

    --NWORKING;
    return NULL;
}





void *complete_jobs(void *arg)
{
    job *jp; /* job pointer */

    NWORKING = 0;
    for (;;)
    {
        if (JOBQ->count > 0 && NWORKING < CONCUR)
        {

            jp = queue_delete(JOBQ);
            pthread_create(&jp->tid, NULL, complete_job, jp);
            pthread_detach(jp->tid);
        }
        sleep(5);
    }
    return NULL;
}



void handle_input()
{
    int i;
    char line[LINELEN];
    char *kw;
    char *cmd;

    printf("Enter `submit COMMAND [ARGS]` to create and run a job.\n"
           "Enter `showjobs` to list all JOBS that are "
           "currently waiting or working.\n"
           "Enter `submithistory` to list all JOBS that were "
           "completed during the current session.\n"
           "Enter `Ctrl+D` or `Ctrl+C` to exit.\n\n");

    i = 0;
    while (printf("> ") && get_line(line, LINELEN) != -1)
    {
        if ((kw = strtok(duplicate(line), " \t\n\r\x0b\x0c")) != NULL)
        {
            if (strcmp(kw, "submit") == 0)
            {
                if (i >= JOBSLEN)
                    printf("Job history full; restart the program to schedule more\n");
                else if (JOBQ->count >= JOBQ->size)
                    printf("Job queue full; try again after more jobs complete\n");
                else
                {
                    cmd = left_strip(strstr(line, "submit") + 6);
                    JOBS[i] = create_job(cmd, i);
                    queue_insert(JOBQ, JOBS + i);
                    printf("Added job %d to the job queue\n", i++);
                }
            }
            else if (strcmp(kw, "showjobs") == 0 ||
                     strcmp(kw, "submithistory") == 0)
					 
                list_jobs(JOBS, i, kw);
        }
    }
    kill(0, SIGINT);
}




int main(int argc, char **argv)
{
    char *fnerr;
    pthread_t tid;

    if (argc != 2)
    {
        printf("Usage: %s CONCURRENCY\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    CONCUR = atoi(argv[1]);
    if (CONCUR < 1)
        CONCUR = 1;
    else if (CONCUR > 8)
        CONCUR = 8;

    printf("Concurrency: %d\n\n", CONCUR);


    fnerr = malloc(sizeof(char) * (strlen(argv[0]) + 5));
    sprintf(fnerr, "%s.err", argv[0]);
    dup2(open_log(fnerr), STDERR_FILENO);

    JOBQ = queue_init(JOBQLEN);


    pthread_create(&tid, NULL, complete_jobs, NULL);


    handle_input();

    exit(EXIT_SUCCESS);
}