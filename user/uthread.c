#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

/* Possible states of a thread: */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4


struct context {
  /*  0 */ uint64 ra;
  /*  8 */ uint64 sp;
  /* 16 */ uint64 s0;
  /* 24 */ uint64 s1;
  /* 32 */ uint64 s2;
  /* 40 */ uint64 s3;
  /* 48 */ uint64 s4;
  /* 56 */ uint64 s5;
  /* 64 */ uint64 s6;
  /* 72 */ uint64 s7;
  /* 80 */ uint64 s8;
  /* 88 */ uint64 s9;
  /* 96 */ uint64 s10;
  /*104 */ uint64 s11;
}; //registers saved for thread switching
// note that only callee-saved registers are saved in context
// this is because the thread_switch() function, which performs the context switch,
struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct context context;        /* swtch() here to run thread */
};
struct thread all_thread[MAX_THREAD];
struct thread *current_thread;
extern void thread_switch(uint64, uint64);
              
void 
thread_init(void)
{
  // main() is thread 0, which will make the first invocation to
  // thread_schedule(). It needs a stack so that the first thread_switch() can
  // save thread 0's state.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void 
thread_schedule(void)
{
  struct thread *t, *next_thread;

  /* Find another runnable thread. */
  next_thread = 0;
  t = current_thread + 1;
  for(int i = 0; i < MAX_THREAD; i++){
    if(t >= all_thread + MAX_THREAD)
      t = all_thread;
    if(t->state == RUNNABLE) {
      next_thread = t;
      break;
    }
    t = t + 1;
  }

  if (next_thread == 0) {
    printf("thread_schedule: no runnable threads\n");
    exit(-1);
  }

  if (current_thread != next_thread) {         /* switch threads?  */
    next_thread->state = RUNNING;
    t = current_thread;
    current_thread = next_thread;
    /* YOUR CODE HERE
     * Invoke thread_switch to switch from t to next_thread:
     * thread_switch(??, ??);
     */
    thread_switch((uint64) &t->context, (uint64) &next_thread->context);
  } else
    next_thread = 0;
}

void 
thread_create(void (*func)()) //initiate a new thread, along with its stack and registers
{
  struct thread *t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  if (t == all_thread + MAX_THREAD) {
    printf("thread_create: no free threads\n");
    exit(-1);
  } // if no free slots, exit
  t->state = RUNNABLE;
  // YOUR CODE HERE
  t->context.sp = (uint64) t->stack + STACK_SIZE; // initialize the thread's stack pointer to the top of its stack
  t->context.ra = (uint64) func; // set the thread's return address to the function that the thread should execute
  t->context.s0 = t->context.s1 = t->context.s2 = t->context.s3 = t->context.s4 = t->context.s5 = t->context.s6 = t->context.s7 = t->context.s8 = t->context.s9 = t->context.s10 = t->context.s11 = 0; // initialize the thread's callee-saved registers to 0
  // printf("thread_create: created thread with stack pointer %p and return address %p\n", t->context.sp, t->context.ra);
}

void 
thread_yield(void)
{
  current_thread->state = RUNNABLE;
  thread_schedule();
}

volatile int a_started, b_started, c_started;
volatile int a_n, b_n, c_n;

void 
thread_a(void)
{
  int i;
  printf("thread_a started\n");
  a_started = 1;
  while(b_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_a %d\n", i);
    a_n += 1;
    thread_yield();
  }
  printf("thread_a: exit after %d\n", a_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_b(void)
{
  int i;
  printf("thread_b started\n");
  b_started = 1;
  while(a_started == 0 || c_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_b %d\n", i);
    b_n += 1;
    thread_yield();
  }
  printf("thread_b: exit after %d\n", b_n);

  current_thread->state = FREE;
  thread_schedule();
}

void 
thread_c(void)
{
  int i;
  printf("thread_c started\n");
  c_started = 1;
  while(a_started == 0 || b_started == 0)
    thread_yield();
  
  for (i = 0; i < 100; i++) {
    printf("thread_c %d\n", i);
    c_n += 1;
    thread_yield();
  }
  printf("thread_c: exit after %d\n", c_n);

  current_thread->state = FREE;
  thread_schedule();
}

int 
main(int argc, char *argv[]) 
{
  a_started = b_started = c_started = 0;
  a_n = b_n = c_n = 0;
  thread_init();
  thread_create(thread_a);
  thread_create(thread_b);
  thread_create(thread_c);
  current_thread = &all_thread[0];
  all_thread[0].state = RUNNING;
  current_thread->state = FREE;
  thread_schedule();
  exit(0);
}
