#include <stdbool.h> // for boolean type

#define NEW_TREE_COMMAND 77
#define NEWLINE_COMMAND 11
#define RIGHT_PAREN_COMMAND 10
#define LEFT_PAREN_COMMAND 9
#define RIGHT_ARROW_COMMAND 8
#define LEFT_ARROW_COMMAND 7

/////////////////////////////////////////////////
///////////////  Token Definition  //////////////
/////////////////////////////////////////////////

typedef enum token_type
{
	WORD,
	SEMICOLON,
	PIPE,
	AND,
	OR,
	LEFT_PAREN,
	RIGHT_PAREN,
  LEFT_ARROW,
  RIGHT_ARROW,
  NEWLINE,
  UNKNOWN
} token_type;

typedef struct token token;
typedef struct token_list* token_list_t; 
typedef struct token_list token_list;

// Add token to linked list
void add_token(token* to_add, token_list_t* head);

// Free memory allocated to linked list
void free_token_list(token_list_t head);

// For debugging purposes
void print_token_list(token_list_t token_list);

/////////////////////////////////////////////////
//////////  Command Stream Definition  //////////
/////////////////////////////////////////////////

typedef struct command command;
typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

typedef struct node* node_t;
typedef struct node node;

// Constructor
void initialize_stream(command_stream_t m_command_stream);

// Add command to command stream
void add_command(command_t to_add_command, command_stream_t m_command_stream);

// Set cur pointer back to head
void reset_traverse(command_stream_t cStream);

command_t traverse(command_stream_t cStream);

// For debugging
void print_stream(command_stream_t cStream);

// Free memory allocated to stream
void free_stream(command_stream_t m_command_stream);

/////////////////////////////////////////////////
///////////////  Stack Definition  //////////////
/////////////////////////////////////////////////

typedef struct st_node st_node;
typedef struct st_node* st_node_t;

typedef struct stack stack;
typedef struct stack* stack_t;

// Constructor
stack_t init_stack();

// Add command onto stack
void push(command_t to_add, stack_t stack);

// Remove and return top command of stack
command_t pop(stack_t stack);

// Return top command on stack
command_t peek(stack_t stack);

// Check if stack is empty
bool isEmpty(stack_t stack);

// Free all memory allocated to stack
void free_stack(stack_t stack);

// Print out contents of stack
void print_stack(stack_t stack);

// Testing Data Structure
void test_stack();

/////////////////////////////////////////////////
/////////////  Additional Functions  ////////////
/////////////////////////////////////////////////

// Checks if passed in character matches characters allowed by the spec
bool is_valid_char(char character);

// Reads input file into a buffer while parsing out the comments
char* read_file_into_buffer(int (*get_next_byte) (void *), void *get_next_byte_argument);

// Creates a linked list of tokens given an input buffer
token_list_t convert_to_tokens(char* buffer);

// Checks passed in token_list to verify that token ordering is syntactically valid
void check_token_list(token_list_t token_list);

// Returns true if the type is an operator
bool is_operator(int type);

// Returns the precendence of a passed in operator
int get_precedence(int type);

// Initializes an empty command
command_t form_basic_command(int type);

// Takes token list and creates a command stream with no depth
command_stream_t make_basic_stream(token_list_t tList);

// Handles different newline cases
command_stream_t solve_newlines(command_stream_t nlStream); 

// Takes a simple command stream and turns it into a forest of command trees
command_stream_t make_advanced_stream(command_stream_t basic_stream);

/////////////////////////////////////////////////
////////////////  Given Functions  //////////////
/////////////////////////////////////////////////

/* Create a command stream from LABEL, GETBYTE, and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.  */
int command_status (command_t);