// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h> // for atoi

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p] [-tn#] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  // will hold max num subprocesses
  int max_sub_proc = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "ptn:"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
      case 'n': 
        // error checking
        if((max_sub_proc = atoi(optarg)) <= 0)
          error(1, errno, "Number after n must be a valid int greater than 0");
        break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
    make_command_stream (get_next_byte, script_stream);

  command_t last_command = NULL;
  command_t command;

  // we wish to execute in parallel
  if(time_travel) {
    // build dependency graph
    dependency_graph_t graph = build_dependency_graph(command_stream);

    // execute dependency graph commands in parallel when possible
    // additionally: passes in the max number of sub processes that can run
    return execute_graph(graph, max_sub_proc);
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
	    execute_command (command, time_travel);
	  }
  }

  return print_tree || !last_command ? 0 : command_status (last_command);
}
