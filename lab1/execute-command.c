// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>		// for error(1, 0, ...)
#include <sys/types.h> 	// for pid_t
#include <sys/wait.h> 	// for waitpid
#include <unistd.h> 	// for execvp, fork
#include <stdio.h> 		// for fprintf
#include <stdlib.h> 	// for exit
#include <fcntl.h> 		// for file open constants
#include <stdbool.h> 	// for bool

int
command_status (command_t c)
{
  return c->status;
}

void
execute_simple (command_t c, bool is_time_travel){

	// fork new process and store pid
	pid_t pid = fork();

	// int to store return status
	int status;

	// parent case
	if(pid > 0) {
		// wait for child
		waitpid(pid, &status, 0);
		// store return status in command status
		c->status = status;
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
	else
		error(1, 0, "Error: Problem when forking");
}

void
execute_subshell (command_t c, bool is_time_travel){
	
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

	execute_command(c->u.subshell_command, is_time_travel);
}

void
execute_and (command_t c, bool is_time_travel){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	execute_command(first_command, is_time_travel);

	// if first command executed correctly then execute second command
	if(first_command->status == 0) {
		execute_command(second_command, is_time_travel);
		c->status = second_command->status;
	}
	else // first command failed
		c->status = first_command->status;
}

void
execute_or (command_t c, bool is_time_travel){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	execute_command(first_command, is_time_travel);

	// if first command failed then we run second command
	if(first_command->status != 0) {
		execute_command(second_command, is_time_travel);
		c->status = second_command->status;
	}
	else // first command succeeded store result
		c->status = first_command->status;
}

void
execute_sequence (command_t c, bool is_time_travel){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	execute_command(first_command, is_time_travel);
	execute_command(second_command, is_time_travel);

	// store result of second command
	c->status = second_command->status;
}

void
execute_pipe (command_t c, bool is_time_travel){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	// will hold status
	int status;
	// buffer holding fds
	int fd_array[2];
	pid_t pid_child1, pid_child2, returned_pid;

	// error case
	if(pipe(fd_array) < 0) 
		error(1, 0, "Error: Problem when creating pipe");

	pid_child1 = fork();

	// parent case
	if(pid_child1 > 0) {

		pid_child2 = fork();

		// parent's parent
		if(pid_child2 > 0) {
			// Close parent input and output
			close(fd_array[0]);
			close(fd_array[1]);

			// Wait for any process to finish (-1)
			returned_pid = waitpid(-1, &status, 0);

			// if the first child returned
			if(returned_pid == pid_child1) {
				// store its status
				c->status = status;
				// wait for second child
				waitpid(pid_child2, &status, 0);
				return;
			}
			// if the second child returned
			else if(returned_pid == pid_child2) {
				// store its status
				c->status = status;
				// wait for first child
				waitpid(pid_child1, &status, 0);
				return;
			}
		}
		// parent's child
		else if (pid_child2 == 0) {
			close(fd_array[0]);
			if(dup2(fd_array[1], 1) < 0)
				error(1, 0, "Error: Failed to copy file descriptor \n");
			execute_command(first_command, is_time_travel);
			// call _exit if execvp fails in execute_command
			// so that it doesn not interfere with parent process
			_exit(first_command->status);
		}
		else
			error(1, 0, "Error: Problem when forking");
	}
	// child case
	else if (pid_child1 == 0) {
		close(fd_array[1]);
		if(dup2(fd_array[0], 0) < 0)
			error(1, 0, "Error: Failed to copy file descriptor \n");
		execute_command(second_command, is_time_travel);
		// call _exit if execvp fails in execute_command
		// so that it doesn not interfere with parent process
		_exit(second_command->status);
	}
	else
		error(1, 0, "Error: Problem when forking");

}

void
execute_command (command_t c, int time_travel)
{
  switch(c->type) {
  	case SIMPLE_COMMAND:
  		execute_simple(c, time_travel);
  		break;
  	case SUBSHELL_COMMAND:
  		execute_subshell(c, time_travel);
  		break;
  	case AND_COMMAND:
  		execute_and(c, time_travel);
  		break;
  	case OR_COMMAND:
  		execute_or(c, time_travel);
  		break;
  	case SEQUENCE_COMMAND:
  		execute_sequence(c, time_travel);
  		break;
  	case PIPE_COMMAND:
  		execute_pipe(c, time_travel);
  		break;
  	// invalid command type
  	default:
  		error(1, 0, "Invalid Command Type");
  }
}
