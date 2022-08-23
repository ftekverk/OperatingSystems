#include "jumbo_file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C does not have a bool type, so I created one that you can use
typedef char bool_t;
#define TRUE 1
#define FALSE 0


static block_num_t current_dir;


// optional helper function you can implement to tell you if a block is a dir node or an inode
static bool_t is_dir(block_num_t block_num) {
  //
  struct block test;
  if(read_block(block_num, (void *) &test) == -1){
      printf("Reading block failed \n");
      return FALSE;
  }
  else{
    //check if the block_number is a directory
    if(test.is_dir == 0) return TRUE;
    else                 return FALSE;

  }
  // return TRUE;
}


//helper function to check if a directory is empty. First check if it exists
static bool_t is_empty_dir(block_num_t block_num) {
  struct block current_buffer;
  read_block(block_num, (void *) &current_buffer);

  if(current_buffer.contents.dirnode.num_entries > 0) return FALSE;
  return TRUE;
}


/* jfs_mount
 *   prepares the DISK file on the _real_ file system to have file system
 *   blocks read and written to it.  The application _must_ call this function
 *   exactly once before calling any other jfs_* functions.  If your code
 *   requires any additional one-time initialization before any other jfs_*
 *   functions are called, you can add it here.
 * filename - the name of the DISK file on the _real_ file system
 * returns 0 on success or -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_mount(const char* filename) {
  int ret = bfs_mount(filename);
  current_dir = 1;
  return ret;
}


/* jfs_mkdir
 *   creates a new subdirectory in the current directory
 * directory_name - name of the new subdirectory
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_mkdir(const char* directory_name) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);
  //Checks
  //1.) do we already have maximum directory entries
  if(current_buffer.contents.dirnode.num_entries >= MAX_DIR_ENTRIES) return E_MAX_DIR_ENTRIES;
  //2.) Is the name too long
  if(strlen(directory_name) > MAX_NAME_LENGTH) return E_MAX_NAME_LENGTH;
  //3.) does new directory name already exist in current directory
  for(int i = 0; i<current_buffer.contents.dirnode.num_entries; i++){
      if(strcmp(directory_name , current_buffer.contents.dirnode.entries[i].name) == 0) return E_EXISTS;
  }




  //Create a new directory if one doesnt exist
  struct block block_buffer;
  block_num_t directory_block_num = allocate_block();
  if(directory_block_num == 0) return E_DISK_FULL;

  int read_status = read_block(directory_block_num, (void *) &block_buffer);

  //update the fields of the buffer
  block_buffer.is_dir = 0;
  block_buffer.contents.dirnode.num_entries = 0;  //access member of the union


  //update new block using buffer data
  write_block(directory_block_num, (void *) & block_buffer);


  //update current directory variables to buffer
  //1.) entry count
  int current_entry_count = current_buffer.contents.dirnode.num_entries;
  // printf("%i\n", current_entry_count);
  current_buffer.contents.dirnode.num_entries++;
  //2.) entries list
  strcpy(current_buffer.contents.dirnode.entries[current_entry_count].name , directory_name);
  current_buffer.contents.dirnode.entries[current_entry_count].block_num = directory_block_num;
  //write data with buffer
  write_block(current_dir, &current_buffer);


  return E_SUCCESS;
}


/* jfs_chdir
 *   changes the current directory to the specified subdirectory, or changes
 *   the current directory to the root directory if the directory_name is NULL
 * directory_name - name of the subdirectory to make the current
 *   directory; if directory_name is NULL then the current directory
 *   should be made the root directory instead
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR
 */
int jfs_chdir(const char* directory_name) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  //no arguments
  if(directory_name == NULL){
    current_dir = 1;
    return E_SUCCESS;
  }
  //arguments given
  else{
    int directory_block_num;
    //search through data and see if a name matches
    for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
      //if we find the directory
      if(strcmp(directory_name, current_buffer.contents.dirnode.entries[i].name) == 0){
        directory_block_num = current_buffer.contents.dirnode.entries[i].block_num;
        //check to see if the block is a directory
        if(!is_dir(directory_block_num)) return E_NOT_DIR;

        //if it is a directory, update!
        current_dir = directory_block_num;
        return E_SUCCESS;
      }
    }
    //if we search all entries and didnt find, it doesnt exist
    return E_NOT_EXISTS;


  }
}


/* jfs_ls
 *   finds the names of all the files and directories in the current directory
 *   and writes the directory names to the directories argument and the file
 *   names to the files argument
 * directories - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * file - array of strings; the function will set the strings in the
 *   array, followed by a NULL pointer after the last valid string; the strings
 *   should be malloced and the caller will free them
 * returns 0 on success or one of the following error codes on failure:
 *   (this function should always succeed)
 */
int jfs_ls(char* directories[MAX_DIR_ENTRIES+1], char* files[MAX_DIR_ENTRIES+1]) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  //keep track of how many of each type we have
  int directory_total = 0;
  int file_total = 0;

  //Populate our arrays, by iterating over all entries
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
    // printf("%i \n", current_buffer.contents.dirnode.num_entries);

    //directory logic
    if(is_dir(current_buffer.contents.dirnode.entries[i].block_num)){
      //allows directory name to be only 200 chars
      directories[directory_total] = malloc(200 * sizeof(char)); 
      strcpy(directories[directory_total] , current_buffer.contents.dirnode.entries[i].name);
      // printf(directories[directory_total] );
      directory_total++;
    }
    //file logic, basically the same as directory logic
    else{
      files[file_total] = malloc(200 * sizeof(char)); 
      strcpy(files[file_total] , current_buffer.contents.dirnode.entries[i].name);
      file_total++;
    }
  }
  // set the spot after last valid to null
  directories[directory_total] = NULL;
  files[file_total] = NULL;


  return E_SUCCESS;
}


/* jfs_rmdir
 *   removes the specified subdirectory of the current directory
 * directory_name - name of the subdirectory to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_NOT_DIR, E_NOT_EMPTY
 */
int jfs_rmdir(const char* directory_name) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  int directory_block_num;
  int directory_index;
  //search through data and see if a name matches
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
    //if we find the directory
    if(strcmp(directory_name, current_buffer.contents.dirnode.entries[i].name) == 0){
      directory_block_num = current_buffer.contents.dirnode.entries[i].block_num;
      //check to see if the block is a directory
      if(!is_dir(directory_block_num)) return E_NOT_DIR;
      if(!is_empty_dir(directory_block_num)) return E_NOT_EMPTY;
      //REMOVE DIRECTORY if it is empty
      directory_index = i;
      int release_status = release_block(directory_block_num);

      //update current directory variables to buffer
      //1.) entry count
      int current_entry_count = current_buffer.contents.dirnode.num_entries;
      // printf("%i\n", current_entry_count);
      current_buffer.contents.dirnode.num_entries--;
      //2.) shift all entries down
      for(int j = directory_index; j < current_entry_count; j++){
        strcpy(current_buffer.contents.dirnode.entries[j].name , current_buffer.contents.dirnode.entries[j+1].name);
        current_buffer.contents.dirnode.entries[j].block_num = current_buffer.contents.dirnode.entries[j+1].block_num;
      }
      //write data with buffer
      write_block(current_dir, &current_buffer);
      return E_SUCCESS;

    }
  }
  //if we dont find the directory in the current directory
  return E_NOT_EXISTS;

  // return E_UNKNOWN;
}


/* jfs_creat
 *   creates a new, empty file with the specified name
 * file_name - name to give the new file
 * returns 0 on success or one of the following error codes on failure:
 *   E_EXISTS, E_MAX_NAME_LENGTH, E_MAX_DIR_ENTRIES, E_DISK_FULL
 */
int jfs_creat(const char* file_name) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);
  //Checks
  //1.) do we already have maximum directory entries
  if(current_buffer.contents.dirnode.num_entries >= MAX_DIR_ENTRIES) return E_MAX_DIR_ENTRIES;
  //2.) Is the name too long
  if(strlen(file_name) > MAX_NAME_LENGTH) return E_MAX_NAME_LENGTH;
  //3.) does new directory name already exist in current directory
  for(int i = 0; i<current_buffer.contents.dirnode.num_entries; i++){
      if(strcmp(file_name , current_buffer.contents.dirnode.entries[i].name) == 0) return E_EXISTS;
  }


  //Create a new file if one doesnt exist
  struct block block_buffer;
  block_num_t directory_block_num = allocate_block();
  if(directory_block_num == 0) return E_DISK_FULL;
  read_block(directory_block_num, (void *) &block_buffer);

  //update the fields of the buffer
  block_buffer.is_dir = 1;
  block_buffer.contents.inode.file_size = 0;
  for(int i = 0; i< MAX_DATA_BLOCKS; i++){
    block_buffer.contents.inode.data_blocks[i] = NULL;
  }


  //update new block using buffer data
  write_block(directory_block_num, (void *) & block_buffer);


  //update current directory variables to buffer
  //1.) entry count
  int current_entry_count = current_buffer.contents.dirnode.num_entries;
  // printf("%i\n", current_entry_count);
  current_buffer.contents.dirnode.num_entries++;
  //2.) entries list
  strcpy(current_buffer.contents.dirnode.entries[current_entry_count].name , file_name);
  current_buffer.contents.dirnode.entries[current_entry_count].block_num = directory_block_num;
  //write data with buffer
  write_block(current_dir, &current_buffer);


  return E_SUCCESS;
}


/* jfs_remove
 *   deletes the specified file and all its data (note that this cannot delete
 *   directories; use rmdir instead to remove directories)
 * file_name - name of the file to remove
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_remove(const char* file_name) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  int file_block_num;
  int file_index;
  //search through data and see if a name matches
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
    //if we find the directory
    if(strcmp(file_name, current_buffer.contents.dirnode.entries[i].name) == 0){
      file_block_num = current_buffer.contents.dirnode.entries[i].block_num;
      //check to see if the block is a directory
      if(is_dir(file_block_num)) return E_IS_DIR;

      file_index = i;
      int release_status = release_block(file_block_num);

      //update current directory variables to buffer
      //1.) entry count
      int current_entry_count = current_buffer.contents.dirnode.num_entries;
      // printf("%i\n", current_entry_count);
      current_buffer.contents.dirnode.num_entries--;
      //2.) shift all entries down
      for(int j = file_index; j < current_entry_count; j++){
        strcpy(current_buffer.contents.dirnode.entries[j].name , current_buffer.contents.dirnode.entries[j+1].name);
        current_buffer.contents.dirnode.entries[j].block_num = current_buffer.contents.dirnode.entries[j+1].block_num;
      }
      //write data with buffer
      write_block(current_dir, &current_buffer);
      return E_SUCCESS;

    }
  }
  //if we dont find the directory in the current directory
  return E_NOT_EXISTS;
}


/* jfs_stat
 *   returns the file or directory stats (see struct stat for details)
 * name - name of the file or directory to inspect
 * buf  - pointer to a struct stat (already allocated by the caller) where the
 *   stats will be written
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS
 */
int jfs_stat(const char* name, struct stats* buf) {

  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  struct block stats_block;
  //check if directory exists
  //search through data and see if a name matches
  int block_num;
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
    //if we find the directory or file
    if(strcmp(name, current_buffer.contents.dirnode.entries[i].name) == 0){

      block_num = current_buffer.contents.dirnode.entries[i].block_num;
      

      //update stats fields
      strcpy(buf->name, current_buffer.contents.dirnode.entries[i].name);
      buf->block_num = block_num;

      //if its a directory
      if(is_dir(block_num)){
        buf->is_dir = 0;
      }
      else{
        buf->is_dir = 1;
        
        read_block(block_num, (void *) &stats_block);
        buf->file_size = stats_block.contents.inode.file_size;

        // Ceiling function
        if(stats_block.contents.inode.file_size % BLOCK_SIZE == 0){
            buf->num_data_blocks = stats_block.contents.inode.file_size / BLOCK_SIZE;
        }
        else{
            buf->num_data_blocks = (stats_block.contents.inode.file_size / BLOCK_SIZE) + 1;
        }

        
      }

      return E_SUCCESS;
    }

  }
  //if we dont find the file/directory
  return E_NOT_EXISTS;
}


/* jfs_write
 *   appends the data in the buffer to the end of the specified file
 * file_name - name of the file to append data to
 * buf - buffer containing the data to be written (note that the data could be
 *   binary, not text, and even if it is text should not be assumed to be null
 *   terminated)
 * count - number of bytes in buf (write exactly this many)
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR, E_MAX_FILE_SIZE, E_DISK_FULL
 */
int jfs_write(const char* file_name, const void* buf, unsigned short count) {
  //get current directory info
  struct block current_buffer;
  read_block(current_dir, (void *) &current_buffer);

  //check if directory exists
  //search through data and see if a name matches
  int block_num;
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
     //if we find the directory or file
    if(strcmp(file_name, current_buffer.contents.dirnode.entries[i].name) == 0){
      block_num = current_buffer.contents.dirnode.entries[i].block_num;
      if(is_dir(block_num)) return E_IS_DIR;

      //if no errors:
      //read the file block number into a buffer
      struct block file_buffer;
      read_block(block_num, (void *) &file_buffer);

      //check if file size is exceeded
      if(count + file_buffer.contents.inode.file_size > MAX_FILE_SIZE) return E_MAX_FILE_SIZE;

      int last_pos = file_buffer.contents.inode.file_size % BLOCK_SIZE;

      //find last index of block
      int total_blocks_used = (file_buffer.contents.inode.file_size / BLOCK_SIZE);
      // printf("Index of last block: %i \n", total_blocks_used);

      // printf("Index: %i \n" , total_blocks_used);


      //allocate completely new block
      int num_new_blocks;
      int num_bytes_to_write;
      int bytes_remaining = count;

      //Finish a half finished block if we have to:
      if(last_pos != 0){
        // printf("here! \n");
        //find last block in use
        block_num_t starting_block = file_buffer.contents.inode.data_blocks[total_blocks_used];

        //how many bytes do we have left to write in the block
        if(count <= BLOCK_SIZE - last_pos)  num_bytes_to_write = count;
        else                                num_bytes_to_write = BLOCK_SIZE - last_pos;

        //declare a buffer that we can read data into
        void *overwriting_block = malloc(BLOCK_SIZE * sizeof(char));
        read_block(file_buffer.contents.inode.data_blocks[total_blocks_used], overwriting_block);
        memcpy(&overwriting_block[last_pos] , buf, num_bytes_to_write);


        //rewrite our data to the block
        write_block(starting_block, overwriting_block);
        free(overwriting_block);

        //keep track of how many blocks, and how much data is left to write
        total_blocks_used++;
        if(count - num_bytes_to_write > 0) bytes_remaining = count - num_bytes_to_write;
        else bytes_remaining = 0;
      }
      
      //After populating the unfinshed block (if we did), continue if necessary
      if(bytes_remaining > 0){
        //calculate number of blocks we will need, using a ceiling function
        if(bytes_remaining % BLOCK_SIZE != 0) num_new_blocks = (bytes_remaining / BLOCK_SIZE) + 1;
        else                                  num_new_blocks = (bytes_remaining / BLOCK_SIZE);

        //  printf("num_new_blocks: %i \n" , num_new_blocks);

        //allocate the required number of blocks, and store the numbers in the inode
        for(int i = 0; i < num_new_blocks; i++){
          block_num_t allocated_block = allocate_block();
          if(allocated_block == 0) return E_DISK_FULL;

          //determine how many bytes to write
          if(bytes_remaining > BLOCK_SIZE) num_bytes_to_write = BLOCK_SIZE;
          else                             num_bytes_to_write = bytes_remaining;

          //declare a buffer that we can read data into (write to the next block)
          void *writing_block = malloc(BLOCK_SIZE * sizeof(char));
          memcpy(writing_block , &buf[i * BLOCK_SIZE], num_bytes_to_write);

          write_block(allocated_block, writing_block);
          free(writing_block);

          //update byres remaining
          bytes_remaining -= num_bytes_to_write;

          //update file information
          file_buffer.contents.inode.data_blocks[total_blocks_used] = allocated_block;
          total_blocks_used++;
        }

      }
      //update filesize
      file_buffer.contents.inode.file_size += count;
      write_block(block_num, (void *) &file_buffer);


      //after all writing is complete, return success!
      return E_SUCCESS;
    }

  }

  //if we don't find the file
  return E_NOT_EXISTS;
}


/* jfs_read
 *   reads the specified file and copies its contents into the buffer, up to a
 *   maximum of *ptr_count bytes copied (but obviously no more than the file
 *   size, either)
 * file_name - name of the file to read
 * buf - buffer where the file data should be written
 * ptr_count - pointer to a count variable (allocated by the caller) that
 *   contains the size of buf when it's passed in, and will be modified to
 *   contain the number of bytes actually written to buf (e.g., if the file is
 *   smaller than the buffer) if this function is successful
 * returns 0 on success or one of the following error codes on failure:
 *   E_NOT_EXISTS, E_IS_DIR
 */
int jfs_read(const char* file_name, void* buf, unsigned short* ptr_count) {

  //get current directory info
  struct block current_buffer;
  int block_num;
  read_block(current_dir, (void *) &current_buffer);

  //find the file
  for(int i = 0; i < current_buffer.contents.dirnode.num_entries; i++){
    if(strcmp(file_name, current_buffer.contents.dirnode.entries[i].name) == 0){
      block_num = current_buffer.contents.dirnode.entries[i].block_num;
      if(is_dir(block_num)) return E_IS_DIR;

      //read the file into a buffer
      struct block file_buffer;
      read_block(block_num, &file_buffer);

      //find the last used index in the block
      int total_blocks_used;
      int last_pos = file_buffer.contents.inode.file_size % BLOCK_SIZE;

      //find how many blocks are used by making a ceiling function
      total_blocks_used = (file_buffer.contents.inode.file_size / BLOCK_SIZE);    
      if(last_pos != 0) total_blocks_used++;         


      //for each block in use, read the data into a buffer and concatenate all the blocks
      for(int i=0; i < total_blocks_used; i++){
        block_num_t curr_block = file_buffer.contents.inode.data_blocks[i];

        //declare a buffer that we can read data into
        void *reading_block = malloc(BLOCK_SIZE * sizeof(char));
        read_block(curr_block, reading_block);

        //Two cases
        //1.) Read until last position for a partial block
        if(i == total_blocks_used - 1){
          //start reading at beginning of ith block
          memcpy(&buf[i*BLOCK_SIZE] , reading_block, last_pos);
        }
        //2.) read entire block
        else{
          memcpy(&buf[i*BLOCK_SIZE] , reading_block, BLOCK_SIZE);
        }
        free(reading_block);
      }
      //ptr will read entire file
      *ptr_count = file_buffer.contents.inode.file_size;

      return E_SUCCESS;
    }
  }
  return E_NOT_EXISTS;;
}


/* jfs_unmount
 *   makes the file system no longer accessible (unless it is mounted again).
 *   This should be called exactly once after all other jfs_* operations are
 *   complete; it is invalid to call any other jfs_* function (except
 *   jfs_mount) after this function complete.  Basically, this closes the DISK
 *   file on the _real_ file system.  If your code requires any clean up after
 *   all other jfs_* functions are done, you may add it here.
 * returns 0 on success or -1 on error; errors should only occur due to
 *   errors in the underlying disk syscalls.
 */
int jfs_unmount() {
  int ret = bfs_unmount();
  return ret;
}
