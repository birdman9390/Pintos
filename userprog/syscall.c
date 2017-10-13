#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

typedef int pid_t;
static void syscall_handler (struct intr_frame *);
void syscall_halt();
void syscall_exit(int status);
pid_t syscall_exec(const char* cmd_line);
int syscall_wait(pid_t pid);
void syscall_create();
void syscall_remove();
void syscall_open();
void syscall_filesize();
void syscall_read();
void syscall_write();
void syscall_seek();
void syscall_tell();
void syscall_close();

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  switch(*((int*)f->esp))
  {
  case SYS_HALT:
    syscall_halt();
  case SYS_EXIT:
    syscall_exit(*((int*)f->esp+1));
  case SYS_EXEC:
  {
    char *str=*(char **)(f->esp+4);
    syscall_exec(str);
  }
  case SYS_WAIT:
    syscall_wait(*((int*)f->esp+1));
  case SYS_CREATE:
    syscall_create();
  case SYS_REMOVE:
    syscall_remove();
  case SYS_OPEN:
    syscall_open();
  case SYS_FILESIZE:
    syscall_filesize();
  case SYS_READ:
    syscall_read();
  case SYS_WRITE:
    syscall_write();
  case SYS_SEEK:
    syscall_seek();
  case SYS_TELL:
    syscall_tell();
  case SYS_CLOSE:
    syscall_close();
  dafault:
    printf("Error loading syscall, syscall num : %d\n",*((int*)f->esp));
  }
  thread_exit ();
}

void syscall_halt()
{
printf("halt\n");
  shutdown_power_off();
}
void syscall_exit(int status)
{
printf("exit\n");
  if(thread_current()->parent==NULL)
    thread_current()->parent->status=status;
  printf("%s: exit(%d)\n",thread_current()->name,status);
  thread_exit();
}
pid_t syscall_exec(const char* cmd_line)
{
printf("exec\n");
  pid_t pid=process_execute(cmd_line);
  return pid;
}
int syscall_wait(pid_t pid)
{
printf("wait\n");
  return process_wait(pid);
}
void syscall_create()
{
printf("create\n");
}
void syscall_remove()
{
printf("remove\n");
}
void syscall_open()
{
printf("open\n");
}
void syscall_filesize()
{
printf("filesize\n");
}
void syscall_read()
{
printf("read\n");
}
void syscall_write()
{
//printf("write\n");
}
void syscall_seek()
{
//printf("seek\n");
}
void syscall_tell()
{
//printf("tell\n");
}
void syscall_close()
{
//printf("close\n");
}
