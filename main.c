// UCLA CS 111 Lab 1 main program

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

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t | -v | -x | -s] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int command_number = 1;
  bool print_tree = false;
  int verbose = 0;
  bool xtrace = false;
  bool step_through = false;
  int debug_level = 0;
  char const *profile_name = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "p:tvxs"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      case 'v': verbose = 1; break;
      case 'x': xtrace = true; break;
      case 's': step_through = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  if(xtrace)
    debug_level = 1;
  if(step_through)
    debug_level = 2;
      

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  if (profile_name)
    {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	    error (1, errno, "%s: cannot open", profile_name);
    }

  command_t last_command = NULL;
  command_t command;
    if(verbose)
    {
        FILE *verbose_stream = fopen (script_name, "r");
        //char **buf = NULL;
        //size_t size = 0;
        //while(getline(buf, &size, verbose_stream) != -1)
        //    printf("%s", *buf);
        char c;
        while((c = getc(verbose_stream)) != -1)
            printf("%c", c);
        fclose(verbose_stream);
    }
    if(debug_level == 2)
    {
      printf("Use ENTER to step through program line by line. Don't be pressing any other characters now >:(");
      getchar();
    }
  while ((command = read_command_stream (command_stream)))
    {
      if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
      else
	{
	  last_command = command;
	  execute_command (command, profiling, debug_level);
	}
    }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
