// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <sys/types.h> // for pid_t
#include <sys/wait.h> // for waitpid
#include <unistd.h> // for execvp, fork
#include <stdio.h> // for fprintf
#include <stdlib.h> // for exit
#include <fcntl.h> // for file open constants

int
command_status (command_t c)
{
  return c->status;
}

void
execute_simple (command_t c){

	// fork new process and store pid
	pid_t pid = fork();

	// int to store return status
	int status;

	// parent case
	if(pid > 0) {
		// wait for 
		waitpid(pid, &status, 0);

		fprintf(stdout, "%i \n", status);

		status = command_status(c);
		// TODO why do these print statements print different values?
		fprintf(stdout, "%i \n", status);
	}
	// child case
	else if (pid == 0) {

		// set stdin
		if(c->input != NULL) {
			// open input file for reading 
			int input_fd = open(c->input, O_RDONLY);

			// open error case
			if (input_fd < 0)
				error(1, 0, "Error: Unable to read input file: %s \n", c->input);
			// sets input_fd to stdin
			if (dup2(input_fd, 0) < 0)
				error(1, 0, "Error: Failed to copy file descriptor \n");
			// closes file
			if (close(input_fd) < 0)
				error(1, 0, "Error: Failed to close file");
		}

		// set stdout
		if(c->output != NULL) {

			// open output file to be created, cleared, wrote to, with RW user permission
			int output_fd = open(c->output, O_CREAT | O_WRONLY | O_TRUNC | S_IWUSR | S_IRUSR );

			// open error case
			if (output_fd < 0)
				error(1, 0, "Error: Unable to read input file: %s \n", c->output);
			// sets output_fd to stdout
			if (dup2(output_fd, 1) < 0)
				error(1, 0, "Error: Failed to copy file descriptor \n");
			// closes file
			if (close(output_fd) < 0)
				error(1, 0, "Error: Failed to close file");
		}

		// execute simple command program
		execvp(c->u.word[0], c->u.word);

		// if child fails to start new process this will run
		error(1, 0, "Error: Unknown command: %s \n", c->u.word[0]);
	}
}

void
execute_subshell (command_t c){
	
}

void
execute_and (command_t c){
	
}

void
execute_or (command_t c){
	
}

void
execute_sequence (command_t c){
	
}

void
execute_pipe (command_t c){
	
}

void
execute_command (command_t c, int time_travel)
{
  switch(c->type) {
  	case SIMPLE_COMMAND:
  		execute_simple(c);
  		break;
  	case SUBSHELL_COMMAND:
  		execute_subshell(c);
  		break;
  	case AND_COMMAND:
  		execute_and(c);
  		break;
  	case OR_COMMAND:
  		execute_or(c);
  		break;
  	case SEQUENCE_COMMAND:
  		execute_sequence(c);
  		break;
  	case PIPE_COMMAND:
  		execute_pipe(c);
  		break;
  	// invalid command type
  	default:
  		error(1, 0, "Invalid Command Type");
  }
}
