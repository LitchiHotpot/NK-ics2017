#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);
	printf("load process %d\n",i);
  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  //_switch(&pcb[i].as);
  //current = &pcb[i];
  //((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

#define PROCWEIGHT 200
int prioritycounter = 0;
int current_game=0;

_RegSet* schedule(_RegSet *prev) {
  // save the context pointer

  static unsigned char switch_count = 0;

  if (current == &pcb[2]) {

    current->tf = prev;

    current = &pcb[current_game];

  }

  else if (switch_count++ == 0) {

    current->tf = prev;

    current = &pcb[0];

  }

  else {

    return NULL;

  }


  // current = &pcb[0];

  // current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  _switch(&current->as);

  return current->tf;                                                                         
}
