# CS111 Lab 1 (Implementing a Bash-like Shell)

	Name: Ryan Peterman
	UID: 704269982

## Lab 1A:
	Method Overview:

		1. Read input file into a buffer and remove commands as we read it into the buffer

		2. Convert input buffer into a linked list of tokens, which are catagorized/labeled 
		operators and commands 

		3. Pass through the created linked list of tokens to make sure the ordering is syntactically
		valid

		4. Now we take the valid list of tokens and create a command stream using an operator stack
		and a command stack to create the command trees that will be linked up in our command stream

	Function Explanations:

		1. read_file_into_buffer - reads file into a buffer, while parsing out comments. If a file
		is too large for the starting size of the buffer, then new memory will be allocated using 
		checked_grow_alloc.

		2. convert_to_tokens - Given a buffer with no comments in it, creates a linked list of 
		"tokens" which are catagorized commands and/or operators of significance. Each token keeps 
		track of its line number and what type of token it is for easier error checking later.

		3. check_token_list - Given a list of tokens, checks the syntax/ordering of the commands 
		and operators to make sure that everything is valid.

		4. print_token_list - For Debugging: prints out all token information to standard output 
		in the following format: Type (type): Line Number (#) : Words (words contained).

		5. is_valid - given a character, this function validates that the character is of the list 
		of allowed characters based on the spec

		6. make_basic_stream - given a valid token list, this function converts the token list into 
		a command stream with no depth

		7. solve_newlines - given a simple command stream, this function converts newlines into its 
		corresponding cases

		8. make_advanced_stream - given a valid basic stream, this function uses a command stack 
		and an operator stack to populate the command stream with many command trees

	Data Structures:

		1. Token - a unique operator/command that contains its type (and vs or vs word vs ...), 
		line number, and string containing words if it was of type word.

		2. Token List - a linked list of token_list (nodes) which each contain a token, pointer to 
		the next structure, and pointer to the previous structure

		3. Command Stream - a linked list of commands (each of which branch of into command trees 
		based on the command data structure given). This linked list has a head pointer, current 
		pointer, and a size variable. The current pointer allows us to iterate through the list 
		and also keep track of the end of the list when adding nodes.

		4. Command Stack and Operator Stack - a linked list of commands. Contains a pointer to the 
		top of the stack and also maintains a variable that contains the current size of the stack 
		as well. 

## Lab 1B:

	Function Explanations:

		execute_command - This function is a switch case that makes a function call based on the 
		type of the command passed in

		set_redirection - prepares input and output for a command that requires the correct stdin 
		and stdout

		execute_simple - shell forks a child which executes the command specified

		execute_subshell - sets input and output to the subshell then executes simple command 
		specified inside (this should not be called with current lab 1A implementation)

		execute_and - if the first command executes correctly it executes the second command

		execute_or - if the first command fails then we execute the second command

		execute_sequence - executes first and second command with return status of the second command

		execute_pipe - parent calls pipe to set up pipe with two different file descriptors then 
		from there forks two chidlren to run the commands

## Lab 1C:

	Data Structures:
		
		graph_node - graph data structure node that keeps track of all the dependencies. Each node 
		contains a pointer to the command as well as an array of pointers to other nodes showing 
		the dependencies

		execution_list_node - linked list used to contain the nodes we which to execute based on 
		their dependencies. Keeps track of the read list and write list of each node.

		dependecy_graph - contains two execution_lists. One list has no dependencies and can 
		therefore be executed trivially, while the other has dependencies and we must therefore 
		wait for each specific one to be done in order depending on the dependencies.

		file_list_node - linked list of strings containing file names to check for dependencies 
		amongs read and writes

	Function Explanations:

		is_dependent - calls is_intersection on two file lists in the correct order. returns true 
		if there is a RAW, WAW, WAR dependency among two lists

		is_intersection - helper function to determine if two linked lists contain the same string

		execute_graph - highest level function to execute a graph that has two execution lists

		build_dependency_graph - returns a graph with all dependencies set up 

		execute_no_dependencies - helper function to execute a list of commands given that we can 
		assume they have no dependencies

		execute_dependencies - helper function to execute a list of commands given that we can 
		assume they are dependent

		fill_read_write_list - recursively searches a command tree and populates a linked list of 
		strings with read and write

		add_file_node - helper function to add a node to file list

		add_execution_node - helper function to add a node to execution list

		add_graph_node - helper function to add a graph_node to an execution_list

##Design Portion (1C): 

	Prompt: 
		
		Limit the amount of parallelism to at most N subprocesses, where N is a parameter that 
		you can set by an argument to the shell.

	To Test:

		No Dependencies:

			$ ./timetrash -t -n3 no_dep.sh    
				- minimum case: the ordering of the output does not vary but much since each tree 
				keeps having to wait for the previous until having enough available subprocesses to execute

			$ ./timetrash -t -n2 no_dep.sh
				- insufficient proc case: kills all processes when it realizes there are not 
				enough available processes listed in the constraint to execute the given script 
				since it requires 3 subprocesses due to having 2 pipes

			$ ./timetrash -t -n10 no_dep.sh
				- as we increase n, the ordering becomes more and more variable and approaches 
				the case where we do not specify the limit since with a lesser constraint the trees 
				all execute in parallel

		Dependencies:

			$ ./timetrash -t -n2 dep.sh
				- minimum case : the ordering of the output of the script is the same as the non 
				parallelized case since we only have enough processes to execute them in order

			$ ./timetrash -t -n10 dep.sh
				- as we increase n, the ordering becomes more and more variable and approaches 
				the case where we do not specify a limit with the n flag. The only constraint 
				here is that the dependent trees execute in order

					i.e. inorder1, inorder2, inorder3, inorder4 are output in the same relative 
					order however other output from no dependent trees is interspersed between 
					each of the in order outputs

	Solution/Method: 
		
		1. Maintain a global variable which represents the number of available 
		subprocesses that aren't in use.

		2. When we wish to execute a command tree we check how many subprocesses it needs to run.

		3. If the number of subprocesses is more than we have maxmimum then we exit with error 
		saying there will never be enough subprocesses to run it.

		4. If the number of subprocesses needed is less than or equal to the number of available 
		subprocesses then we can execute the tree.

		5. When we execute the tree make sure to decrement the number of available subprocesses 
		in the parent process.

		6. Then when that child finishes executing it exits with the number of processes it frees.

		7. Meanwhile the parent process is constantly checking to see if any of its children finished 
		executing and if they did then it uses its exit code to increment the number of available processes.

		8. If there is not currently enough available subprocesses to run a command tree then we just 
		continuously loop until finally the child process exits then we increment the number of 
		available processes so that we can then execute the next tree.