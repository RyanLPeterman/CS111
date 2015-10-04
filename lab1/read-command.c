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
  int lin_num;
} token;

typedef struct token_list* token_list_t; 

typedef struct token_list {
  token m_token;
  token_list_t m_next;
  token_list_t m_prev;
} token_list;

void add_token(token to_add, token_list_t head) {

  // empty list
  if(head == NULL) {
    head = (token_list_t) checked_malloc(sizeof(token_list));

    // initialize node
    head->m_token = to_add;
    head->m_next = NULL;
  }
  else // !empty list
  {
    token_list_t p = head;

    // seek p to point to the last node
    for(; p->m_next != NULL; p = p->m_next) {}

    // initialize new node
    p->m_next = (token_list_t) checked_malloc(sizeof(token_list));
    p->m_next->m_next = NULL;
    p->m_next->m_prev = p;
    p->m_next->m_token = to_add;
  }

  return;
}

bool is_valid_char(char character) {

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
typedef struct node* node_t;

typedef struct node {
  command_t m_dataptr;
  node_t m_next;
} node;

typedef struct command_stream {
  node_t m_head;
  int size;
} command_stream;


void add_command(command_t to_add_command, command_stream_t m_command_stream) {

  // empty stream
  if(m_command_stream->m_head == NULL) {
    m_command_stream->m_head = (node_t) checked_malloc(sizeof(command_stream));

    // initialize node
    m_command_stream->m_head->m_dataptr = to_add_command;
    m_command_stream->m_head->m_next = NULL;
  }
  else // ! empty stream
  {
    node_t p = m_command_stream->m_head;
    // seek p to point to the last node
    for(; p->m_next != NULL; p = p->m_next) {}

    // initialize new node
    p->m_next = (node_t) checked_malloc(sizeof(node));
    p->m_next->m_next = NULL;
    p->m_next->m_dataptr = to_add_command;
  }

  m_command_stream->size++;
  return;
}

// Converts input buffer into a linked list of tokens 
// This helps to categorize the inputs
token_list_t convert_to_tokens(char* buffer) {

  // list info
  token_list_t m_head = NULL;
  token_list_t p = m_head;

  // token info
  token_type type;
  int lin_num;
  
  // iteration variables
  int iter = 0;
  char current;
  char next;

  // empty buffer case
  if(buffer[iter] == '\0')
    return NULL;

  // While we are not at the end of the buffer
  while (buffer[iter] != '\0') {

    current = buffer[iter];
    next = buffer[iter + 1];

    switch(current) {
      case '&':
        // correctly have '&&'
        if(current == next) {
          type = AND;
          iter++; // inc to account for one '&'
        }
        else
          type = UNKNOWN;

        break;

      case '|':
        if(current == next) {
          type = OR;
          iter++;
        }
        else
          type = PIPE;

        break;

      case ';':
        type = SEMICOLON;
        break;

      case '(':
        type = LEFT_PAREN;
        break;

      case ')':
        type = RIGHT_PAREN;
        break;

      case '<':
        type = LEFT_ARROW;
        break;

      case '>':
        type = RIGHT_ARROW;
        break;

      // skip over newlines and increment linum
      case '\n':
        type = NEWLINE;

        // advance iter to first nonnewline char
        while(buffer[iter] == '\n') {
          iter++;
          lin_num++; 
        }

        // move back one to account for addition at the end
        iter--;
        break;

      // skip over whitespace
      case ' ':
      case '\t':
        iter++;
        continue;
      default:
        type = UNKNOWN;
        break;
    }

    // Word Case
    int len = 1;
    int word_begin = iter;
    if (is_valid_char(current)) {
      type = WORD;
      // loop through word and keep track of length
      for(; is_valid_char(buffer[iter + len]); len++) {}

      // advance iterator to last character of word
      iter += len - 1;
    }

    // Now add token to linked list
    token* temp_token = (token*) checked_malloc(sizeof(token));
    temp_token->type = type;
    temp_token->lin_num = lin_num;

    // Print error message
    if (type == UNKNOWN) {
      fprintf(stderr, "Error: Line %i: Unknown Token -> %c \n", lin_num, current);
      exit(1);
    }
    else if (type == WORD) {
      // Allocate memory for full string
      temp_token->words = (char*) checked_malloc((sizeof(char) * len) + 1); //+1 for '\0'
      int i = 0;

      // Store characters
      for(; i < len; i++) {
        temp_token->words[i] = buffer[word_begin + i];
      }

      // Null terminate string
      temp_token->words[i] = '\0';
    }
    else 
      temp_token->words = NULL;
  
    add_token(*temp_token, m_head);
    iter++;
  }

  return m_head;
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