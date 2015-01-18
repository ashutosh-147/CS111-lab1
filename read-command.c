// UCLA CS 111 Lab 1 command reading

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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "alloc.h"

//#include <stdio.h>  // only for testing


/////////////////////////////////////////////////
/////////////////////////////////////////////////
//STACK IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////
enum branch_word
{
  DO,
  DONE,
  THEN,
  ELSE_OR_FI,
  FI,
  SUBSHELL,
  ERROR,
};

struct stack_t
{
  enum branch_word * stack;
  int max_size;
  int head;
} my_stack;

void init_stack()
{
  my_stack.stack = checked_malloc(sizeof(enum branch_word));
  my_stack.max_size = 1;
  my_stack.head = 0;
}

bool stack_is_empty()
{
    return my_stack.head == 0;
}

enum branch_word peek()
{
  if(my_stack.head < 0)
    return ERROR;
  return my_stack.stack[my_stack.head-1];
}

void push(enum branch_word obj)
{
  if(my_stack.head == my_stack.max_size)
    {
      my_stack.max_size *= 2;
      my_stack.stack = checked_realloc(my_stack.stack, my_stack.max_size * sizeof(enum branch_word));
    }
  my_stack.stack[my_stack.head++] = obj;
}

void pop(char *input)
{
  switch(peek())
    {            
    case DO:
      my_stack.stack[my_stack.head-1] = DONE;
      return;
    case THEN:
      my_stack.stack[my_stack.head-1] = ELSE_OR_FI;
      return;        
    case ELSE_OR_FI:
        if(strcmp("else", input) == 0)
        {
            my_stack.stack[my_stack.head-1] = FI;
            return;
        }
    default: //DONE and FI and SUBSHELL
      my_stack.head--;
      return;
    }
}

bool word_on_stack(char *str)
{
    enum branch_word bw = peek();
    if(strcmp("do",str) == 0)
    {
        return bw == DO;
    }
    else if(strcmp("done",str) == 0)
    {
        return bw == DONE;
    }
    else if(strcmp("then",str) == 0)
    {
        return bw == THEN;
    }
    else if(strcmp("else",str) == 0)
    {
        return bw == ELSE_OR_FI;
    }
    else if(strcmp("fi",str) == 0)
    {
        return bw == ELSE_OR_FI || bw == FI;
    }
    else if(strcmp(")",str) == 0)
    {
        return bw == SUBSHELL;
    }
    else
    {
        return false;
    }
}
/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF STACK IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//GET_NEXT_WORD IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

void append_char(char **buf, char c, size_t *size, size_t *max_size)
{
    if(*size == *max_size)
    {
        *buf = checked_grow_alloc(*buf, max_size);
    }
    buf[0][(*size)++] = c;
}

bool isEmptyString(char *buf)
{
    return *buf == '\0';
}

const char acceptable[] = "!%+,-./:@^_";
bool isAcceptable(char c)
{
    if(isalpha(c))
        return true;
    else if(isdigit(c))
        return true;
    else
    {
        int i;
        for(i = 0; i < 11; i++)
            if(c == acceptable[i])
                return true;
        return false;
    }
}

enum end_of_word
{
    END_OF_FILE,
    END_OF_LINE,
    CHAIN_COMMAND,
    MORE_ARGS,
    SS_COMMAND,
};

// using in get_next_word
//      modified by check_future_char
char lastChar = '\0';
unsigned int current_line = 1;

enum end_of_word check_future_char(int (*get_next_byte) (void *), void *get_next_byte_argument)
{
    int nb;
    char nb_c;
    do
    {
        nb = get_next_byte(get_next_byte_argument);
        nb_c = (char) nb;
    } while(nb_c == ' ' || nb_c == '\t');
    if(nb == -1)
        return END_OF_FILE;
    lastChar = nb_c;
    switch(nb_c)
    {
        case '|':
        case ';':
        case '<':
        case '>':
            return CHAIN_COMMAND;
        case '(':
        case ')':
            return SS_COMMAND;
        case '\n':
            return END_OF_LINE;
        case '#':
            do
            {
                nb = get_next_byte(get_next_byte_argument);
                if(nb == -1)
                    return END_OF_FILE;
            } while((char) nb != '\n');
            lastChar = '\n';
            return END_OF_LINE;
        default:
            if(!isAcceptable(nb_c))
                error(1, 0, "%d: bad character found ... exiting\n", current_line);
            return MORE_ARGS;
    }
}

enum end_of_word get_next_word(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument)
{
    int nb;
    *buf_size = 0;

    if(lastChar != '\0')
    {
        switch(lastChar)
        {
            case '|':
            case ';':
            case '<':
            case '>':
            case '(':
            case ')':
                append_char(buf, lastChar, buf_size, max_size);
                append_char(buf, '\0', buf_size, max_size);
                lastChar = '\0';
                return check_future_char(get_next_byte, get_next_byte_argument);
            case '\n':
                lastChar = '\0';
                return get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
            default:
                append_char(buf, lastChar, buf_size, max_size);
                lastChar = '\0';
        }
            
    }

    for(;;)
    {
        nb = get_next_byte(get_next_byte_argument);
        if(nb == -1)
        {  // end of file
            append_char(buf, '\0', buf_size, max_size);
            return END_OF_FILE;
        }
        switch((char) nb)
        {
            case '\n':
                current_line++;
                append_char(buf, '\0', buf_size, max_size);
                return END_OF_LINE;
            case '(':
                if(*buf_size != 0)
                    error(1, 0, "%d: cannot have '(' in argument ... exiting\n", current_line);
            case ')':
                append_char(buf, '\0', buf_size, max_size);
                lastChar = (char) nb;
                return SS_COMMAND;
            case '|':
            case '<':
            case '>':
            case ';':
                append_char(buf, '\0', buf_size, max_size);
                lastChar = (char) nb;
                return CHAIN_COMMAND;
            case '#':
                if(*buf_size != 0)
                    error(1, 0, "%d: '#' must stand alone ... exiting\n", current_line);
                do
                {
                    nb = get_next_byte(get_next_byte_argument);
                    if(nb == -1)
                        return END_OF_FILE;
                } while((char) nb != '\n');
                append_char(buf, '\0', buf_size, max_size);
                lastChar = '\n';
                return END_OF_LINE;
            case ' ':
            case '\t':
                if(*buf_size == 0)
                    return get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
                append_char(buf, '\0', buf_size, max_size);
                while((char) nb == ' ' || (char) nb == '\t')
                    nb = get_next_byte(get_next_byte_argument);
                if(nb == -1)
                    return END_OF_FILE;
                lastChar = (char) nb;
                switch((char) nb)
                {
                    case '(':
                        return SS_COMMAND;
                    case '|':
                    case '<':
                    case '>':
                    case ';':
                        return CHAIN_COMMAND;
                    case '\n':
                        return END_OF_LINE;
                    default:
                        if(isAcceptable((char) nb))
                            return MORE_ARGS;
                        else
                        {
                            error(1, 0, "%d: bad character found ... exiting\n", current_line);
                        }
                }
            default:
                if(isAcceptable((char) nb))
                    append_char(buf, (char) nb, buf_size, max_size);
                else
                    error(1, 0, "%d: bad character found ... exiting\n", current_line);
        }
    }
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF GET_NEXT_WORD IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//COMMAND STREAM IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

struct command_stream
{
    command_t *stream;
    size_t current_read;
    size_t current_write;
    size_t max_size;
};

command_stream_t init_command_stream()
{
    command_stream_t cs = checked_malloc(sizeof(struct command_stream));
    cs->stream = checked_malloc(sizeof(command_t));
    cs->current_read = 0;
    cs->current_write = 0;
    cs->max_size = 1;
    return cs;
}

void add_command(command_stream_t cs, command_t c)
{
    if(cs->current_write == cs->max_size)
    {
        cs->max_size *= 2;
        cs->stream = checked_realloc(cs->stream, cs->max_size * sizeof(command_t));
    }
    cs->stream[cs->current_write++] = c;
}

// function used for reading command stream
command_t get_command(command_stream_t cs)
{
    if(cs->current_read == cs->current_write)
        return NULL;
    return cs->stream[cs->current_read++];
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF COMMAND STREAM IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//CREATE_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

command_t create_pipe_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command);
command_t create_sequence_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command);
command_t add_io_redirection(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command, bool isInput);
command_t create_if_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof);
command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool isWhile);
command_t create_subshell_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof);
command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, enum end_of_word first_word_status);

bool invalid_start_of_command(char ** buf)
{
    return strcmp(")", *buf) == 0 ||
           strcmp("|", *buf) == 0 || strcmp(";", *buf) == 0 ||
           strcmp("<", *buf) == 0 || strcmp(">", *buf) == 0 ||
           strcmp("do", *buf) == 0 || strcmp("done", *buf) == 0 || 
           strcmp("then", *buf) == 0 || strcmp("else", *buf) == 0 || strcmp("fi", *buf) == 0;
}

command_t create_command_based_on_buf(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, enum end_of_word end)
{
    if(isEmptyString(*buf))
    {
        return NULL;
    }
    if(strcmp("if", *buf) == 0)
    {
        return create_if_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof);
    }
    else if(strcmp("(", *buf) == 0)
    {
        return create_subshell_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof);
    }
    else if(strcmp("until", *buf) == 0)
    {
        return create_while_or_until_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);
    }
    else if(strcmp("while", *buf) == 0)
    {
        return create_while_or_until_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, true);
    }
    else if(invalid_start_of_command(buf))
    {
        error(1, 0, "%d: cannot create a new command starting with '%s' ... exiting\n", current_line, *buf);
        return NULL;
    }
    else
    {
        return create_simple_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, end);
	}

}

command_t create_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool check_stack)
{
    enum end_of_word end;
    
    do
    {
        end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        switch(end)
        {
            case END_OF_FILE:
                *eof = true;
                return NULL;
            case SS_COMMAND:
            case END_OF_LINE:
            case CHAIN_COMMAND:
            case MORE_ARGS:
                break;
        }        
    }
    while(isEmptyString(*buf));

    if(end == END_OF_FILE)
    {
        *eof = true;
        return NULL;
    }
    if(check_stack)
    {
        if(word_on_stack(*buf))
        {
            if((strcmp("done", *buf) == 0 || strcmp("fi", *buf) == 0) && end == MORE_ARGS)
                error(1, 0, "%d: cannot have additional commands directly after '%s'\n", current_line, *buf);
            pop(*buf);
            return NULL;
        }
    }

    return create_command_based_on_buf(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, end);
}

// creates command for | ; < >
command_t create_chain_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command)
{
    enum end_of_word eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
    
    if(strcmp(";", *buf) == 0)
    {
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
            case SS_COMMAND:
                return first_command;
            case CHAIN_COMMAND:
                error(1, 0, "%d: cannot have consecutive chain commands ... exiting\n", current_line);
                return first_command;
            case MORE_ARGS:
                break;
        }

        return create_sequence_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, first_command);
    }

    switch(eow)
    {
        case END_OF_FILE:
            *eof = true;
        case END_OF_LINE:
        case CHAIN_COMMAND:
            error(1, 0, "%d: cannot have consecutive chain commands ... exiting\n", current_line);
            return first_command;
        case MORE_ARGS:
        case SS_COMMAND:
            break;
    }
    
    if(strcmp("|", *buf) == 0)
    {
        return create_pipe_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, first_command);
    }
    else if(strcmp("<", *buf) == 0)
    {
        return add_io_redirection(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, first_command, true);
    }
    else
    {
        return add_io_redirection(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, first_command, false);
    }
}

command_t create_pipe_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = PIPE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);

    if(second_command == NULL)
    {
        error(1, 0, "%d: must have second command for pipe ... exiting\n", current_line);
        return first_command;
    }
    else
    {
        com->u.command[0] = first_command;
        com->u.command[1] = second_command;
        return com;
    }
}

command_t create_sequence_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SEQUENCE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, true);
    
    if(second_command == NULL)
    {
        return first_command;
    }
    else
    {
        com->u.command[0] = first_command;
        com->u.command[1] = second_command;
        return com;
    }
}

command_t create_sequence_after_loop(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command, enum end_of_word end)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SEQUENCE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command_based_on_buf(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, end);
    
    if(second_command == NULL)
    {
        return first_command;
    }
    else
    {
        com->u.command[0] = first_command;
        com->u.command[1] = second_command;
        return com;
    }
}

command_t add_io_redirection(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, command_t first_command, bool isInput)
{
    enum end_of_word eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);

    switch(eow)
    {
        case END_OF_FILE:
            *eof = true;
        case SS_COMMAND:
            error(1, 0, "%d: cannot redirect I/O into or out of subshell ... exiting\n", current_line);
        case END_OF_LINE:
        case CHAIN_COMMAND:
            if(isEmptyString(*buf))
            {
                error(1, 0, "%d: cannot have chain command after io redirection ... exiting\n", current_line);
                return first_command;
            }
        case MORE_ARGS:
            break;
    }

    if(isInput)
    {
        if(first_command->input != NULL || first_command->output != NULL)
        {
            error(1, 0, "%d: must define input first - Cannot define input twice ... exiting\n", current_line);
            return first_command;
        }
        first_command->input = checked_malloc(*buf_size);
        strcpy(first_command->input, *buf);
    }
    else
    {
        if(first_command->output != NULL)
        {
            error(1, 0, "%d: cannot define output twice ... exiting\n", current_line);
            return first_command;
        }
        first_command->output = checked_malloc(*buf_size);
        strcpy(first_command->output, *buf);
    }

    if(eow == CHAIN_COMMAND)
    {
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, first_command);
    }
    return first_command;
}

command_t create_if_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof)
{
    enum end_of_word eow;

    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = IF_COMMAND;
    com->input = NULL;
    com->output = NULL;
    com->u.command[0] = NULL;
    com->u.command[1] = NULL;
    com->u.command[2] = NULL;

    push(THEN);
    com->u.command[0] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);

    if(*eof)
    {
        return NULL;
    }

    while(peek() == THEN)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case CHAIN_COMMAND:
                error(1, 0, "%d: cannot define chain command after 'then' ... exiting\n", current_line);
                return NULL;
            case END_OF_LINE:
            case MORE_ARGS:
            case SS_COMMAND:
                if(word_on_stack(*buf))
                {
                    pop(*buf);
                }
                else
                {
                    com->u.command[0] = create_sequence_after_loop(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com->u.command[0], eow);
                    if(*eof)
                        return NULL;
                }
                break; 
        }
    }

    com->u.command[1] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);

    if(*eof)
    {
        return NULL;
    }

    while(peek() == ELSE_OR_FI)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case CHAIN_COMMAND:
                error(1, 0, "%d: cannot define chain command after 'else' ... exiting\n", current_line);
                return NULL;
            case END_OF_LINE:
            case MORE_ARGS:
            case SS_COMMAND:
                if(word_on_stack(*buf))
                {
                    pop(*buf);
                }
                else
                {
                    com->u.command[1] = create_sequence_after_loop(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com->u.command[1], eow);
                    if(*eof)
                        return NULL;
                }
                break; 
        }
    }
    
    if(peek() == FI)
    {
        com->u.command[2] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);
    }
    
    while(peek() == FI)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
            case CHAIN_COMMAND:
            case MORE_ARGS:
            case SS_COMMAND:
                if(word_on_stack(*buf))
                {
                    pop(*buf);
                }
                else
                {
                    com->u.command[2] = create_sequence_after_loop(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com->u.command[2], eow);
                    if(*eof)
                        return NULL;
                }
                break; 
        }
    }
    if(eow == CHAIN_COMMAND)
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com);
    return com;
}

command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool isWhile)
{
    enum end_of_word eow;
    
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = isWhile ? WHILE_COMMAND : UNTIL_COMMAND;
    com->input = NULL;
    com->output = NULL;

    push(DO);
    com->u.command[0] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);

    if(*eof)
    {
        return NULL;
    }

    while(peek() == DO)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case CHAIN_COMMAND:
                error(1, 0, "%d: cannot define chain command after 'do' ... exiting\n", current_line);
                return NULL;
            case END_OF_LINE:
            case MORE_ARGS:
            case SS_COMMAND:
                if(word_on_stack(*buf))
                {
                    //if(eow == MORE_ARGS)
                    //    error(1, 0, "%d: cannot have commands directly after 'do'\n", current_line);
                    pop(*buf);
                }
                else
                {
                    com->u.command[0] = create_sequence_after_loop(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com->u.command[0], eow);
                    if(*eof)
                        return NULL;
                }
                break; 
        }
    }

    com->u.command[1] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);
    
    while(peek() == DONE)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
            case CHAIN_COMMAND:
            case MORE_ARGS:
            case SS_COMMAND:
                if(word_on_stack(*buf))
                {
                    if(eow == MORE_ARGS)
                        error(1, 0, "%d: cannot have commands directly after 'done'\n", current_line);
                    pop(*buf);
                }
                else
                {
                    com->u.command[1] = create_sequence_after_loop(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com->u.command[1], eow);
                    if(*eof)
                        return NULL;
                }
                break; 
        }
    }
    if(eow == CHAIN_COMMAND)
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com);
    return com;
}

command_t create_subshell_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SUBSHELL_COMMAND;
    com->input = NULL;
    com->output = NULL;

    enum end_of_word eow;

    push(SUBSHELL);
    
    com->u.command[0] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, false);
    if(peek() == SUBSHELL)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
            case CHAIN_COMMAND:
            case SS_COMMAND:
                if(strcmp("(", *buf) == 0)
                    error(1, 0, "%d: cannot have adjacent subshells ... exiting\n", current_line);
            case MORE_ARGS:
                if(word_on_stack(*buf))
                {
                    pop(*buf);
                }
                else
                {
                    error(1, 0, "%d: unable to find ')' ... exiting\n", current_line);
                    return NULL;
                }
                break; 
        }
    }
    if(eow == CHAIN_COMMAND)
    {
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com);
    }
    return com;
}

command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, enum end_of_word first_word_status)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;
    com->u.word = checked_malloc(sizeof(char*));
    com->u.word[0] = NULL;
    size_t numLines = 1;

    enum end_of_word end = first_word_status;

    for(;;)
    {
        if(!isEmptyString(*buf))
        {
            com->u.word = checked_realloc(com->u.word, sizeof(char*) * ++numLines);
            com->u.word[numLines - 1] = NULL;
            com->u.word[numLines - 2] = checked_malloc(*buf_size);
            strcpy(com->u.word[numLines - 2], *buf);
        }

        switch(end)
        {
            case END_OF_FILE:          
                *eof = true;
            case END_OF_LINE:
            case SS_COMMAND:
                if(numLines == 1)
                {
                    return NULL;
                } 
                return com;    
            case CHAIN_COMMAND:
                return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, com);
            case MORE_ARGS:
                end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        }
    }
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF CREATE_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
                    void *get_next_byte_argument)
{
  init_stack();

  command_stream_t stream = init_command_stream();

  size_t buf_size, max_size = 5;
  char *buf = checked_malloc(max_size);
  bool eof = false;

  command_t c;
  while(1)
  {
    c = create_command(&buf, &buf_size, &max_size, get_next_byte, get_next_byte_argument, &eof, false);
    if(eof)
        break;
    if(c != NULL)
        add_command(stream, c);
  }
    if(!stack_is_empty())
        error(1, 0, "%d: items still on stack - syntax error detected ... exiting\n", current_line);

    return stream;
}

command_t
read_command_stream (command_stream_t s)
{
    return get_command(s);
}
