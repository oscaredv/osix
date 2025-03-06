#include <i386/gdt.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/system.h>
#include <vm/pmap.h>

void *sched_esp;

void yield() {
  if (cur_proc == NULL)
    return;

  // Switch to scheduler stack
  swtch(&cur_proc->esp0, sched_esp);
}

void sched() {
  for (;;) {
    int nrunnable = 0;
    for (int p = 0; p < NPROC; p++) {
      if (proc[p].state == STATE_RUNNING || (proc[p].state == STATE_SLEEP && signal_pending(&proc[p]))) {
        ++nrunnable;
        cur_proc = &proc[p];

        // Set process kstack as return stack pointer in TSS
        tss_set_esp(cur_proc->kstack + STACK_SIZE);

        // Switch to process page directory
        pmap_activate(cur_proc);

        // Switch to process
        swtch(&sched_esp, cur_proc->esp0);

        cur_proc = NULL;
      }
    }

    if (nrunnable == 0) {
      // No runnable processes, halt CPU until next interrupt
      asm("sti");
      asm("hlt");
      asm("cli");
    }
  }
}
