// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

// TODO don't forget to call free at the end of the program to free 
// all the memory (do it in a remove function) 

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

//////////////////////////////////////////////////////////////
//////////////////  Stack Implementation  ////////////////////
//////////////////////////////////////////////////////////////

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  // These data structures will be important later
  stack_t* command_stack = (stack_t*) malloc(sizeof(stack_t));
  stack_t* operator_stack = (stack_t*) malloc(sizeof(stack_t));
  command_stack->is_command_stack = true;
  operator_stack->is_command_stack = false;
  
  // Grab bytes to fill buffer
  char next_byte = (*get_next_byte)(get_next_byte_argument);
  int capacity = 1000;
  char* buffer[capacity];
  int i = 0;

  while (next_byte != EOF) {
    // TODO: Casting not working here (getc returns int)
    buffer[i] = (char) next_byte;
    i++;
    // buffer is too small for next byte
    if (i + 1 > capacity) {
      buffer = (char*) checked_realloc(buffer, capacity*2);
      capacity *= 2;
    }
    next_byte = (*get_next_byte)(get_next_byte_argument);
  }

  // error (1, 0, "command reading not yet implemented1");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented2");
  return 0;
}
