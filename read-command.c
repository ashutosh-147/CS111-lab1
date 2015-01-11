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
//    printf("before if\n");
    if(*size == *max_size)
    {
//        printf("allocating more memory\n");
        *buf = checked_grow_alloc(*buf, max_size);
    }
//    printf("appending char %d of max %d\n", *size, *max_size);
    buf[0][(*size)++] = c;
//    printf("after append\n");
}

bool isEmptyString(char *buf)
{
    return *buf == '\0';
}

void printString(char *buf, size_t size)
{
    int i;
    for(i = 0; i < size; i++)
        printf("%c", buf[i]);
    printf("\n");
}

enum end_of_word
{
    CONTINUE,
    END_OF_COMMAND,
    END_OF_FILE,
    NEW_COMMAND
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
        return CONTINUE;
    }
//    printf("past if\n");
    for(;;)
    {
        int nb = get_next_byte(get_next_byte_argument);
        if(nb == -1)
        {  // end of file
//            printf("reached end of file\n");
            append_char(buf, '\0', buf_size, max_size);
            return END_OF_FILE;
        }
//        printf("%c\n", (char) nb);
        switch((char) nb)
        {
            case '\n':
//                printf("reached end of command\n");
                append_char(buf, '\0', buf_size, max_size);
//                printf("%s\n", *buf);
                return END_OF_COMMAND;
            case '|':
            case '(':
            case ')':
            case '<':
            case '>':
            case ';':
//                printf("found new command\n");
                append_char(buf, '\0', buf_size, max_size);
                lastChar = (char) nb;
                return NEW_COMMAND;
            case ' ':
            case '\t':
//                printf("reached end of word\n");
                append_char(buf, '\0', buf_size, max_size);
//                printf("properly appended\n");
                return CONTINUE;
            default:
//                printf("de neving\n");
                append_char(buf, (char) nb, buf_size, max_size);
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
command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, bool isWhile);
command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, enum end_of_word first_word_status);

command_t create_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax)
{
//  printf("before_get_word_in_create_command\n");
  enum end_of_word end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
//  printf("after_get_word_in_create_command\n");
  if(end == END_OF_FILE)
    {
      *eof = true;
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
    enum end_of_word end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
    
    if(strcmp("|", *buf) == 0)
    {
        return create_pipe_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command);
    }
    else //if(strcmp(";", *buf) == 0)
    {
        return create_sequence_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, first_command);
    }
}

command_t create_pipe_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = PIPE_COMMAND;
    com->input = NULL;
    com->output = NULL;
    
//    com->u.command[0] = first_command;
//    com->u.command[1] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax);

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax);

    if(second_command == NULL)
    {
        *syntax = true;
        return first_command;
    }
    else
    {
        com->u.command[0] = first_command;
        com->u.command[1] = second_command;
    }
    return com;
}

command_t create_sequence_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, command_t first_command)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->status = -1;
    com->type = SEQUENCE_COMMAND;
    com->input = NULL;
    com->output = NULL;

    command_t second_command = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax);
    
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

command_t create_while_or_until_command(char ** buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax, bool isWhile)
{
    enum end_of_word eow;
    
  command_t com = checked_malloc(sizeof(struct command));
  com->status = -1;
  com->type = isWhile ? WHILE_COMMAND : UNTIL_COMMAND;
  com->input = NULL;
  com->output = NULL;
  push(DO);//onto stack
  printf("getting Command A\n");
  com->u.command[0] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax);
  if(*eof)
  {
    printf("reached end of file\n");
    return NULL;
  }
  if(*syntax)
  {
    printf("bad syntax ... exiting\n");
    return NULL;
  }
  if(peek() == DO)
  {
    printf("do on stack\n");
    do 
    {
     eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
     printf("%s\n", *buf);
    } while(isEmptyString(*buf) && eow != END_OF_FILE);
    switch(eow)
    {
        case END_OF_FILE:
            printf("reached end of file\n");
            return NULL;
        default:
            if(word_on_stack(*buf))
            {
                pop();
                printf("found do\n");
            }
            else
            {
                printf("did not find do\n");
                *syntax = true;
                return NULL;
            }
            break; 
    }
  }
  
    printf("getting command B\n");
  com->u.command[1] = create_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax);
  if(peek() == DONE)
  {
    printf("done on stack\n");
    do 
    {
     eow = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
     printf("%s\n", *buf);
    } while(isEmptyString(*buf) && eow != END_OF_FILE);
    switch(eow)
    {
        case END_OF_FILE:
            printf("reached end of file\n");
            return NULL;
        default:
            if(word_on_stack(*buf))
            {
                pop();
                printf("found done\n");
            }
            else
            {
                printf("did not find done\n");
                *syntax = true;
                return NULL;
            }
            break; 
    }
  }
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

    do
    {
//        printf("hello\n");
        
//        printf("assigned to end\n");

        if(!isEmptyString(*buf))
        {
            printf("%s\n", *buf);
            com->u.word = checked_realloc(com->u.word, sizeof(char*) * ++numLines);
            com->u.word[numLines - 1] = NULL;
            com->u.word[numLines - 2] = checked_malloc(*buf_size);
            strcpy(com->u.word[numLines - 2], *buf);
            //printf("%d\n", numLines);
        }

        if(END_OF_FILE == end)
        {
//            printf("reached end of file\n");          
            *eof = true;
            if(numLines == 1)
            {
                return NULL;
            } 
            return com;
        }
        if(END_OF_COMMAND == end)
        {
            printf("end of command reached - simple command\n");
            if(numLines == 1)
            {
                return NULL;
            }
            return com;
        }
        if(NEW_COMMAND == end)
        {
            return create_chain_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof, syntax, com);
        }
	end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
    } while(1);
}

/////////////////////////////////////////////////
/////////////////////////////////////////////////
//END OF CREATE_COMMAND IMPLEMENTATION
/////////////////////////////////////////////////
/////////////////////////////////////////////////

void printThingy(command_t c)
{
    if(c == NULL)
        return;
    char **w = c->u.word;
	printf ("%*s%s", 2, "", *w);
	while (*++w)
	  printf (" %s", *w);
}

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
  int i;
  bool eof = false;
  bool syntax = false;  // true if bad syntax

  command_t c;
  int commands = 1;
  while(1)
  {
    c = create_command(&buf, &buf_size, &max_size, get_next_byte, get_next_byte_argument, &eof, &syntax);
    if(eof)
        break;
    if(syntax)
        error(1, 0, "bad syntax");
    if(c != NULL)
        add_command(stream, c);
//    printf("\n");
//    printf("commands %d\n", commands++);
  } 

    return stream;

//  error (1, 0, "command reading not yet implemented");
//  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
    return get_command(s);
  //error (1, 0, "command reading not yet implemented");
  //return 0;
}
