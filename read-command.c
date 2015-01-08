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
#include "alloc.h"

#include <stdio.h>  // only for testing

void append_char(char **buf, char c, size_t *size, size_t *max_size) {
    if(*size == *max_size) {
//        printf("allocating more memory\n");
        *buf = checked_grow_alloc(*buf, max_size);
    }
//    printf("appending char %d of max %d\n", *size, *max_size);
    buf[0][(*size)++] = c;
}

void printString(char *buf, size_t size) {
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
enum end_of_word get_next_word(char **buf, size_t *buf_size, size_t *max_size, int (*get_next_byte) (void *), void *get_next_byte_argument) {
    *buf_size = 0;

    if(lastChar != '\0') {
        append_char(buf, lastChar, buf_size, max_size);
        append_char(buf, '\0', buf_size, max_size);
        lastChar = '\0';
        return CONTINUE;
    }

    for(;;) {
        int nb = get_next_byte(get_next_byte_argument);
        if(nb == -1) {  // end of file
//            printf("reached end of file\n");
            append_char(buf, '\0', buf_size, max_size);
            return END_OF_FILE;
        }
        //printf("%c\n", (char) nb);
        switch((char) nb) {
            case '\n':
            case ';':
//                printf("reached end of command\n");
                append_char(buf, '\0', buf_size, max_size);
                return END_OF_COMMAND;
            case '|':
            case '(':
            case ')':
            case '<':
            case '>':
//                printf("found new command\n");
                append_char(buf, '\0', buf_size, max_size);
                lastChar = (char) nb;
                return NEW_COMMAND;
            case ' ':
//                printf("reached end of word\n");
                append_char(buf, '\0', buf_size, max_size);
                return CONTINUE;
            default:
                append_char(buf, (char) nb, buf_size, max_size);
        }
    }
}

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  size_t buf_size, max_size = 5;
  char *buf = checked_malloc(5);
  int i;
  while(END_OF_FILE != get_next_word(&buf, &buf_size, &max_size, get_next_byte, get_next_byte_argument)) {
    printf("%s\n", buf);
  } 


  error (1, 0, "command reading not yet implemented");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
