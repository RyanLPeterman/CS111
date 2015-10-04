// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

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

//////////////////////////////////////////////////////////////
/////////////  Command Stream Implementation  ////////////////
//////////////////////////////////////////////////////////////

static command_stream_t m_command_stream;

typedef struct node {
    struct node* m_next;
    command_t m_dataptr;
  } stream_node;

typedef struct command_stream {
  stream_node* m_head;
} command_stream;


void add_command(command_t to_add_command) {

  // empty stream
  if(m_command_stream->m_head == 0) {
    m_command_stream->m_head = (stream_node*) malloc(sizeof(stream_node));
    m_command_stream->m_head->m_next = 0;
    m_command_stream->m_head->m_dataptr = to_add_command;
  }
  else // ! empty stream
  {
    stream_node* p = m_command_stream->m_head;
    // seek p to point to the last node
    for(; p->m_next != 0; p = p->m_next) {}

    p->m_next = (stream_node*) malloc(sizeof(stream_node));
    p->m_next = 0;
    p->m_dataptr = to_add_command;
  }
  return;
}

//////////////////////////////////////////////////////////////
//////////////////  Stack Implementation  ////////////////////
//////////////////////////////////////////////////////////////

typedef struct stack_node {
  void* m_dataptr;
  struct stack_node* m_next;
} stack_node;

typedef struct stack_t
{
  int m_count;
  stack_node* m_head;
  stack_node* m_tail;
  bool is_command_stack;
} stack_t;

void push(void* to_push, stack_t* stack) {

  // TODO: is there a way to remove command_stack in front of 
  // each of the variables?

  // empty stack
  if (stack->m_head == 0) {
    // Allocating memory for stack_node
    stack->m_head = (stack_node*) malloc(sizeof(stack_node));

    // Fill in stack_node and update head & tail
    stack->m_head->m_next = 0;
    stack->m_head->m_dataptr = to_push;
    stack->m_head = stack->m_tail;
  }
  else {
    // Allocating memory for stack_node
    stack->m_tail->m_next = (stack_node*) malloc(sizeof(stack_node));
    
    // Fill in stack_node and update tail
    stack->m_tail->m_dataptr = to_push;
    stack->m_tail->m_next = 0;
    stack->m_tail = stack->m_tail->m_next;
  }

  stack->m_count++;

  return;
}

void* pop(stack_t* stack) {

  if(stack->m_head == 0){
    error(1, 0, "cannot pop, stack is empty");
    return 0;
  }

  void* to_return = stack->m_tail->m_dataptr; 

  // free the last stack_node on the stack
  free(stack->m_tail);

  stack->m_count--;

  // restore the tail pointer to the end of the stack
  int i = 0;
  for(; i < (stack->m_count - 1); i++) {
    stack->m_tail = stack->m_head;

    stack->m_tail = stack->m_tail->m_next;
  }

  return to_return;
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
  printf("%s", buffer);

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
