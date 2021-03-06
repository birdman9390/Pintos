#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

#include "userprog/process.h"
#include "devices/shutdown.h"

struct file_element {
  struct file *file;
  int fd;
  struct list_elem elem;
};

static void syscall_handler (struct intr_frame *);
void syscall_halt();
void syscall_exit(int status);
tid_t syscall_exec(const char *cmd_line);
int syscall_wait(tid_t _pid);
bool syscall_create (const char *file, unsigned initial_size);
bool syscall_remove (const char *file);
int syscall_open (const char *file);
int syscall_filesize (int fd);
int syscall_read (int fd, void *buffer, unsigned size);
int syscall_write (int fd, const void *buffer, unsigned size);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);

void cp_load_check(struct child_process* cp);
void get_argument (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
int get_kernel_pointer_addr(const void *vaddr);
struct file* get_file_fd(int fd);

void
syscall_init (void)
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int arg[3];   // it takes argv[1], [2], [3] - maximum number of argu 3

  // int * p = f->esp;
  //check_valid_ptr((const void*)p);     // valid pointer check! - for sc-bad-sp.ck case
  int p = get_kernel_pointer_addr((const void*)f->esp);
  int system_call = *(int*)p;
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
    arg[0] = (const char*) get_kernel_pointer_addr((const void*)arg[0]);
    f->eax = syscall_exec(arg[0]);
    break;
  case SYS_WAIT:
    get_argument(f, arg, 1);
    f->eax = syscall_wait(arg[0]);
    break;
  case SYS_CREATE:
    get_argument(f,arg,2);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);  // new file
    arg[1] = (unsigned) arg[1];     // initial_size
    f->eax = syscall_create(arg[0], arg[1]);
    break;
  case SYS_REMOVE:
    get_argument(f,arg,1);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]); // get the file
    f->eax = syscall_remove(arg[0]);
    break;
  case SYS_OPEN:
    get_argument(f,arg,1);
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);  // file
    f->eax = syscall_open(arg[0]);
    break;
  case SYS_FILESIZE:
    get_argument(f,arg,1);
    f->eax = syscall_filesize(arg[0]);     // file descriptor
    break;
  case SYS_READ:
    get_argument(f,arg,3);
    check_valid_buffer((void*)arg[1],(unsigned)arg[2]); // for bad-read case
    arg[0] = (int) arg[0];      // file descriptor
    arg[1] = get_kernel_pointer_addr((const void*)arg[1]);  // file buffer
    arg[2] = (unsigned) arg[2];
    f->eax = syscall_read(arg[0],arg[1],arg[2]);
    break;
  case SYS_WRITE:
    get_argument(f,arg,3);
    check_valid_buffer((void*)arg[1],(unsigned)arg[2]);// for bad-write case
    arg[0] = (int)arg[0];   // file descriptor
    arg[1] = get_kernel_pointer_addr((const void*) arg[1]); // file buffer (contents)
    arg[2] = (unsigned)arg[2]; // file size
    f->eax = syscall_write(arg[0], arg[1], arg[2]);
    break;
  case SYS_SEEK:
    get_argument(f,arg,2);
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
  struct thread *cur = thread_current();
  if(thread_alive(cur->parent) && cur->cp){
    cur->cp->status=status;
  }
  printf ("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

tid_t syscall_exec(const char *cmd_line)
{
  child_is_loaded=load_unloaded;

  tid_t t=process_execute(cmd_line);
  // load안 된 경우 체크.
  while(child_is_loaded==load_unloaded)
    barrier();
  if(child_is_loaded==load_fail)
    return -1;

  struct child_process* cp = get_child_process(t);
  if(cp==NULL)
     return -1;

  cp_load_check(cp);

  if(cp->is_loaded==load_fail){
      remove_child_process(cp);
     return -1;
   }
  else
     return t;
}

void cp_load_check(struct child_process* cp){
  if(cp->is_loaded == 0){
    sema_down(&cp->load_sema);        // OR using lock
  }
  return;
}

int syscall_wait(tid_t _pid)
{
  return process_wait(_pid);      // pid of child process. will start
}
bool syscall_create (const char *file, unsigned initial_size)
{
  lock_acquire(&file_lock);
  bool result = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return result;
}

bool syscall_remove (const char *file)
{
  lock_acquire(&file_lock);
  bool result = filesys_remove(file);
  lock_release(&file_lock);
  return result;
}

int syscall_open (const char *file)
{
  int returnVal;
  lock_acquire(&file_lock);
  struct file *f = filesys_open(file);  // file open
  if(!f){
    returnVal = -1;
  }  // file is not open
  else {
    struct file_element *file_pointer = malloc(sizeof(struct file_element));
    if(!file_pointer){
      return -1;
    }
    file_pointer->file = f;
    file_pointer->fd = thread_current()->fd;
    thread_current()->fd += 1;
    // single file이 두번 이상 불리는 경우도 처리. // close도 독립적
    list_push_back(&thread_current()->file_list, &file_pointer->elem);

    returnVal = file_pointer->fd;
  }
  lock_release(&file_lock);
  return returnVal;
}
int syscall_filesize (int fd)
{
  int returnVal;
  lock_acquire(&file_lock);
  struct file *f = get_file_fd(fd);
  if(f == NULL){
    returnVal = -1;      // file cannot open
  } else {
    returnVal = file_length(f);
  }
  lock_release(&file_lock);
  return returnVal;
}
int syscall_read (int fd, void *buffer, unsigned size){
  // size 만큼을 읽어서 buffer에 쓴다.
  int returnVal;

  if(fd == STDIN_FILENO){   // fd ==0 , keyboard case
      int i = 0;
      char* buf = (char*) buffer;  //buffer pointer
      for(;i<size;i++){
        buf[i] = input_getc();
      }
      returnVal = size;
  }
  else {
      lock_acquire(&file_lock);
      struct file *f = get_file_fd(fd);
      if(f == NULL){
        returnVal = -1;
      }
      else {
        returnVal = file_read(f, buffer, size);  // file.h
      }
      lock_release(&file_lock);
  }
  return returnVal;
}

int syscall_write (int fd, const void *buffer, unsigned size)
{
  int returnVal;

  if (fd == STDOUT_FILENO){
    putbuf((char *)buffer, (size_t)size);     // console writing case
    returnVal = (int)size;
  }
  else {
    lock_acquire(&file_lock);
    struct file *f = get_file_fd(fd);
    if(f == NULL){
      returnVal = -1;
    }
    else {
      returnVal = file_write(f,buffer,size);
    }
    lock_release(&file_lock);
  }
  return returnVal;
}

void syscall_seek(int fd, unsigned position){
  lock_acquire(&file_lock);
  struct file *f = get_file_fd(fd);
  if(f == NULL){}
  else {
    file_seek(f, position);      // file 이 끝부분 뒤를 읽는 예외는 없다.
  }
  lock_release(&file_lock);
}

unsigned syscall_tell(int fd){
  unsigned returnVal;
  lock_acquire(&file_lock);
  struct file *f = get_file_fd(fd);
  if(f == NULL){
    returnVal = -1;
  } else {
    returnVal = file_tell(f);      // off_t -> 시작부터 센 bytes 위치.
  }
  lock_release(&file_lock);
  return returnVal;
}
void syscall_close(int fd){
  lock_acquire(&file_lock);
  struct thread *t = thread_current();
  struct list_elem *e = list_begin(&t->file_list);
  struct list_elem *next;

  for(e = list_begin(&t->file_list); e != list_end(&t->file_list);e = list_next(e)){
    struct file_element *file_pointer = list_entry(e, struct file_element,elem);
    // if(file_pointer==NULL){
    //   break;
    // }
    if(fd == file_pointer->fd || fd == -1){
      file_close(file_pointer->file);
      list_remove(&file_pointer->elem);
      free(file_pointer);
      break;
    }
    if(fd != -1){
      return;
    }
    // process exit 의 경우에는 모든 fd를 지워준다.
    //(all closing case) should be added
  }
  lock_release(&file_lock);
}

struct file* get_file_fd(int fd){
  struct thread *t = thread_current();
  struct list_elem *e;

  for(e = list_begin(&t->file_list); e != list_end(&t->file_list);e = list_next(e)){
    struct file_element *file_pointer = list_entry(e, struct file_element,elem);
    // file_list 중 fd가 parameter로 들어온 fd와 같으면 해당 파일 return
    if(fd = file_pointer->fd){
      return file_pointer->file;
    }
  }
  return NULL;
}


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

void check_valid_buffer (void* buffer, int size){
  char *buf = (char *)buffer;
  int i=0;
  for(;i<size;i++){
    check_valid_ptr((const void*)buf);
    buf += 1;   //next buffer
  }
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


struct child_process* add_child_process (int pid)
{
  struct child_process* cp = malloc(sizeof(struct child_process));
  if (!cp){
      return NULL;
  }
  cp->pid = pid;
  cp->is_loaded = 0;
  cp->wait = false;
  cp->exit = false;
  sema_init(&cp->load_sema, 0);
  sema_init(&cp->exit_sema, 0);
  // init and add child_list
  list_push_back(&thread_current()->child_list, &cp->elem);

  return cp;
}


struct child_process* get_child_process (int pid)
{
  struct thread *t = thread_current();
  struct list_elem *e;

  for (e = list_begin (&t->child_list); e != list_end (&t->child_list); e = list_next (e)){
      struct child_process *cp = list_entry (e, struct child_process, elem);
      if (pid == cp->pid){
	      return cp;
	    }
  }
  return NULL;
}
void remove_child_process (struct child_process *cp)
{
  list_remove(&cp->elem);
  free(cp);
}
