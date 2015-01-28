// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <error.h>

#include <sys/resource.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  //error (0, 0, "warning: profiling not yet implemented");
  //return -1;
    return open(name, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
}


int
command_status (command_t c)
{
  return c->status;
}

void run_command(command_t c, int in, int out);


int xtrace;
bool command_failed = false;
int proffile;
void
execute_command (command_t c, int profiling, int _xtrace)
{
    proffile = profiling;
    command_failed = false;
    xtrace = _xtrace;

    run_command(c, 0, 1);
    if(command_failed)
        c->status = -1;
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//START OF RUN_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

void run_if_command(command_t c, int in, int out);
void run_pipe_command(command_t c, int in, int out);
void run_sequence_command(command_t c, int in, int out);
void run_simple_command(command_t c, int in, int out);
void run_subshell_command(command_t c, int in, int out);
void run_until_command(command_t c, int in, int out);
void run_while_command(command_t c, int in, int out);

void run_command(command_t c, int in, int out)
{
    //if(command_failed && c->type != SEQUENCE_COMMAND)  // do not execute anything else if the command failed
    //    return;

    switch(c->type)
    {
        case IF_COMMAND:
//            printf("executing if\n");
            run_if_command(c, in, out); break;
        case PIPE_COMMAND:
//            printf("executing pipe\n");
            run_pipe_command(c, in, out); break;
        case SEQUENCE_COMMAND:
//            printf("executing sequence\n");
            run_sequence_command(c, in, out); break;
        case SIMPLE_COMMAND:
//            printf("executing simple\n");
            run_simple_command(c, in, out); break;
        case SUBSHELL_COMMAND:
//            printf("executing subshell\n");
            run_subshell_command(c, in, out); break;
        case UNTIL_COMMAND:
//            printf("executing until\n");
            run_until_command(c, in, out); break;
        case WHILE_COMMAND:
//            printf("executing while\n");
            run_while_command(c, in, out); break;
    }
}

void set_io(command_t c, int *in, int *out)
{
    if(c->input != NULL)
    {
        *in = open(c->input, O_RDONLY);
//        close(*in);
    }

    if(c->output != NULL)
    {
        *out = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
//        close(*out);
    }
}

void run_if_command(command_t c, int in, int out)
{
    set_io(c, &in, &out);
    run_command(c->u.command[0], in, out);
    if(command_status(c->u.command[0]) == 0)
    {
        run_command(c->u.command[1], 0, out);
        c->status = command_status(c->u.command[1]);
    }
    else
    {
        run_command(c->u.command[2], 0, out);
        c->status = command_status(c->u.command[2]);
    }
//    printf("exited if with status %d\n", c->status);
}

void run_pipe_command(command_t c, int in, int out)
{
    int pipefd[2];
    pipe(pipefd);

    run_command(c->u.command[0], in, pipefd[1]);
    close(pipefd[1]);
    
    run_command(c->u.command[1], pipefd[0], out);
    close(pipefd[0]);

    c->status = command_status(c->u.command[1]);
//    printf("exited pipe with status %d\n", c->status);
}

void run_sequence_command(command_t c, int in, int out)
{
    run_command(c->u.command[0], in, 1);

    command_failed = false;
    run_command(c->u.command[1], 0, out);

    c->status = command_status(c->u.command[1]);
//    printf("exited sequence with status %d\n", c->status);
}

double total_user_time = 0;
double total_sys_time = 0;

void run_simple_command(command_t c, int in, int out)
{
    if(command_failed)
        return;
    if(xtrace)
    {
        char **w = c->u.word;
	    printf ("+ %s", *w);
	    while (*++w)
	        printf (" %s", *w);
        if(xtrace == 2)
        {
            int result = getchar();
            if(result != 10)
                error(1, 0, "foo, you typed character(s) >:( no debug for you ... \n");
        }
        else
            printf("\n");
    }
    int pipefd[2];
    pipe(pipefd);

    struct timespec t1;
    clock_gettime(CLOCK_REALTIME, &t1);

    int pid = fork();
    if(pid < 0)
        error(1, 0, "fork error\n");
    if(pid == 0)
    {
        close(pipefd[0]);
        fcntl(pipefd[1], FD_CLOEXEC);
        if(c->input != NULL)
        {
            in = open(c->input, O_RDONLY);
            dup2(in, 0);
            close(in);
        }
        else
            dup2(in, 0);

        if(c->output != NULL)
        {
            out = open(c->output, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(out, 1);
            close(out);
        }
        else
            dup2(out, 1);

        if(strcmp("exec", *(c->u.word)) == 0)
            execvp(*(c->u.word+1), c->u.word+1);
        else
            execvp(*(c->u.word), c->u.word);
        write(pipefd[1], "a", 1);
        exit(-1);
    }
    close(pipefd[1]);
    int result;
    waitpid(pid, &result, 0);

    const int prof_buf_size = 1024;
    int prof_buf_index = 0;
    char prof_buf[prof_buf_size];

    const long long nsec_to_sec = 1000000000;
    const long long usec_to_sec = 1000000;
    
    struct timespec abs_time;
    clock_gettime(CLOCK_REALTIME, &abs_time);

    // record absolute time
    double end_time = (double) abs_time.tv_sec + (double) abs_time.tv_nsec / nsec_to_sec;
    //dprintf(proffile, "%.2f ", end_time);
    prof_buf_index = snprintf(prof_buf, prof_buf_size, "%.2f ", end_time);

    // record total execution time
    double exec_time = (double) (abs_time.tv_sec - t1.tv_sec) + (((double) (abs_time.tv_nsec - t1.tv_nsec)) / nsec_to_sec);
    prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, "%.3f ", exec_time);

    struct rusage usage;
    getrusage(RUSAGE_CHILDREN, &usage);

    // record user cpu time
    prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, "%.3f ", ((double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec / usec_to_sec) - total_user_time);
    total_user_time = (double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec / usec_to_sec;
    // record system cpu time
    prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, "%.3f ", ((double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec / usec_to_sec) - total_sys_time);
    total_sys_time = (double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec / usec_to_sec;

    char buf[5];
    if(read(pipefd[0], buf, 3) != 0)
    {
        command_failed = true;
        //error(1, 0, "cannot find command '%s' ... exiting\n", *(c->u.word));
        fprintf(stderr, "%d: '%s' command not found\n", pid, *(c->u.word));
        if(proffile != -1)
        {
            prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, "[%d]\n", pid);
            dprintf(proffile, "%s\n", prof_buf);
        }
    }
    else if(proffile != -1)
    {
        char **w = c->u.word;
        prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, "%s", *w);
        while(*++w && prof_buf_index < prof_buf_size)
            prof_buf_index += snprintf(prof_buf + prof_buf_index, prof_buf_size - prof_buf_index, " %s", *w);
        dprintf(proffile, "%s\n", prof_buf);
    }
    close(pipefd[0]);
    c->status = WEXITSTATUS(result);
//    printf("exited simple with status %d\n", c->status);
}

void run_subshell_command(command_t c, int in, int out)
{
    set_io(c, &in, &out);
    run_command(c->u.command[0], in, out);
    c->status = command_status(c->u.command[0]);
//    printf("exited subshell with status %d\n", c->status);
}

void run_until_command(command_t c, int in, int out)
{
    set_io(c, &in, &out);
    run_command(c->u.command[0], in, out);
    while(command_status(c->u.command[0]) != 0)
    {
        run_command(c->u.command[1], 0, out);
        run_command(c->u.command[0], in, out);
    }
    c->status = command_status(c->u.command[0]);
//    printf("exited until with status %d\n", c->status);
}

void run_while_command(command_t c, int in, int out)
{
    set_io(c, &in, &out);
    run_command(c->u.command[0], in, out);
    while(command_status(c->u.command[0]) == 0)
    {
        run_command(c->u.command[1], 0, out);
        run_command(c->u.command[0], in, out);
    }
    c->status = command_status(c->u.command[0]);
//    printf("exited while with status %d\n", c->status);
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF RUN_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////
