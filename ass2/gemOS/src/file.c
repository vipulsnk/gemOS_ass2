#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>
#include<pipe.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/
void free_file_object(struct file *filep)
{
    if(filep)
    {
       os_page_free(OS_DS_REG ,filep);
       stats->file_objects--;
    }
}

struct file *alloc_file()
{
  
  struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
  file->fops = (struct fileops *) (file + sizeof(struct file)); 
  bzero((char *)file->fops, sizeof(struct fileops));
  stats->file_objects++;
  return file; 
}

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
  kbd_read(buff);
  return 1;
}

static int do_write_console(struct file* filep, char * buff, u32 count)
{
  struct exec_context *current = get_current_ctx();
  return do_write(current, (u64)buff, (u64)count);
}

struct file *create_standard_IO(int type)
{
  struct file *filep = alloc_file();
  filep->type = type;
  if(type == STDIN)
     filep->mode = O_READ;
  else
      filep->mode = O_WRITE;
  if(type == STDIN){
        filep->fops->read = do_read_kbd;
  }else{
        filep->fops->write = do_write_console;
  }
  filep->fops->close = generic_close;
  filep->ref_count = 1;
  return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
   int fd = type;
   struct file *filep = ctx->files[type];
   if(!filep){
        filep = create_standard_IO(type);
   }else{
         filep->ref_count++;
         fd = 3;
         while(ctx->files[fd])
             fd++; 
   }
   ctx->files[fd] = filep;
   return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/



void do_file_fork(struct exec_context *child)
{
   /*TODO the child fds are a copy of the parent. Adjust the refcount*/
 
}

void do_file_exit(struct exec_context *ctx)
{
   /*TODO the process is exiting. Adjust the ref_count
     of files*/
}

long generic_close(struct file *filep)
{
  /** TODO Implementation of close (pipe, file) based on the type 
   * Adjust the ref_count, free file object
   * Incase of Error return valid Error code 
   * 
   */
    int ret_fd = -EINVAL; 
    return ret_fd;
}

static int do_read_regular(struct file *filep, char * buff, u32 count)
{
   /** TODO Implementation of File Read, 
    *  You should be reading the content from File using file system read function call and fill the buf
    *  Validate the permission, file existence, Max length etc  HOW??????
    *  Incase of Error return valid Error code HOW????????
    * */
    struct inode * inode1 = filep->inode;
    u32 ofst = filep->offp;
    int bytread = flat_read(inode1, buff, count, &ofst);
    filep->offp=bytread+filep->offp;
    int ret_fd = bytread; 
    return ret_fd;
}


static int do_write_regular(struct file *filep, char * buff, u32 count)
{
    /** TODO Implementation of File write, 
    *   You should be writing the content from buff to File by using File system write function
    *   Validate the permission, file existence, Max length etc
    *   Incase of Error return valid Error code 
    * */
    struct inode * inode1 = filep->inode;
    u32 ofst = filep->offp;
    int bytwrite = flat_write(inode1, buff, count, &ofst);
    filep->offp = filep->offp + bytwrite;
    int ret_fd = bytwrite; 
    return ret_fd;
}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{
    /** TODO Implementation of lseek 
    *   Set, Adjust the ofset based on the whence
    *   Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    return ret_fd;
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{ 
  /**  TODO Implementation of file open, 
    *  You should be creating file(use the alloc_file function to creat file), 
    *  To create or Get inode use File system function calls, 
    *  Handle mode and flags 
    *  Validate file existence, Max File count is 32, Max Size is 4KB, etc
    *  Incase of Error return valid Error code 
    * */

    // s2: find free fd
    int i=3; // looking from index 3
    while(ctx->files[i]){
      i++;
      //printf("looking for free fd, ind is %d\n", i);
    }
    //printf("free fd found: %d\n", i);
    int fd=i;


    // s3: allocating a file object 
    struct file * filep = alloc_file();

    if((flags & O_CREAT) == O_CREAT){
      // create a new file
      //printf("creating a new file\n");
      // s1: get inode
      struct inode * inode1 = create_inode(filename, mode);
      filep->inode = inode1;
    }else{
      // open an existing file
      // s1: look for the inode
      //printf("opening an existing file\n");
      struct inode* inode2 = lookup_inode(filename);
      if(inode2==NULL){
        // error occured
        //printf("error occured in opening file, inode is NULL\n");
        return -EINVAL; // return appropriate exit code 
      }else{
        // assuming no error
        u32 mode_inode;
        mode_inode = inode2->mode; // may be an error if mode is not assigned in inode
        // checking permissions
        if ((((flags & O_READ) == O_READ) && ((mode_inode & O_READ) != O_READ)) || (((flags & O_WRITE) == O_WRITE) && ((mode_inode & O_WRITE) != O_WRITE)) || (((flags & O_EXEC) == O_EXEC) && ((mode_inode & O_EXEC) != O_EXEC) )) {
          // permission denied
          return -EACCES;
        }else{
          //printf("correct permissions, good to go\n");
          filep->inode = inode2;
          inode2->ref_count=inode2->ref_count++; // new struct object created
        }
      }
      
    }
    // assuming no error
    // s4: filling the fields
    filep->pipe = NULL;
    filep->mode = flags; // assigning mode of opening :: ERROR mode = flags - OCREAT
    filep->type = REGULAR;
    filep->offp = 0;
    filep->fops->read = do_read_regular;
    filep->fops->write = do_write_regular;
    filep->fops->close = generic_close;
    ctx->files[fd] = filep;
    int ret_fd = fd; 
    return ret_fd;
}

int fd_dup(struct exec_context *current, int oldfd)
{
     /** TODO Implementation of dup 
      *  Read the man page of dup and implement accordingly 
      *  return the file descriptor,
      *  Incase of Error return valid Error code 
      * */
    int ret_fd = -EINVAL; 
    return ret_fd;
}


int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
  /** TODO Implementation of the dup2 
    *  Read the man page of dup2 and implement accordingly 
    *  return the file descriptor,
    *  Incase of Error return valid Error code 
    * */
    int ret_fd = -EINVAL; 
    return ret_fd;
}
