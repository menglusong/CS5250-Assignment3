#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#define MAJOR_NUMBER 61
#define DEVICE_SIZE 4000000
 
/* forward declaration */
int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void onebyte_exit(void);
int read_count=0;
int write_count=0;

/* definition of file_operation structure */
struct file_operations onebyte_fops = {
     read:     onebyte_read,
     write:    onebyte_write,
     open:     onebyte_open,
     release: onebyte_release
};
char *four_MB_data = NULL;

int onebyte_open(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}

int onebyte_release(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}

ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
{
    int maxbytes; /*maximum bytes that can be read from f_pos to DEVICE_SIZE*/
    int bytes_to_read; /* gives the number of bytes to read*/
    int bytes_read; /*number of bytes actually read*/

    maxbytes = DEVICE_SIZE - *f_pos;
    if (maxbytes > count)
        bytes_to_read = count;
    else
        bytes_to_read = maxbytes;

    if (bytes_to_read == 0) {
        printk(KERN_INFO "Reached the end of the device\n");
        return 0;
    }
    bytes_read = bytes_to_read - copy_to_user(buf, four_MB_data + *f_pos, bytes_to_read);
    read_count += bytes_read;
    printk(KERN_INFO "%d Byte(s) has been read \n", read_count);
    *f_pos += bytes_read;
    return bytes_read;
}

ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
{
      int maxbytes; /*maximum bytes that can be read from ppos to DEVICE_SIZE*/
      int bytes_to_write; /* gives the number of bytes to write*/
      int bytes_writen; /*number of bytes actually writen*/
      maxbytes = DEVICE_SIZE - *f_pos;
      if (maxbytes > count)
          bytes_to_write = count;
      else
          bytes_to_write = maxbytes;

      if (count && bytes_to_write) {
          bytes_writen = bytes_to_write - copy_from_user(four_MB_data + *f_pos, buf, bytes_to_write);
          write_count += bytes_writen;
          printk(KERN_INFO "%d Byte(s) has been written to device\n", write_count);
          *f_pos += bytes_writen;
          printk(KERN_INFO "Done writing to device\n");
          return bytes_writen;
      }

      if (count > DEVICE_SIZE) {
          return -ENOSPC;
      }
}

static int onebyte_init(void)
{
     int result;
     // register the device
     result = register_chrdev(MAJOR_NUMBER, "onebyte", &onebyte_fops);
     if (result < 0) {
          return result;
     }
     // allocate one byte of memory for storage
     // kmalloc is just like malloc, the second parameter is
     // the type of memory to be allocated.
     // To release the memory allocated by kmalloc, use kfree.
     four_MB_data = kmalloc(DEVICE_SIZE, GFP_KERNEL);
     if (!four_MB_data) {
          onebyte_exit();
          // cannot allocate memory
          // return no memory error, negative signify a failure
          return -ENOMEM;
     }
     // initialize the value to be X
     *four_MB_data = 'X';
     printk(KERN_ALERT "This is a 4 MB device module\n");
     return 0;
}

static void onebyte_exit(void)
{
     // if the pointer is pointing to something
     if (four_MB_data) {
          // free the memory and assign the pointer to NULL
          kfree(four_MB_data);
          four_MB_data = NULL;
     }
     // unregister the device
     unregister_chrdev(MAJOR_NUMBER, "onebyte");
     printk(KERN_ALERT "4 MB device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);
