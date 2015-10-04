

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