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

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

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
        cs->stream = checked_grow_alloc(cs->stream, &(cs->max_size));
    cs->stream[cs->current_write++] = c;
}

command_t get_command(command_stream_t cs)
{
    if(cs->current_read == cs->current_write)
        return NULL;
    return cs->stream[cs->current_read++];
}

command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof);

command_t create_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof, bool *syntax)
{
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
    else if(strcmp("until", *buf) == 0)
    {

    }
    else if(strcmp("while", *buf) == 0)
    {

    }
    else
    { */
        return create_simple_command(buf, buf_size, max_size, get_next_byte, get_next_byte_argument, eof);
//    }
}

command_t create_simple_command(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument, bool *eof)
{
    command_t com = checked_malloc(sizeof(struct command));
    com->type = SIMPLE_COMMAND;
    com->input = NULL;
    com->output = NULL;
    com->u.word = checked_malloc(sizeof(char*));
    com->u.word[0] = NULL;
    size_t numLines = 1;

    enum end_of_word end;

    do
    {
//        printf("hello\n");
        end = get_next_word(buf, buf_size, max_size, get_next_byte, get_next_byte_argument);
//        printf("assigned to end\n");

        if(!isEmptyString(*buf))
        {
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
//            printf("reached end of command\n");
            if(numLines == 1)
            {
                com->u.word[0] = checked_malloc(sizeof(char));
                com->u.word[0][0] = '\0';
            }
            return com;
        }
        if(NEW_COMMAND == end)
        {
//            printf("reached new command\n");
            // create that command
            // return that command
        }
    } while(1);
}

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

  command_stream_t stream = init_command_stream();

  size_t buf_size, max_size = 5;
  char *buf = checked_malloc(max_size);
  int i;
  bool eof = false;
  bool syntax = true;

  command_t c;
  int commands = 1;
  while(1)
  {
    if(eof)
        break;
    c = create_command(&buf, &buf_size, &max_size, get_next_byte, get_next_byte_argument, &eof, &syntax);
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
