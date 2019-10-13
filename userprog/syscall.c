#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
/* My Implementation */
#include "threads/vaddr.h"
#include "threads/init.h"
#include "userprog/process.h"
#include <list.h>
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "devices/input.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);


typedef int pid_t;

static int sys_write (int fd, const void *buffer, unsigned length);
static int sys_halt (void);

static struct file *find_file_by_fd (int fd);
static struct fd_elem *find_fd_elem_by_fd (int fd);
static int alloc_fid (void);
static struct fd_elem *find_fd_elem_by_fd_in_process (int fd);

typedef int (*handler) (uint32_t, uint32_t, uint32_t);
static handler syscall_vec[128];
static struct lock file_lock;



int
sys_exit (int status)
{
  
  struct thread *t;
  // // struct list_elem *l;
  
  t = thread_current ();
  
  
  t->ret_status = status;
  thread_exit ();
  return -1;
}

struct fd_elem
  {
    int fd;
    struct file *file;
    struct list_elem elem;
    struct list_elem thread_elem;
  };
  
static struct list file_list;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  syscall_vec[SYS_HALT] = (handler)sys_halt;
  syscall_vec[SYS_WRITE] = (handler)sys_write;
  syscall_vec[SYS_EXIT] = (handler)sys_exit;


  list_init (&file_list);
  lock_init (&file_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  handler h;
  int *p;
  int ret;
  
  p = f->esp;
  
  if (!is_user_vaddr (p))
    goto terminate;
  
  if (*p < SYS_HALT || *p > SYS_INUMBER)
    goto terminate;
  
  h = syscall_vec[*p];
  
  if (!(is_user_vaddr (p + 1) && is_user_vaddr (p + 2) && is_user_vaddr (p + 3)))
    goto terminate;
  
  ret = h (*(p + 1), *(p + 2), *(p + 3));
  
  f->eax = ret;
  
  return;
  
terminate:
  sys_exit (-1);

}


static int sys_halt(void)
{
	power_off();
}

static int
sys_write (int fd, const void *buffer, unsigned length)
{
  struct file * f;
  int ret;
  
  ret = -1;
  lock_acquire (&file_lock);
  if (fd == STDOUT_FILENO) /* stdout */
    putbuf (buffer, length);
  
  lock_release (&file_lock);
  return ret;
}



