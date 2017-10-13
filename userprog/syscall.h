#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/thread.h"

void syscall_init (void);

//struct lock file_lock;
struct process_file{
  int fd;
  struct file *file;
  struct list_elem file_elem;
};


#endif /* userprog/syscall.h */
