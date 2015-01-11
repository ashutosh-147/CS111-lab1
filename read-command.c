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

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "alloc.h"

#include <stdio.h>  // only for testing


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
  ELSE_OR_FI, //will have to consider special branching
  FI,
  ERROR,
};

struct stack_t
{
  enum branch_word * stack;
  size_t max_size;
  size_t head;
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

void pop()
{
  switch(peek())
    {
    case DO:
      my_stack.stack[my_stack.head-1] = DONE;
      return;
    default: //DONE and FI
      my_stack.head--;
      return;
    }
}

enum branch_word str_to_branch_word(char * str)
{
  if(strcmp("do",str) == 0)
    {
      return DO;
    }
  else if(strcmp("done",str) == 0)
    {
      return DONE;
    }
  else if(strcmp("then",str) == 0)
    {
      return THEN;
    }
  else if(strcmp("else",str) == 0)
    {
      return ELSE_OR_FI;
    }
  else if(strcmp("fi",str) == 0)
    {
      return FI;
    }
  else
    {
      return ERROR;
    }
}

//implement
bool word_on_stack(char * w)
{
  enum branch_word temp = str_to_branch_word(w);
  if(peek(my_stack) == temp)
    {
      return true;
/*
      if(temp != ELSE_OR_FI)
	{//not ELSE or FI
	  return true;
	}
      else if(temp == ELSE_OR_FI && strcmp("else",w) == 0)
	{//ELSE CASE
	  
	}
      else
	{//FI CASE
	  
	}
*/
    }  
  return false;
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

/*
enum end_of_word
{
    CONTINUE,
    END_OF_COMMAND,
    END_OF_FILE,
    NEW_COMMAND
}; */

enum end_of_word
{
    END_OF_FILE,
    END_OF_LINE,
    CHAIN_COMMAND,
    MORE_ARGS,
    // add enums for ( and )
};

char lastChar = '\0';
enum end_of_word get_next_word(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument)
{
    *buf_size = 0;

    if(lastChar != '\0')
    {
        append_char(buf, lastChar, buf_size, max_size);
        append_char(buf, '\0', buf_size, max_size);
        lastChar = '\0';
        return MORE_ARGS;
    }

    for(;;)
    {
        int nb = get_next_byte(get_next_byte_argument);
        if(nb == -1)
        {  // end of file
            append_char(buf, '\0', buf_size, max_size);
            return END_OF_FILE;
        }
        switch((char) nb)
        {
            case '\n':
                //if(*buf_size == 0)
                //    return get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
                append_char(buf, '\0', buf_size, max_size);
                return END_OF_LINE;
            case '|':
//            case '(':
//            case ')':
            case '<':
            case '>':
            case ';':
                append_char(buf, '\0', buf_size, max_size);
                lastChar = (char) nb;
                return CHAIN_COMMAND;
/*            case '\':
                if(*buf_size == 0)
                {
                    switch(get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument))
                    {

                    }
                }
                else
                    error(1, 0, "cannot have '\\' in a word ... exiting"); */
            case ' ':
            case '\t':
                if(*buf_size == 0)
                    return get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
                append_char(buf, '\0', buf_size, max_size);
                return MORE_ARGS;
            default:
                if(isAcceptable((char) nb))
                    append_char(buf, (char) nb, buf_size, max_size);
                else
                    error(1, 0, "bad character found ... exiting\n");
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

command_t create_pipe_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command);
command_t create_sequence_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command);
command_t add_io_redirection(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command, bool isInput);
command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, bool isWhile);
command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, enum end_of_word first_word_status);

// need to a check word on stack option
command_t create_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, bool check_stack)
{
//  printf("before_get_word_in_create_command\n");
  enum end_of_word end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
//  printf("after_get_word_in_create_command\n");
    if(end == END_OF_FILE)
    {
        *eof = true;
        return NULL;
    }
    if(check_stack)
        if(word_on_stack(*buf))
        {
            pop();
            return NULL;
        }
    if(isEmptyString(*buf))
    {
        return NULL;
    }
/*    if(strcmp("if", *buf) == 0)
    {
        
    }
    else if(strcmp("|", *buf) == 0)
    {

    }
    else if(strcmp(";", *buf) == 0)
    {

    }
    else if(strcmp("(", *buf) == 0)
    {

    }
    else */ if(strcmp("until", *buf) == 0)
    {
        printf("creating until command\n");
        return create_while_or_until_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, false);
    }
    else if(strcmp("while", *buf) == 0)
    {
        printf("creating while command\n");
        return create_while_or_until_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, true);
    }
    else
    {
        printf("creating simple command\n");
        return create_simple_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, end);
	}
}

// creates command for | ; < >
command_t create_chain_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command)
{
    // use end to check end of file and other errors
    enum end_of_word eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
    
    if(strcmp(";", *buf) == 0)
    {
        switch(eow)
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
                return first_command;
            case CHAIN_COMMAND:
                error(1, 0, "Error: double chain command ... exiting\n");
                *syntax = true;
                return first_command;
            case MORE_ARGS:
                break;
        }

        printf("creating ; command\n");
        return create_sequence_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command);
    }

    switch(eow)
    {
        case END_OF_FILE:
            *eof = true;
        case END_OF_LINE:
        case CHAIN_COMMAND:
            error(1, 0, "Error: double chain command ... exiting\n");
            *syntax = true;
            return first_command;
        case MORE_ARGS:
            break;
    }
    
    if(strcmp("|", *buf) == 0)
    {
        printf("creating | command\n");
        return create_pipe_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command);
    }
    else if(strcmp("<", *buf) == 0)
    {
        printf("adding <\n");
        return add_io_redirection(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command, true);
    }
    else
    {
        printf("adding >\n");
        return add_io_redirection(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command, false);
    }
}

command_t create_pipe_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = PIPE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, false);

    if(second_command == NULL)
    {
        error(1, 0, "Must have second command for pipe ... exiting\n");
        *syntax = true;
        return first_command;
    }
    else
    {
        com->u.command[0] = first_command;
        com->u.command[1] = second_command;
        return com;
    }
}

command_t create_sequence_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SEQUENCE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, true);
    
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

command_t add_io_redirection(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command, bool isInput)
{
    enum end_of_word eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);

    switch(eow)
    {
        case END_OF_FILE:
            *eof = true;
        case END_OF_LINE:
        case CHAIN_COMMAND:
            if(isEmptyString(*buf))
            {
                error(1, 0, "Cannot have chain command after io redirection ... exiting\n");
                *syntax = true;
                return first_command;
            }
        case MORE_ARGS:
            break;
    }

    if(isInput)
    {
        if(first_command->input != NULL || first_command->output != NULL)
        {
            error(1, 0, "Must define input first - Cannot define input twice ... exiting\n");
            *syntax = true;
            return first_command;
        }
        first_command->input = checked_malloc(*buf_size);
        strcpy(first_command->input, *buf);
    }
    else
    {
        if(first_command->output != NULL)
        {
            error(1, 0, "Cannot define output twice ... exiting\n");
            *syntax = true;
            return first_command;
        }
        first_command->output = checked_malloc(*buf_size);
        strcpy(first_command->output, *buf);
    }

    if(eow == CHAIN_COMMAND)
    {
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command);
    }
    return first_command;
}

command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, bool isWhile)
{
    enum end_of_word eow;
    
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = isWhile ? WHILE_COMMAND : UNTIL_COMMAND;
    com->input = NULL;
    com->output = NULL;

    push(DO);//onto stack
    com->u.command[0] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, false);
    if(*eof)
    {
        return NULL;
    }
    if(*syntax)
    {
        return NULL;
    }
    if(peek() == DO)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow) // deal with enum cases instead of default
        {
            case END_OF_FILE:
                *eof = true;
            case CHAIN_COMMAND:
                error(1, 0, "Cannot define chain command after 'do' ... exiting\n");
                *syntax = true;
                return NULL;
            case END_OF_LINE:
            case MORE_ARGS:
                if(word_on_stack(*buf))
                {
                    pop();
                }
                else
                {
                    error(1, 0, "Unable to find 'do' ... exiting\n");
                    *syntax = true;
                    return NULL;
                }
                break; 
        }
    }

    com->u.command[1] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, false);
    if(peek() == DONE)
    {
        eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
        
        switch(eow) // deal with enum cases instead of default
        {
            case END_OF_FILE:
                *eof = true;
            case END_OF_LINE:
            case CHAIN_COMMAND:
            case MORE_ARGS:
                if(word_on_stack(*buf))
                {
                    pop();
                }
                else
                {
                    error(1, 0, "Unable to find done ... exiting\n");
                    *syntax = true;
                    return NULL;
                }
                break; 
        }
    }
    if(eow == CHAIN_COMMAND)
        return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, com);
    return com;
}

command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, enum end_of_word first_word_status)
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
                if(numLines == 1)
                {
                    return NULL;
                } 
                return com;
            case CHAIN_COMMAND:
                return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, com);
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
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  init_stack();

  command_stream_t stream = init_command_stream();

  size_t buf_size, max_size = 5;
  char *buf = checked_malloc(max_size);
  bool eof = false;
  bool syntax = false;  // true if bad syntax

  command_t c;
  while(1)
  {
    c = create_command(&buf, &buf_size, &max_size, get_next_byte, get_next_byte_argument, &eof, &syntax, false);
    if(eof)
        break;
    if(syntax)
        error(1, 0, "bad syntax");  // improve error message
    if(c != NULL)
        add_command(stream, c);
  } 

    return stream;
}

command_t
read_command_stream (command_stream_t s)
{
    return get_command(s);
}
