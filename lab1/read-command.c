#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

//////////////////////////////////////////////////////////////
////////////////////////  Definitions  ///////////////////////
//////////////////////////////////////////////////////////////

typedef struct token {
  token_type type;
  char* words;
} token;

bool is_valid(char character) {

  if(isalnum(character))
    return true;

  switch(character)
  {
    case '!':
    case '%':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case '@':
    case '^':
    case '_':
      return true;
    default:
      return false;
  }

}

// Progress: Done and Working
// Read file into buffer and preprocess the characters:
// Comments removed
char* read_file_into_buffer(int (*get_next_byte) (void *), void *get_next_byte_argument) 
{
  size_t capacity = 1024; // arbitrary size
  size_t iter = 0;
  char* buffer = (char*) checked_malloc(capacity);
  char next_byte;

  // loop through entire file
  while ((next_byte = get_next_byte(get_next_byte_argument)) != EOF) {

    // Comment: loop until newline
    if(next_byte == '#') {
      while ((next_byte = get_next_byte(get_next_byte_argument)) != '\n') 
      {
        // test
        // printf("%c", next_byte);
      }

      // get next byte disregard '\n'
      continue;
    }
  
    // store in buffer
    buffer[iter++] = next_byte;

    // if buffer full
    if (capacity == iter)
      buffer = (char*) checked_grow_alloc(buffer, &capacity);
  }

  // null terminate buffer 
  buffer[iter] = '\0';
  
  // test  
  // printf("%s", buffer);

  return buffer;
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /*
    General Method:
    1. Read Data into Buffer and Preprocess
    2. Create tokens and append into linked list
    3. Check to make sure tokens are valid
    4. Convert list of tokens into commands
  */

  // Read data into buffer and preprocess
  char* buffer = read_file_into_buffer(get_next_byte, get_next_byte_argument);


  error (1, 0, "End of function: make_command_stream");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented2");
  return 0;
}