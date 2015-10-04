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

typedef struct token_list* token_list_t; 

typedef struct token_list {
  token m_token;
  token_list_t m_next;
  token_list_t m_prev;
} token_list;

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

//////////////////////////////////////////////////////////////
/////////////  Command Stream Implementation  ////////////////
//////////////////////////////////////////////////////////////

//CHANGES MADE:
//added an "m_curr" pointer to command_stream 
//	ideally will later on allow read_command_stream to iterate through list
//	allows add_command function to not have to seek to last node
//
//added an "initialize_stream" function that make_command_stream may eventually use
//old code is commented out not removed
//renamed "size" to "m_size"
//added traverse_stream method using "m_curr" pointer to iterate through the list

typedef struct node* node_t;

typedef struct node {
  command_t m_dataptr;
  node_t m_next;
} node;

//typedef struct command_stream *command_stream_t; (from command.h)
struct command_stream {
  node_t m_head;
  node_t curr;
  int m_size;
};

void initialize_stream(command_stream_t){
  m_command_stream->m_head = NULL;
  m_command_stream->m_curr = NULL;
}

void add_command(command_t to_add_command, command_stream_t m_command_stream) {
  // empty stream
  if(m_command_stream->m_head == NULL) {
    
    m_command_stream->m_curr = (node_t) checked_malloc(sizeof(node));
    m_command_stream->m_curr->m_dataptr = to_add_command;
    m_command_stream->m_curr->m_next = NULL;
    m_command_stream->m_head = m_command_stream->m_curr;
    
    
    //m_command_stream->m_head = (node_t) checked_malloc(sizeof(node));

    // initialize node
    //m_command_stream->m_head->m_dataptr = to_add_command;
    //m_command_stream->m_head->m_next = NULL;
    //m_command_stream->m_curr = m_command_stream->m_head;
  }
  else // ! empty stream
  {
    //node_t p = m_command_stream->m_head;
    // seek p to point to the last node
    //for(; p->m_next != NULL; p = p->m_next) {}
    
    
    //
    m_command_stream->m_curr->m_next = (node_t) checked_malloc(sizeof(node));
    m_command_stream->m_curr = m_command_stream->m_curr->m_next;
    
    m_command_stream->m_curr->m_dataptr = to_add_command;
    m_command_stream->m_curr->m_next = NULL;
    
    
    // initialize new node
    //p->m_next = (node_t) checked_malloc(sizeof(node));
    //p->m_next->m_next = NULL;
    //p->m_next->m_dataptr = to_add_command;
  }

  m_command_stream->m_size++;
  return;
}


// Converts input buffer into a linked list of tokens 
// This helps to categorize the inputs
token_list_t convert_to_tokens(char* buffer) {

  int iter = 0;
  char current;
  char next;

  // empty buffer case
  if(buffer[iter] == '\0')
    return NULL;

  // While we are not at the end of the buffer
   while (buffer[iter] != '\0') {

    current = buffer[iter++];
    next = buffer[iter];

    switch(current) {
      case '&':
      case '|':
      case ';':
      case '(':
      case ')':
      case '<':
      case '>':
      case '\n':
      case ' ':
      case '\t':
      default:
        break;
    }
  }


  return NULL;
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

  token_list_t token_list = convert_to_tokens(buffer);

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