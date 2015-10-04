#include <stdbool.h> // for boolean type

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
  COMMENT,
  NEWLINE,
  UNKNOWN
} token_type;

typedef struct token token;
typedef struct token_list* token_list_t; 
typedef struct token_list token_list;


void add_token(token to_add, token_list_t head);

/////////////////////////////////////////////////
//////////  Command Stream Definition  //////////
/////////////////////////////////////////////////

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

typedef struct node* node_t;
typedef struct node node;

void initialize_stream(command_stream_t);
void add_command(command_t to_add_command, command_stream_t m_command_stream);

/////////////////////////////////////////////////
/////////////  Additional Functions  ////////////
/////////////////////////////////////////////////

bool is_valid(char character);

char* read_file_into_buffer(int (*get_next_byte) (void *), void *get_next_byte_argument);
token_list_t convert_to_tokens(char* buffer);

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
