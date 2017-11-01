#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

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
void syscall_close(int fd);

void get_argument (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
int get_kernel_pointer_addr(const void *vaddr);
struct file* get_file_fd(int fd);

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
    arg[0] = get_kernel_pointer_addr((const void*)arg[0]);  // file
    syscall_open(arg[0]);
    break;
  case SYS_FILESIZE:
    get_argument(f,arg,1);
    syscall_filesize(arg[0]);     // file descriptor
    break;
  case SYS_READ:
    get_argument(f,arg,3);
    // should check valid buffer // for bad-read case
    arg[0] = (int) arg[0];      // file descriptor
    arg[1] = get_kernel_pointer_addr((const void*)arg[1]);  // file buffer
    arg[2] = (unsigned) arg[2];
    f->eax = syscall_read(arg[0],arg[1],arg[2]);
    break;
  case SYS_WRITE:
    get_argument(f,arg,3);
    // should check valid buffer // for bad-write case
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
  struct file *f = filesys_open(file);  // file open
  int fd;
  if(!f){
    return -1;
  }  // file is not open
  else {
    struct file_element *file_pointer = malloc(sizeof(struct file_element));
    file_pointer->file = f;
    file_pointer->fd = thread_current()->fd;
    thread_current()->fd += 1;
    // single file이 두번 이상 불리는 경우도 처리. // close도 독립적
    list_push_back(&thread_current()->file_list, &file_pointer->elem);
    // add file to the file_list of thread
    return file_pointer->fd;
  }
  return fd;
}
int syscall_filesize (int fd)
{
  struct file *f = get_file_fd(fd);
  if(f == NULL){
    return -1;      // file cannot open
  } else {
    return file_length(f);
  }
}
int syscall_read (int fd, void *buffer, unsigned size){
  // size 만큼을 읽어서 buffer에 쓴다.
  if(fd == STDIN_FILENO){   // fd ==0 , keyboard case
      int i = 0;
      char* buf = (char*) buffer;  //buffer pointer
      for(;i<size;i++){
        buf[i] = input_getc();
      }
      return size;
  }
  else {
      struct file *f = get_file_fd(fd);
      if(f == NULL){
        return -1;
      }
      else {
        return file_read(f, buffer, size);  // file.h
      }
  }
}

int syscall_write (int fd, const void *buffer, unsigned size)
{
  if (fd == STDOUT_FILENO){
    putbuf((char *)buffer, (size_t)size);     // console writing case
    return (int)size;
  }
  else {
    struct file *f = get_file_fd(fd);
    if(f == NULL){
      return -1;
    }
    else {
      return file_write(f,buffer,size);
    }
  }
}

void syscall_seek(int fd, unsigned position){
  struct file *f = get_file_fd(fd);
  if(f == NULL){
    return;
  } else {
    return file_seek(f, position);      // file 이 끝부분 뒤를 읽는 예외는 없다.
  }
}

unsigned syscall_tell(int fd){
  struct file *f = get_file_fd(fd);
  if(f == NULL){
    return;
  } else {
    return file_tell(f);      // off_t -> 시작부터 센 bytes 위치.
  }
}
void syscall_close(int fd){
  struct thread *t = thread_current();
  struct list_elem *e;

  for(e = list_begin(&t->file_list); e != list_end(&t->file_list);e = list_next(e)){
    struct file_element *file_pointer = list_entry(e, struct file_element,elem);
    if(fd == file_pointer->fd){
      file_close(file_pointer->file);
      list_remove(&file_pointer->elem);
      free(file_pointer);
    }
    // process exit 의 경우에는 모든 fd를 지워준다.
    //(all closing case) should be added
  }
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
int get_kernel_pointer_addr(const void *vaddr)
{
  check_valid_ptr(vaddr);
  void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
  if (!ptr){
      syscall_exit(-1); // error case
    }
  return (int) ptr;
}
