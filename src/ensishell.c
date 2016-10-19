/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "variante.h"
#include "readcmd.h"

#include <sys/types.h> //
#include <sys/wait.h> //
#include <sys/time.h> //
#include <fcntl.h> //
#include <signal.h> //
#include <string.h> //
#include <libguile.h> //

struct process_background_linked {
    char* name;
    int pid;
    struct timeval start_time;
    struct process_background_linked* next;
};

struct process_background_linked* process_background_head = NULL;
struct process_background_linked* process_background_tail = NULL;

void terminate(char *line);
void add_process_background(char* name, int pid);
void remove_process_background(int pid);
void jobs();
void modify_io(char *in, char *out);
void connect_process(char** cmd_1, char** cmd_2);
void handler(int sig);
int seq_length(struct cmdline *l);
void display_cmdline(struct cmdline *l);
void exec_cmdline(struct cmdline *l);

#ifndef VARIANTE
#error "Variante non défini !!"
#endif
struct sigaction act; //
/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#define USE_GUILE 1
#if USE_GUILE == 1
#include <libguile.h>

int question6_executer(char *line)
{
    /* Question 6: Insert your code to execute the command line
     * identically to the standard execution scheme:
     * parsecmd, then fork+execvp, for a single command.
     * pipe and i/o redirection are not required.
     */
//    printf("Not implemented yet: can not execute %s\n", line);

    struct cmdline *l = parsecmd(&line);
    if (!l) terminate(0);

    display_cmdline(l);
    exec_cmdline(l);

    /* Remove this line when using parsecmd as it will free it */
    free(line);

    return 0;
}

SCM executer_wrapper(SCM x)
{
    return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif


void terminate(char *line) {
#if USE_GNU_READLINE == 1
    /* rl_clear_history() does not exist yet in centOS 6 */
    clear_history();
#endif
    if (line)
        free(line);
    printf("exit\n");
    exit(0);
}

int is_background_process(pid_t pid) {
    struct process_background_linked* current = process_background_head;
    while (current != NULL) {
        if (current->pid == pid) break;
        current = current->next;
    }
    return (current ? 1 : 0);
}

void add_process_background(char* name, int pid) {
    if (name != NULL) {
        struct process_background_linked* current = (struct process_background_linked*) malloc(sizeof(struct process_background_linked));
        current->name = (char*) malloc(strlen(name) + 1);
        strcpy(current->name, name);
        current->pid = pid;
        gettimeofday(&(current->start_time), NULL);
        current->next = NULL;

        if (process_background_head == NULL) {
            process_background_head = current;
            process_background_tail = current;

        } else {
            process_background_tail->next = current;
            process_background_tail = current;
        }

    }
}

void remove_process_background(int pid) {
    printf("pid à supprimer : %d\n", pid);
    struct process_background_linked* current = process_background_head;
    struct process_background_linked* prev = NULL;
    while (current != NULL) {
        if (current->pid == pid) break;
        prev = current;
        current = current->next;
    }
    if (current == NULL) return;
    struct timeval exec_time;
    gettimeofday(&exec_time, NULL);
    exec_time.tv_sec = current->start_time.tv_sec;
    printf("PID : %d\tCommand : %s\t\tExecution time : %d\n", current->pid, current->name, (int) exec_time.tv_sec);
    if (prev) {
        prev->next = current->next;
    } else {
        process_background_head = NULL;
        process_background_tail = NULL;
    }
    free(current->name);
    free(current);
}

void jobs() {
    if (process_background_head != NULL) {
        struct process_background_linked* current = process_background_head;
        while (current != NULL) {
            printf("Command : %s, PID :%i \n", current->name, current->pid);
            current = current->next;
        }
    }
}

void modify_io(char *in, char *out) {
    int fd_in, fd_out;
    if (in) {
        fd_in = open(in, O_RDONLY);
        if (fd_in == -1) {
            fprintf(stderr, "%s :No such file or directory \n", in);
            exit(-1);   
        }
        dup2(fd_in, 0);
        close(fd_in);
    }
    if (out) {
        fd_out = open(out, O_RDWR);
        if (fd_out == -1) {
            fd_out = open(out, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH);
            close(fd_out);
            fd_out = open(out, O_RDWR);
        }
        dup2(fd_out, 1);
        close(fd_out);
    }
}



void connect_process(char** cmd_1, char** cmd_2) {
    int file_descriptors[2];
    pipe(file_descriptors); 
    int pid_cmd1 = fork();
    if (!pid_cmd1) {
        dup2(file_descriptors[1], 1);
        close(file_descriptors[0]);
        close(file_descriptors[1]);
        execvp(cmd_1[0], cmd_1);
    } else {
        int pid_cmd2 = fork();
        if (!pid_cmd2) {
            dup2(file_descriptors[0], 0);
            close(file_descriptors[0]);
            close(file_descriptors[1]);
            execvp(cmd_2[0], cmd_2);
        } else {	
            close(file_descriptors[0]);
            close(file_descriptors[1]);
            int status;
            wait(&status);
            wait(&status);
        }
    }
}


void sigchld_sigaction(int sig, siginfo_t *siginfo, void *useless_arg) {
    if (is_background_process(siginfo->si_pid)) {
        printf("Received signal : %i\tFrom PID : %d\n", sig, siginfo->si_pid);
        remove_process_background(siginfo->si_pid);
        printf("User time : %d\tSystem time : %d\n", (int) siginfo->si_utime, (int) siginfo->si_stime);
    }
    // sigset_t ens2;
    //sigaddset(&ens2, sig);
    //sigprocmask(SIG_SETMASK, &ens1, NULL);
    //int status;
    //wait(&status);
}

int seq_length(struct cmdline *l) {
    int i = 0;
    while (l->seq[i]) i++;
    return i;
}

void display_cmdline(struct cmdline *l) {
    int i, j;
    if (l->err) {
        /* Syntax error, read another command */
        printf("error: %s\n", l->err);
        return;
    }

    if (l->in) printf("in: %s\n", l->in);
    if (l->out) printf("out: %s\n", l->out);
    if (l->bg) printf("background (&)\n");

    /* Display each command of the pipe */
    for (i=0; l->seq[i]!=0; i++) {
        char **cmd = l->seq[i];
        printf("seq[%d]: ", i);
        for (j=0; cmd[j]!=0; j++) {
            printf("'%s' ", cmd[j]);
        }
        printf("\n");
    }
}

void exec_cmdline(struct cmdline *l) {
    if (l->err) {
        printf("error : %s\n", l->err);
        return;
    }

    int nb_pipe = seq_length(l) - 1;

    /* Pipe Multiple */ 
    int i, status;
    if (nb_pipe >= 1) {
        int fd[2*nb_pipe];
        for (i=0; i<nb_pipe; i++) {
            pipe(fd + 2*i);
        }
        for (i = 0; l->seq[i] != 0; i++) {
            int pid=fork();
            if (!pid) {
                if (i == 0) {
                    dup2(fd[1], 1);
                    modify_io(l->in, NULL);
                } else if (i < nb_pipe) {				
                    dup2(fd[2*(i+1)-4], 0);
                    dup2(fd[2*(i+1)-1], 1);
                } else if (i == nb_pipe) {
                    dup2(fd[2*(i+1)-4],0);
                    modify_io(NULL, l->out);
                }
                for (int j=0; j<2*nb_pipe; j++) {
                    close(fd[j]);
                }
                if (execvp(l->seq[i][0], l->seq[i]) == -1) {
                    fprintf(stderr, "Commande '%s' non valide.\n", l->seq[i][0]);
                    exit(-1);
                }    
            }		    
        } 

        for (i = 0; i < 2*nb_pipe; i++)
            close(fd[i]);
        for (i = 0; i < nb_pipe+1; i++)
            wait(&status);

        /* Fin Pipe Multiple */
    } else {
        int pid = fork();
        if (!pid) {
            modify_io(l->in, l->out);
            if (!strncmp(l->seq[0][0], "jobs", 4)) {
                jobs();
                exit(0);
            }
            if (execvp(l->seq[0][0], l->seq[0]) == -1) {
                fprintf(stdout, "Commande non valide.\n");
                exit(-1);
            }
        } else {
            if (!l->bg) {
                int status;
                waitpid(pid, &status, 0);
            } else {
                add_process_background(l->seq[0][0], pid);
            }
        }
    }

}

int main() {
    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
    scm_init_guile();
    /* register "executer" function in scheme */
    scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

    struct sigaction sigchld_act;
    memset(&sigchld_act, '\0', sizeof(struct sigaction));
    sigchld_act.sa_sigaction = sigchld_sigaction;
    sigchld_act.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &sigchld_act, NULL);

    /* TEST */
    //struct sigaction act;
    //act.sa_handler = &handler; 
    //sigemptyset(&act.sa_mask);
    //act.sa_flags = 0; // SA_NOCLDSTOP | SA_NOCLDWAIT;
    //sigaction(SIGCHLD, &act, 0); 
    /*
       char *strsignal(int sig);
       void psignal(int sig, const char *s);
       for (int signum=0; signum<=NSIG; signum++) {
       fprintf (stderr, "(%2d)  : %s\n", signum, strsignal(signum));
       } */
    /* TEST */
    while (1) {
        struct cmdline *l;
        char *line=0;
        char *prompt = "\x1b[31mensishell> \x1b[0m";

        /* Readline use some internal memory structure that
           can not be cleaned at the end of the program. Thus
           one memory leak per command seems unavoidable yet */
        line = readline(prompt);
        if (line == 0 || ! strncmp(line,"exit", 4)) {
            terminate(line);
        }

#if USE_GNU_READLINE == 1
        add_history(line);
#endif


#if USE_GUILE == 1
        /* The line is a scheme command */
        if (line[0] == '(') {
            char catchligne[strlen(line) + 256];
            sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
            scm_eval_string(scm_from_locale_string(catchligne));
            free(line);
            continue;
        }
#endif

        /* parsecmd free line and set it up to 0 */
        l = parsecmd( & line);

        /* If input stream closed, normal termination */
        if (!l) {

            terminate(0);
        }

        /* Affichage Signaux pendants */
        //sigset_t ens2;
        //sigpending(&ens2);
        //printf("Signaux pendants : ");
        //for (int sig=0; sig<=NSIG; sig++) {
        //    if(sigismember(&ens2, sig)) printf("%d ", sig);
        //}
        //printf("\n");
        /* Fin Affichage Signaux pendants */

        display_cmdline(l);
        exec_cmdline(l);
    }			
}




