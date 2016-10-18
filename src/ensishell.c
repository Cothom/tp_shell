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
#include <fcntl.h> //
#include <signal.h> //
#include <string.h> //

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */

#if USE_GUILE == 1
#include <libguile.h>

int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	printf("Not implemented yet: can not execute %s\n", line);

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

struct process_background_linked {
        char* name;
        int pid;
        struct process_background_linked* next;
};

struct process_background_linked* process_background_head = NULL;
struct process_background_linked* process_background_tail = NULL;
      

void add_process_background(char* name, int pid) {
    if (name != NULL) {
        struct process_background_linked* current = (struct process_background_linked*) malloc(sizeof(struct process_background_linked));
        current->name = (char*) malloc(strlen(name) + 1);
        strcpy(current->name, name);
        current->pid = pid;
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
    struct process_background_linked* current = process_background_head;
    struct process_background_linked* prev = NULL;
    while (current != NULL || current->pid != pid) {
        prev = current;
        current = current->next;
    }
    if (current == NULL) return;
    prev->next = current->next;
    free(current->name);
    free(current);
}

void jobs() {
    if (process_background_head != NULL) {
	struct process_background_linked* current = process_background_head;
	while (current != NULL) {
	    printf("Commande : %s, PID :%i \n", current->name, current->pid);
	    current = current->next;
	}
    }
}

/* Fin Modification RANA */
/*
void modify_io(struct cmdline *l) {
    int fd_in, fd_out;
    if (l->in) {
	fd_in = open(l->in, O_RDONLY);
	if (fd_in == -1) {
	    fprintf(stderr, "%s :No such file or directory \n", l->in);
	    exit(-1);   
	}
	dup2(fd_in, 0);
	close(fd_in);
    }
    if (l->out) {
	fd_out = open(l->out, O_RDWR);
	if (fd_out == -1) {
	    fd_out = open(l->out, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH);
	    close(fd_out);
	    fd_out = open(l->out, O_RDWR);
	}
	dup2(fd_out, 1);
	close(fd_out);
    }
}

*/

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


void handler(int sig) {
    //printf("Test Signal : %i \n", sig);
    //int status;
    //wait(&status);
}

int main() {
        printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
        scm_init_guile();
        /* register "executer" function in scheme */
        scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif
	

	/* TEST */
        //struct sigaction act;
	//act.sa_handler = &handler; 
	//sigemptyset(&act.sa_mask);
	//act.sa_flags = 0; // SA_NOCLDSTOP | SA_NOCLDWAIT;
	
	// sigaction(SIGCHLD, &act, 0); 
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
		int i, j;
		char *prompt = "ensishell> ";

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
		
	  
		
		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->bg) printf("background (&)\n");

		int nb_pipe=0;
		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
			    printf("'%s' ", cmd[j]);
			}
			printf("\n");
			if (!strcmp(l->seq[i][0], "jobs")) {
			    jobs();
			    continue;
			}
			if (i!=0) {
			    nb_pipe++;
			}
		}	

		/* Pipe Multiple */ 
		int status;
		if (nb_pipe >= 1) {
		    int fd[2*nb_pipe];
		    for (i=0; i<nb_pipe; i++) {
			pipe(fd + 2*i);
		    }
		    for (i=0; l->seq[i]!=0; i++) {
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
		    
		    for (i=0; i<2*nb_pipe; i++) {                                                                                          
			close(fd[i]);                                                                                                       
		    }                       
		    for (i=0; i<nb_pipe+1; i++) {
			wait(&status);
		    }
		
		/* Fin Pipe Multiple */
		} else {
		    int pid = fork();
		    if (!pid) {
			modify_io(l->in, l->out);
			if (execvp(l->seq[0][0], l->seq[0]) == -1) {
			    fprintf(stderr, "Commande non valide.\n");
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
}



