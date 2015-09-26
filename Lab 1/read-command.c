// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

typedef struct stack_t
{
  int m_count;
  node* m_head;
  node* m_tail;
  bool is_command_stack;

  typedef struct node {
    void* m_dataptr;
    node* m_next;
  };
};

// TODO: is this dangerous programming?
static stack_t* command_stack;
static stack_t* operator_stack;
command_stack->is_command_stack = true;
operator_stack->is_command_stack = false;

void push(void* to_push) {

  // TODO: is there a way to remove command_stack in front of 
  // each of the variables?

  // empty stack
  if (command_stack->m_head == null) {
    // Allocating memory for node
    command_stack->m_head = (node*) malloc(sizeof(node)));

    // Fill in node and update head & tail
    command_stack->m_head->m_next = null;
    command_stack->m_head->m_dataptr = to_push;
    command_stack->m_head = command_stack->m_tail;
  }
  else {
    // Allocating memory for node
    command_stack->m_tail->m_next = (node*) malloc(sizeof(node)));
    
    // Fill in node and update tail
    command_stack->m_tail->m_dataptr = to_push;
    command_stack->m_tail->m_next = null;
    command_stack->m_tail = command_stack->m_tail->m_next;
  }

  command_stack->m_count++;

  return;
}

void* pop() {

  if(command_stack->m_head == null){
    error(1, 0, "cannot pop, stack is empty");
    return;
  }

  void* to_return = command_stack->m_tail->m_dataptr; 

  // free the last node on the stack
  free(command_stack->m_tail);

  command_stack->m_count--;

  // restore the tail pointer to the end of the stack
  for(int i = 0; i < (command_stack->m_count - 1); i++) {
    command_stack->m_tail = command_stack->m_head;

    command_stack->m_tail = command_stack->m_tail->m_next;
  }

  return to_return;
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* Method: TODO implement the stream which should be a linked list
  of characters to be processed in read_command_stream

  TODO: repurpose the stack which is already a working linked list
  to work for the stream struct as well potentially to reuse the code
  */
  
  char next_byte = (*get_next_byte)(get_next_byte_argument);


  error (1, 0, "command reading not yet implemented1");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented2");
  return 0;
}
