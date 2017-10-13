#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
void syscall_halt();
void syscall_exit(int status);
tid_t syscall_exec(const char *cmd_line);
int syscall_wait(tid_t _pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize(int fd);
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);

void get_argument (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
int get_kernel_pointer_addr(const void *vaddr);

struct file* get_file_by_fd(int fd);

static struct lock file_lock;


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

void
syscall_init (void)
{
  lock_init(&file_lock);
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
    get_argument(f,arg,1);
    syscall_filesize(arg[0]);
    break;
  case SYS_READ:
    get_argument(f,arg,3);
    arg[1]=get_kernel_pointer_addr((const void*)arg[1]);
    syscall_read(arg[0],arg[1],arg[2]);
    break;
  case SYS_WRITE:
    get_argument(f,arg,3);
    arg[0] = (int)arg[0];   // file descriptor
    arg[1] = get_kernel_pointer_addr((const void*) arg[1]); // file buffer (contents)
    arg[2] = (unsigned)arg[2]; // file size
    f->eax = syscall_write(arg[0], arg[1], arg[2]);
    break;
  case SYS_SEEK:
    get_argument(f,arg,2);
    arg[1]=(unsigned)arg[1];
    syscall_seek(arg[0],arg[1]);
    break;
  case SYS_TELL:
    get_argument(f,arg,1);
    syscall_tell(arg[0]);
    break;
  case SYS_CLOSE:
    get_argument(f,arg,1);
    syscall_close(arg[0]);
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
  pid_t p=process_execute(cmd_line);
//  printf("What is the return value? : %d\n",p);
  return p;  
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
  lock_acquire(&file_lock);
  return filesys_remove(file);
  lock_release(&file_lock);
}
int syscall_open (const char *file)
{
  lock_acquire(&file_lock);
  struct file *f=filesys_open(file);
  struct process_file* pf=malloc(sizeof(struct process_file));
  if(f==NULL)
  {
    lock_release(&file_lock);
    return -1;
  }
  pf->file=f;
  pf->fd=thread_current()->fd;
  thread_current()->fd++;
  list_push_back(&thread_current()->file_list,&pf->file_elem);
  lock_release(&file_lock);

  return pf->fd;
}
int syscall_filesize(int fd)
{
  lock_acquire(&file_lock);

  int size;
  struct list_elem *cur;
  struct file *f=get_file_by_fd(fd);

  if(f==NULL)
    size=-1;
  else
    size=file_length(f);

  lock_release(&file_lock);
  return size;
}
int syscall_read (int fd, void *buffer, unsigned size)
{
  lock_acquire(&file_lock);
  struct file* f=get_file_by_fd(fd);
  int bytes_read;
  if(fd==0)
  {
  lock_release(&file_lock);
  return 0;
  //get input by input_getc
  }
  else if(f!=NULL)
  {
    bytes_read=file_read(f,buffer,size);
    lock_release(&file_lock);
    return bytes_read;
  }
  else
  {
    lock_release(&file_lock);
    return -1;
  }
}
int syscall_write (int fd, const void *buffer, unsigned size)
{
  lock_acquire(&file_lock);
  struct file* f=get_file_by_fd(fd);
  int bytes_write;

  if (fd == STDOUT_FILENO){
    putbuf((char *)buffer, (size_t)size);
    lock_release(&file_lock);
    return (int)size;
  }
  else if(f!=NULL)
  {
    bytes_write=file_write(f,buffer,size);
    lock_release(&file_lock);
    return bytes_write;
  }
  else
  {
    lock_release(&file_lock);
    return -1;
  }
  // Need to implement other fd case
  return -1;
}
void syscall_seek(int fd, unsigned position)
{
  lock_acquire(&file_lock);
  struct file* f=get_file_by_fd(fd);

  if(f!=NULL)
    file_seek(f,position);
  lock_release(&file_lock);

}
unsigned syscall_tell(int fd)
{
  lock_acquire(&file_lock);
  struct file* f=get_file_by_fd(fd);
  unsigned p;

  if(f!=NULL)
    p=file_tell(f);
  else
    p=-1;
  lock_release(&file_lock);
  return p;
}
void syscall_close(int fd)
{
  lock_acquire(&file_lock);
  //Need to implement!

  lock_acquire(&file_lock);
}

struct file* get_file_by_fd(int fd)
{
  struct thread* t=thread_current();
  struct process_file *pf;
  struct list_elem *cur;

  for(cur=list_begin(&t->file_list);cur!=list_end(&t->file_list);cur=list_next(cur))
  {
    pf=list_entry(cur,struct process_file,file_elem);
    if(pf->fd==fd)
      return pf->file;
  }
  return NULL;
}

