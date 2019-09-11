#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
/***********************************************************************
 * Use this function to allocate pipe info && Don't Modify below function
 ***********************************************************************/
struct pipe_info* alloc_pipe_info()
{
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    pipe->pipe_buff = buffer;
    return pipe;
}


void free_pipe_info(struct pipe_info *p_info)
{
    if(p_info)
    {
        os_page_free(OS_DS_REG ,p_info->pipe_buff);
        os_page_free(OS_DS_REG ,p_info);
    }
}
/*************************************************************************/
/*************************************************************************/


int pipe_read(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Read the contect from buff (pipe_info -> pipe_buff) and write to the buff(argument 2);
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    /*
    if(!filep){
        return -EINVAL;
    }
    int i=0;
    char * w_ptr = filep->pipe->pipe_buff;
    int s_rd = filep->pipe->read_pos;
    int s_wr = filep->pipe->write_pos;
    while(i<count){
        if(s_rd>=s_wr){
            return -1*s_rd; // lw ahead
        }
        buff[i]=w_ptr[s_rd];
        i++;
        s_rd++;
        filep->pipe->buffer_offset--;
    }
    filep->pipe->read_pos=s_rd; // start reading from this position next time
    int ret_fd = -EINVAL; 
    return s_rd;
    */

   // cyclic implementation
    if(!filep){
        return -EINVAL;
    }
    int i=0;
    while(i<count){
        if(filep->pipe->buffer_offset == 0){
            return -EINVAL; // memory empty, nothing to read
        }else{
            if(filep->pipe->read_pos == 4096){ // exceed => cyclicity introduced
                filep->pipe->read_pos = 0;
            }
            //common updation in both cases
            buff[i] = filep->pipe->pipe_buff[filep->pipe->read_pos];
            i++;
            filep->pipe->buffer_offset--;
            filep->pipe->read_pos++;
        }
    }
    return filep->pipe->read_pos; // for checking
}


int pipe_write(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Write the content from   the buff(argument 2);  and write to buff(pipe_info -> pipe_buff)
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
   /*
    if(!filep){
            return -EINVAL;
        }
    int i=0;
    char * w_ptr = filep->pipe->pipe_buff;
    int s_wr = filep->pipe->write_pos;
    while(i<count){
        if(s_wr==4096){
            return -1; // memory limit exceeded
        }
        w_ptr[s_wr]=buff[i];   // ensure that s_wr < 4096
        s_wr++;
        i++;
        filep->pipe->buffer_offset++;
    }
    filep->pipe->write_pos=s_wr; // start writing next time from this place
    int ret_fd = -EINVAL; 
    return s_wr;
    */
   // cyclic implementation

    if(!filep){
        return -EINVAL;
    }
    int i=0;
    while(i<count){
        if(buff[i]=='\0'){ // buff exhausted
            return i;
        }
        if(filep->pipe->buffer_offset == 4096){
            return -EINVAL; // memory full
        }else{
        //    if(filep->pipe->write_pos >= filep->pipe->read_pos){ // normal seqeunce
        //         if(filep->pipe->write_pos == 4096){ // exceed => cyclicity introduced
        //             filep->pipe->write_pos = 0;
        //         }
        //    }
            if(filep->pipe->write_pos == 4096){ // exceed => cyclicity introduced
                filep->pipe->write_pos = 0;
            }
            //common updation in both cases
            filep->pipe->pipe_buff[filep->pipe->write_pos] = buff[i];
            i++;
            filep->pipe->buffer_offset++;
            filep->pipe->write_pos++;
        }
    }
   return filep->pipe->write_pos; // for checking
}
// one pipe => one file structure and one pipe_ifo structure
int create_pipe(struct exec_context *current, int *fd)
{
    /**
    *  TODO:: Implementation of Pipe Create
    *  Create file struct by invoking the alloc_file() function, 
    *  Create pipe_info struct by invoking the alloc_pipe_info() function
    *  fill the valid file descriptor in *fd param
    *  Incase of Error return valid Error code 
    */

    // s1 : find free fd
    int i=0, fd_read, fd_write; // looking from index 0
    while(current->files[i]){
      i++;
    }
    fd_read=i;
    i++;
    while(current->files[i]){
        i++;
    }
    fd_write=i;
    // s2 : allocate file object to read and write end of pipe

    // assuming no error
    // s3: allocating a file object 
    struct file * filep = alloc_file();
    // s4: filling the fields
    filep->inode = NULL;
    // pipe structure
    struct pipe_info *pipe1 = alloc_pipe_info();
    pipe1->is_ropen=1;
    pipe1->is_wopen=1;
    pipe1->read_pos=0;
    pipe1->write_pos=0;
    pipe1->buffer_offset=0;
    filep->pipe = pipe1;
    // filep->mode = flags; // assigning mode of opening :: ERROR mode = flags - OCREAT
    filep->type = PIPE;
    filep->ref_count=2; // needed in the implementation when deleting fds
    filep->offp = 0;
    filep->fops->read = pipe_read;
    filep->fops->write = pipe_write;
    filep->fops->close = generic_close;
    current->files[fd_read] = filep;
    current->files[fd_write] = filep;
    fd[0]=fd_read;
    fd[1]=fd_write;
    int ret_fd = 0;  // error unknown
    return ret_fd;
}

