#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
void syscall_halt();
void syscall_exit(int status);
tid_t syscall_exec(const char *cmd_line);
int syscall_wait(tid_t _pid);
// void syscall_create();
// void syscall_remove();
// void syscall_open();
// void syscall_filesize();
// void syscall_read();
// void syscall_write();
// void syscall_seek();
// void syscall_tell();
// void syscall_close();

void get_argument (struct intr_frame *f, int *arg, int n);

void check_valid_ptr (const void *vaddr);


void get_argument (struct intr_frame *f, int *arg, int n)
{
  int i;
  int *ptr;
  for (i = 0; i < n; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_ptr((const void *) ptr);
      arg[i] = *ptr;
    }
}
void check_valid_ptr (const void *vaddr)
{
  if (!is_user_vaddr(vaddr) || vaddr < (void*)0x08048000)
    {
      syscall_exit(-1);
    }
}
void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int arg[5];
  // valid pointer check!!!!!!!!!!!!!!!! f->esp

  switch(*((int*)f->esp))
  {
  case SYS_HALT:
    syscall_halt();
    break;
  case SYS_EXIT:
    get_argument(f, arg, 1);
    syscall_exit(arg[0]);
    break;
  case SYS_EXEC:
    get_argument(f, arg, 1);
    // user pointer ->  kernel pointer?
    // should I change eax?
    syscall_exec(arg[0]);
    break;
  case SYS_WAIT:
    get_argument(f, arg, 1);
    syscall_wait(arg[0]);
    break;
  // case SYS_CREATE:
  //   syscall_create();
  // case SYS_REMOVE:
  //   syscall_remove();
  // case SYS_OPEN:
  //   syscall_open();
  // case SYS_FILESIZE:
  //   syscall_filesize();
  // case SYS_READ:
  //   syscall_read();
  // case SYS_WRITE:
  //   syscall_write();
  // case SYS_SEEK:
  //   syscall_seek();
  // case SYS_TELL:
  //   syscall_tell();
  // case SYS_CLOSE:
  //   syscall_close();
  dafault:
    printf("Error loading syscall, syscall num : %d\n",*((int*)f->esp));
  }
  thread_exit ();
}

void syscall_halt()
{
  shutdown_power_off();
}

void syscall_exit(int status)
{
  thread_exit();
}

tid_t syscall_exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

int syscall_wait(tid_t _pid)
{
  return process_wait(_pid);      // pid of child process. will start
}
// void syscall_create()
// {
// printf("create\n");
// }
// void syscall_remove()
// {
// printf("remove\n");
// }
// void syscall_open()
// {
// printf("open\n");
// }
// void syscall_filesize()
// {
// printf("filesize\n");
// }
// void syscall_read()
// {
// printf("read\n");
// }
// void syscall_write()
// {
// printf("write\n");
// }
// void syscall_seek()
// {
// printf("seek\n");
// }
// void syscall_tell()
// {
// printf("tell\n");
// }
// void syscall_close()
// {
// printf("close\n");
// }
