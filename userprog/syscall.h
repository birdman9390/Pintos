#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"


void syscall_init (void);

#endif /* userprog/syscall.h */


struct lock file_lock;

struct child_process {
  int pid;
  int is_loaded;
  bool wait;
  bool exit;
  int status;
  struct semaphore load_sema;
  struct semaphore exit_sema;
  struct list_elem elem;
};

struct child_process* add_child_process (int pid);
struct child_process* get_child_process (int pid);

void remove_child_process (struct child_process *cp);

void syscall_close(int fd);
