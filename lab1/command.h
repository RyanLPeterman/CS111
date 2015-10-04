#include <stdbool.h> // for boolean type

// UCLA CS 111 Lab 1 command interface

typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

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


bool is_valid(char character);
char* read_file_into_buffer(int (*get_next_byte) (void *), void *get_next_byte_argument);

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
