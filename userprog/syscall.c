#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

typedef int pid_t;
static void syscall_handler (struct intr_frame *);
void syscall_halt();
void syscall_exit(int status);
<<<<<<< HEAD
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
=======
tid_t syscall_exec(const char *cmd_line);
int syscall_wait(tid_t _pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
void syscall_filesize();
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);
void syscall_seek();
void syscall_tell();
void syscall_close();

void get_argument (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
int get_kernel_pointer_addr(const void *vaddr);


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
  // this function is in the thread/vaddr.h
  if (is_kernel_vaddr(vaddr) || vaddr < (void*)0x08048000)
      syscall_exit(-1);
}
int get_kernel_pointer_addr(const void *vaddr)
{
  check_valid_ptr(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr){
      syscall_exit(-1); // error case
    }
  return (int) ptr;
}
>>>>>>> 13d2bd4b25bb5f610f4df8ac477ad2f5b74b7cef

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int arg[5];   // it takes argv[1], [2] ...
  // valid pointer check!!!!!!!!!!!!!!!! f->esp
  int * p = f->esp;
  int system_call = *p;

  // printf("%d\n\n",system_call);

  switch(system_call)
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
    // printf("%d\n\n",arg[0]);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);
    // printf("%d\n\n",arg[0]);
    // printf("%s\n\n",(const char*)arg[0]);

    f->eax = syscall_exec((const char*)arg[0]);
    break;
  case SYS_WAIT:
    get_argument(f, arg, 1);
    f->eax = syscall_wait(arg[0]);
    break;
  case SYS_CREATE:
    get_argument(f,arg,2);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);  // new file name
    arg[1] = (unsigned) arg[1];     // initial_size
    syscall_create(arg[0], arg[1]);
    break;
  case SYS_REMOVE:
    get_argument(f,arg,1);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]); // removing file name
    syscall_remove(arg[0]);
    break;
  case SYS_OPEN:
    get_argument(f,arg,1);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);
    syscall_open(arg[0]);
    break;
  case SYS_FILESIZE:
    syscall_filesize();
    break;
  case SYS_READ:
//    syscall_read(f);
    break;
  case SYS_WRITE:
    get_argument(f,arg,3);
    arg[0] = (int)arg[0];   // file descriptor
    arg[1] = get_kernel_pointer_addr((const void*) arg[1]); // file buffer (contents)
    arg[2] = (unsigned)arg[2]; // file size
    f->eax = syscall_write(arg[0], arg[1], arg[2]);
    break;
  case SYS_SEEK:
    syscall_seek();
    break;
  case SYS_TELL:
    syscall_tell();
    break;
  case SYS_CLOSE:
    syscall_close();
    break;
  dafault:
    printf("Error loading syscall, syscall num : %d\n",*((int*)f->esp));
    thread_exit ();
  }
}

void syscall_halt()
{
  shutdown_power_off();
}

void syscall_exit(int status)
{
  if(status == -1){
    //error 처리
  }
  printf ("%s: exit(%d)\n", thread_current()->name, status);
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
bool syscall_create (const char *file, unsigned initial_size)
{
  return filesys_create(file, initial_size);
}
bool syscall_remove (const char *file)
{
  return filesys_remove(file);
}
int syscall_open (const char *file)
{
printf("open\n");
}
void syscall_filesize()
{
printf("filesize\n");
}
int read (int fd, void *buffer, unsigned size){

}
int syscall_write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO){
    putbuf((char *)buffer, (size_t)size);
    return (int)size;
  }
  // Need to implement other fd case
  return -1;
}
void syscall_seek()
{
printf("seek\n");
}
void syscall_tell()
{
printf("tell\n");
}
void syscall_close()
{
<<<<<<< HEAD
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
=======
printf("close\n");
>>>>>>> 13d2bd4b25bb5f610f4df8ac477ad2f5b74b7cef
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
