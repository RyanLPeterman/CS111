// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

//////////////////////////////////////////////////////////////
/////////////  Command Stream Implementation  ////////////////
//////////////////////////////////////////////////////////////

struct command_stream {
  stream_node* m_head;

  typedef struct stream_node {
    stream_node* m_next;
    command* m_dataptr;
  };
};


void add_command(command* to_add_command) {

  // empty stream
  if(m_head == null) {
    m_head = (stream_node*) malloc(sizeof(stream_node));
    m_head->m_next = null;
    m_head->m_dataptr = to_add_command;
  }
  else // ! empty stream
  {
    stream_node* p = m_head;
    // seek p to point to the last node
    for(; p->m_next != null; p = p->m_next) {}

    p->m_next = (stream_node*) malloc(sizeof(stream_node));
    p->m_next = null;
    p->m_dataptr = to_add_command;
  }
  return;
}

//////////////////////////////////////////////////////////////
//////////////////  Stack Implementation  ////////////////////
//////////////////////////////////////////////////////////////

typedef struct stack_t
{
  int m_count;
  stack_node* m_head;
  stack_node* m_tail;
  bool is_command_stack;

  typedef struct stack_node {
    void* m_dataptr;
    stack_node* m_next;
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
    // Allocating memory for stack_node
    command_stack->m_head = (stack_node*) malloc(sizeof(stack_node)));

    // Fill in stack_node and update head & tail
    command_stack->m_head->m_next = null;
    command_stack->m_head->m_dataptr = to_push;
    command_stack->m_head = command_stack->m_tail;
  }
  else {
    // Allocating memory for stack_node
    command_stack->m_tail->m_next = (stack_node*) malloc(sizeof(stack_node)));
    
    // Fill in stack_node and update tail
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

  // free the last stack_node on the stack
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
  /* Method: TODO implement linked list that 
  is the command forest the TA talked about in lecture
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
