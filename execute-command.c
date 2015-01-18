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

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}


int
command_status (command_t c)
{
  return c->status;
}

void run_command(command_t c, int in, int out);


void
execute_command (command_t c, int profiling)
{
    run_command(c, 0, 1);
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
//void run_until_command(command_t c, int in, int out);
//void run_while_command(command_t c, int in, int out);

void run_command(command_t c, int in, int out)
{
    switch(c->type)
    {
        case UNTIL_COMMAND:
        case WHILE_COMMAND:
            error(1, 0, "haven't implemented this yet, why you trying to execute this foo??\n");
        case IF_COMMAND:
            printf("executing if\n");
            run_if_command(c, in, out); break;
        case PIPE_COMMAND:
            printf("executing pipe\n");
            run_pipe_command(c, in, out); break;
        case SEQUENCE_COMMAND:
            printf("executing sequence\n");
            run_sequence_command(c, in, out); break;
        case SIMPLE_COMMAND:
            printf("executing simple\n");
            run_simple_command(c, in, out); break;
        case SUBSHELL_COMMAND:
            printf("executing subshell\n");
            run_subshell_command(c, in, out); break;
    }
}

void run_if_command(command_t c, int in, int out)
{
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
    printf("exited if with status %d\n", c->status);
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
    printf("exited pipe with status %d\n", c->status);
}

void run_sequence_command(command_t c, int in, int out)
{
    run_command(c->u.command[0], in, 1);
    run_command(c->u.command[1], 0, out);

    c->status = command_status(c->u.command[1]);
    printf("exited sequence with status %d\n", c->status);
}

void run_simple_command(command_t c, int in, int out)
{
    int pid = fork();
    if(pid < 0)
        error(1, 0, "fork error\n");
    if(pid == 0)
    {
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

        execvp(*(c->u.word), c->u.word);
    }
    int result;
    waitpid(pid, &result, NULL);
    c->status = result;
    printf("exited simple with status %d\n", c->status);
}

void run_subshell_command(command_t c, int in, int out)
{
    run_command(c->u.command[0], in, out);
    c->status = command_status(c->u.command[0]);
    printf("exited subshell with status %d\n", c->status);
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF RUN_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////
