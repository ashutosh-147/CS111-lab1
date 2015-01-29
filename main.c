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

#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>
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
  int prof_failed = 0;
  int profiling = -1;
  struct timespec t1;
  if (profile_name)
  {
    profiling = prepare_profiling (profile_name);
    clock_gettime(CLOCK_REALTIME, &t1);
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
        char c, lastChar;
        while((c = getc(verbose_stream)) != -1)
        {
            printf("%c", c);
            lastChar == c;
        }
        fclose(verbose_stream);
        if(lastChar != '\n');
        printf("\n");
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
	  prof_failed += execute_command (command, profiling, debug_level);
	}
    }

    if (profile_name)
    {
        if(prof_failed != 0)
          return -1;
        const long long nsec_to_sec = 1000000000;
        const long long usec_to_sec = 1000000;
    
        struct timespec abs_time;
        clock_gettime(CLOCK_REALTIME, &abs_time);

        // record absolute time
        double end_time = (double) abs_time.tv_sec + (double) abs_time.tv_nsec / nsec_to_sec;
        dprintf(profiling, "%.2f ", end_time);

        // record total execution time
        double exec_time = (double) (abs_time.tv_sec - t1.tv_sec) + (((double) (abs_time.tv_nsec - t1.tv_nsec)) / nsec_to_sec);
        dprintf(profiling, "%.3f ", exec_time);

        struct rusage usage;
        getrusage(RUSAGE_CHILDREN, &usage);

        // record user cpu time
        dprintf(profiling, "%.3f ", ((double) usage.ru_utime.tv_sec + (double) usage.ru_utime.tv_usec / usec_to_sec));
        // record system cpu time
        dprintf(profiling, "%.3f ", ((double) usage.ru_stime.tv_sec + (double) usage.ru_stime.tv_usec / usec_to_sec));

        dprintf(profiling, "[%d]\n", getpid());
    }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
