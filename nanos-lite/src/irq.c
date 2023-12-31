#include "common.h"

_RegSet* schedule(_RegSet *prev);
_RegSet* do_syscall(_RegSet *r);
static _RegSet* do_event(_Event e, _RegSet* r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      do_syscall(r);
return schedule(r);
      break;
    case _EVENT_TRAP: 
      return schedule(r); 
    case _EVENT_IRQ_TIME: 
//	Log("The time irq is working!"); 
	return schedule(r); 
    default: panic("Unhandled event ID = %d", e.event);
  }

  return NULL;
}

void init_irq(void) {
  _asye_init(do_event);
}
