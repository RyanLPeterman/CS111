// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>		// for error(1, 0, ...)
#include <sys/types.h> 	// for pid_t
#include <sys/wait.h> 	// for waitpid
#include <unistd.h> 	// for execvp, fork
#include <stdio.h> 		// for fprintf
#include <stdlib.h> 	// for exit
#include <fcntl.h> 		// for file open constants
#include <stdbool.h> 	// for bool
#include <string.h>		// for strcmp

int
command_status (command_t c)
{
  return c->status;
}

///////////////////////////////////////////////////////////////////////////////////
//////////////////////    Serial Execution Functions    ///////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// sets redirection for subshells and simple commands
void set_redirection(command_t c) {

	// set stdin
	if(c->input != NULL) {
		// open input file for reading and writing
		int input_fd = open(c->input, O_RDWR);

		// open error case
		if (input_fd < 0)
			error(1, 0, "Error: Unable to open input file: %s \n", c->input);
		// sets input_fd to stdin
		if (dup2(input_fd, 0) < 0)
			error(1, 0, "Error: Failed to copy input file descriptor \n");
		// closes file
		if (close(input_fd) < 0)
			error(1, 0, "Error: Failed to close input file");
	}

	// set stdout
	if(c->output != NULL) {

		// open output file to be created/written to with necessary flags
		int output_fd = open(c->output, O_CREAT | O_WRONLY | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		// open error case
		if (output_fd < 0)
			error(1, 0, "Error: Unable to open output file: %s \n", c->output);
		// sets output_fd to stdout
		if (dup2(output_fd, 1) < 0)
			error(1, 0, "Error: Failed to output copy file descriptor \n");
		// closes file
		if (close(output_fd) < 0)
			error(1, 0, "Error: Failed to close output file");
	}
	return;
}

void
execute_simple (command_t c){

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

		// set input and output for command
		set_redirection(c);

		// execute simple command program
		execvp(c->u.word[0], c->u.word);

		// if child fails to start new process this will run
		error(1, 0, "Error: Unknown command: %s \n", c->u.word[0]);
	}
	else
		error(1, 0, "Error: Problem when forking");
}

void
execute_subshell (command_t c){
	
	// set input and output for command
	set_redirection(c);

	// start executing subshell's commands
	execute_command(c->u.subshell_command, false);
}

void
execute_and (command_t c){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	// execute first command
	execute_command(first_command, false);

	// if first command executed correctly then execute second command
	if(first_command->status == 0) {
		execute_command(second_command, false);
		c->status = second_command->status;
	}
	else // first command failed
		c->status = first_command->status;
}

void
execute_or (command_t c){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	// execute first command
	execute_command(first_command, false);

	// if first command failed then we run second command
	if(first_command->status != 0) {
		execute_command(second_command, false);
		c->status = second_command->status;
	}
	else // first command succeeded store result
		c->status = first_command->status;
}

void
execute_sequence (command_t c){
	// store commands to be executed
	command_t first_command = c->u.command[0];
	command_t second_command = c->u.command[1];

	// execute both commands in sequence
	execute_command(first_command, false);
	execute_command(second_command, false);

	// store result of second command
	c->status = second_command->status;
}

void
execute_pipe (command_t c){
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

	// child inherits open file discriptors
	pid_child1 = fork();

	// parent case
	if(pid_child1 > 0) {

		// child inherits open file discriptors
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
		// second child case
		else if (pid_child2 == 0) {
			// close file for reading
			close(fd_array[0]);

			// copy fd for writing
			if(dup2(fd_array[1], 1) < 0)
				error(1, 0, "Error: Failed to copy file descriptor \n");

			execute_command(first_command, false);

			// call _exit if execvp fails in execute_command
			// so that it doesn not interfere with parent process
			_exit(first_command->status);
		}
		else
			error(1, 0, "Error: Problem when forking");
	}
	// first child case
	else if (pid_child1 == 0) {
		// close file for writing
		close(fd_array[1]);

		// copy fd for reading
		if(dup2(fd_array[0], 0) < 0)
			error(1, 0, "Error: Failed to copy file descriptor \n");

		execute_command(second_command, false);
		
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

///////////////////////////////////////////////////////////////////////////////////
/////////////////////    Parallel Execution Functions    //////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// executes graph
int execute_graph(dependency_graph_t graph){
	int status = 0; 	// for exit status
	pid_t pid = fork(); // for child to execute graph

	// child case
	if(pid == 0) {

		// execute not dependent list and dependent list of commands
		execute_no_dependencies(graph->no_dependencies);
		execute_dependencies(graph->dependencies);

		// first we wait for not dependent processes to all finish in parallel
		execution_list_node_t waiting_to_finish = graph->no_dependencies;

		while(waiting_to_finish != NULL) {
			// wait for any process to finish
			waitpid(-1, &status, 0);
			// increment until we get to end of list
			waiting_to_finish = waiting_to_finish->next;
		}

		// now we wait for dependencies to finish
		waiting_to_finish = graph->dependencies;

		while(waiting_to_finish != NULL) {

			// wait specifically for each command
			waitpid(waiting_to_finish->node->pid, &status, 0);
			// increment until end of the list
			waiting_to_finish = waiting_to_finish->next;
		}

		// exit when finished
		_exit(0);
	}
	// parent waits for child to finish
	else if(pid > 0) {
		waitpid(pid, &status, 0);
	}
	return status;
}

// execute all commands that have no dependencies
void execute_no_dependencies(execution_list_node_t execution_list) {
	
	// init iteration pointer
	execution_list_node_t execution_ptr = execution_list;

	// loop for every element in the execution list
	while(execution_ptr != NULL) {

		// for a process for every command
		pid_t pid = fork();

		// child case
		if(pid == 0) {
			// execute command
			execute_command(execution_ptr->node->cmd, false);
			_exit(0);
		}
		else // set pid for waitpid later
			execution_ptr->node->pid = pid;

		// iterate to next command
		execution_ptr = execution_ptr->next;
	}

	return;
}

// execute all commands that have dependencies
void execute_dependencies(execution_list_node_t execution_list) {
	
	// init iter pointer
	execution_list_node_t dependency_iter = execution_list;

	// for every element in the execution list
	while(dependency_iter != NULL) {

		graph_node_t curr = dependency_iter->node;
		graph_node_t* dependencies = curr->dependencies;
		int status;

		// while there are dependencies left
		while(*dependencies != NULL) {

			// wait for dependent process to end
			waitpid((*dependencies)->pid, &status, 0);
			// increment iterator in dependencies pointer array 
			dependencies++;
		}

		pid_t pid = fork();
		// child executes command
		if(pid == 0) {
			// execute command
			execute_command(curr->cmd, false);
			_exit(0);
		} // set pid for waitpid later
		else if (pid > 0)
			curr->pid = pid;

		// iterate to next ptr in dependent commands list
		dependency_iter = dependency_iter->next;
	}
	return;
}

///////////////////////////////////////////////////////////////////////////////////
//////////////////    Build Dependency Graph Functions    /////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// given a command and its execution list_node fills out its read/write list
void fill_read_write_list(command_t cmd, execution_list_node_t node) {

	switch(cmd->type) {
		case SIMPLE_COMMAND:
			// add the redirects to simple command to read/write list
			if(cmd->input != NULL)
				add_file_node(cmd->input, &(node->read_list));
			if(cmd->output != NULL)
				add_file_node(cmd->output, &(node->write_list)); 

			char** words = cmd->u.word;
			// start at word[1] since word[0] is the name of the command
			int i = 1;
			for (; words[i] != NULL; i++)
			{	
				// if the first char of the string is not a - 
				if(words[i][0] != '-') {
					add_file_node(words[i], &(node->read_list));
				}
			}

			break;
		case SUBSHELL_COMMAND:
			// add the input redirects to subshell to read/write list
			if(cmd->input != NULL)
				add_file_node(cmd->input, &(node->read_list));
			if(cmd->output != NULL)
				add_file_node(cmd->output, &(node->write_list)); 

			// recursively add subshell's files to read write list
			fill_read_write_list(cmd->u.subshell_command, node);				
			break;
		// each of these commands contains two more commands
		case AND_COMMAND:
		case SEQUENCE_COMMAND:
		case OR_COMMAND:
		case PIPE_COMMAND:
			// recursively add new command's files to read write list
			fill_read_write_list(cmd->u.command[0], node);
			fill_read_write_list(cmd->u.command[1], node);
			break;
	}
	return;
}

// cross references the node's read_lists and write lists 
bool is_dependent(const execution_list_node_t a, const execution_list_node_t b) {
	// Read after write dependency
	if(is_intersection(a->write_list, b->read_list))
		return true;
	// Write after write dependency
	if(is_intersection(b->write_list, b->write_list))
		return true;
	// Write after read dependency
	if(is_intersection(a->read_list, b->write_list))
		return true;

	// return false if no dependency
	return false;
}

// returns true if the two linked lists of strings contains the same element
bool is_intersection(const file_list_node_t a, const file_list_node_t b) {

	// init iteration pointers
	file_list_node_t a_ptr = a;
	file_list_node_t b_ptr = b;

	// for every file in a
	while(a_ptr) {

		// reinit b 
		b_ptr = b;

		// check every file in b
		while(b_ptr) {

			// if the file names are equal
			if(strcmp(a_ptr->file_name, b_ptr->file_name) == 0)
				return true;

			// increment b_ptr
			b_ptr = b_ptr->next;
		}
		// increment a_ptr
		a_ptr = a_ptr->next;
	}

	return false;
}

// builds dependency graph
dependency_graph_t build_dependency_graph(command_stream_t command_stream){
	
	// initialize graph
	dependency_graph_t graph = checked_malloc(sizeof(dependency_graph));
	graph->no_dependencies = NULL;
	graph->dependencies = NULL;

	command_t cmd = NULL;
	// will contain all commands and add one at a time to be looped through to check for dependencies
	execution_list_node_t total_list = NULL;

	// loop through every command in command_stream
	while( (cmd = read_command_stream(command_stream)) ) {

		// default construct a new graph node
		graph_node_t new_graph_node = checked_malloc(sizeof(graph_node));
		new_graph_node->cmd = cmd;
		new_graph_node->dependencies = NULL;
		new_graph_node->num_dependencies = 0;
		new_graph_node->pid = -1;

		// default construct a new execution_list_node
		execution_list_node_t new_exec_node = checked_malloc(sizeof(execution_list_node));
		new_exec_node->read_list = NULL;
		new_exec_node->write_list = NULL;
		new_exec_node->next = NULL;
		new_exec_node->node = new_graph_node;

		// now we set the read_list and write_list of the exec node
		fill_read_write_list(cmd, new_exec_node);

		// now set up the dependencies of the graph
		execution_list_node_t curr_list = total_list;
		int num_dependencies = 0;
		int capacity = 256;     // max number of dependencies initially
		new_graph_node->dependencies = checked_malloc(sizeof(graph_node_t) * capacity);

		// while there is a command to check against the new command
		while (curr_list) {

			// if new node is dependent on current node in curr_list
			if(is_dependent(new_exec_node, curr_list)) {

				// set dependency and increment counter
				new_graph_node->dependencies[num_dependencies] = curr_list->node;
				num_dependencies++;

				// more dependencies than capacity can hold
				if(num_dependencies == capacity) {

					// increase capacity and reallocate memory
					capacity *= 2;
					new_graph_node->dependencies = checked_realloc(new_graph_node->dependencies, sizeof(graph_node_t) * capacity);
				}
			}

			// increment total list pointer
			curr_list = curr_list->next;
		}

		// insert the new node into the total list to be looped through
		add_execution_node(new_exec_node, &total_list);

		// no dependencies
		if(num_dependencies == 0)
			// insert the node into the no_dependencies graph
			add_graph_node(new_graph_node, &(graph->no_dependencies));
		else
			// insert the node into the dependencies part of the graph
			add_graph_node(new_graph_node, &(graph->dependencies));
	}

	return graph;
}


/////////////////////////////////////////////////////////////////////
///////////////// Linked List Helper Functions //////////////////////
/////////////////////////////////////////////////////////////////////


// adds a graph node to an execution list
// lets us populate the list of commands in the context of checking for dependencies
void add_execution_node(execution_list_node_t to_add, execution_list_node_t* list) {
	
	// empty list case
	if(*list == NULL) {
		*list = to_add;
	}
	else {
		execution_list_node_t temp = *list;

		// seek temp to the end of list
		while(temp->next) {
			temp = temp->next;
		}

		temp->next = to_add;
	}
	return;
}

// adds a graph node to an execution list
// lets us populate the graph's execution lists
void add_graph_node(graph_node_t to_add, execution_list_node_t* list) {

	// initialize node to be added
	execution_list_node_t add = checked_malloc(sizeof(execution_list_node));
	add->node = to_add;
	add->read_list = NULL;
	add->write_list = NULL;
	add->next = NULL;

	// empty list
	if(*list == NULL) {
		*list = add;
	}
	else {
		execution_list_node_t temp = *list;

		// seek temp to the end of list
		while(temp->next) {
			temp = temp->next;
		}

		temp->next = add;
	}
	return;
}

// adds a node to the list
void add_file_node(char* to_add, file_list_node_t* list) {
	
	// initialize node to be added
	file_list_node_t add_node = checked_malloc(sizeof(file_list_node));
	add_node->file_name = to_add;
	add_node->next = NULL;

	// empty list
	if(*list == NULL) {
		*list = add_node;
	}
	else {

		file_list_node_t ptr = *list;
		// seek to end of list
		while(ptr->next != NULL) {
			ptr = ptr->next;
		}

		ptr->next = add_node;
	}

	return;
}