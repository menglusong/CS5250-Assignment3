#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#define MAJOR_NUMBER 63
#define DEVICE_SIZE 4000000
#define MSG_SIZE 400
#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOC_MAXNR 14
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_SETMSG _IOW(SCULL_IOC_MAGIC, 2, int)
#define SCULL_GETMSG _IOR(SCULL_IOC_MAGIC, 3, int)
 
/* forward declaration */
int scull_open(struct inode *inode, struct file *filep);
int scull_release(struct inode *inode, struct file *filep);
ssize_t scull_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
loff_t scull_lseek(struct file *filep, loff_t off, int whence);
long ioctl_example (struct file *filp, unsigned int cmd, unsigned long arg);
static void scull_exit(void);
int read_count=0;
int write_count=0;

/* definition of file_operation structure */
struct file_operations scull_fops = {
     read:     scull_read,
     write:    scull_write,
     open:     scull_open,
     release: scull_release,
     llseek:  scull_lseek,
     unlocked_ioctl : ioctl_example
};
char *four_MB_data = NULL;
//char *dev_msg = NULL;
char dev_msg[200];
int dev_msg_size = 0;

int scull_open(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}

int scull_release(struct inode *inode, struct file *filep)
{
     return 0; // always successful
}

ssize_t scull_read(struct file *filep, char *buf, size_t count, loff_t *f_pos)
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
    //printk(KERN_INFO "bytes_to_read %d \n", bytes_to_read);
    //printk(KERN_INFO "f_pos before + bytes_read %d \n", *f_pos);
    *f_pos += bytes_read;
    read_count = 0;
    //printk(KERN_INFO "f_pos %d \n", *f_pos);
    return bytes_read;
}

ssize_t scull_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos)
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
          write_count += bytes_to_write;
          printk(KERN_INFO "%d Byte(s) has been written to device\n", write_count);
          *f_pos += bytes_writen;
          printk(KERN_INFO "Done writing to device\n");
          return bytes_writen;
      } else {
          write_count = 0;
      }

      if (count > DEVICE_SIZE) {
          return -ENOSPC;
      }
}

loff_t scull_lseek(struct file *filep, loff_t off, int whence) {
      loff_t new_pos = 0;
      printk(KERN_INFO "scull device : lseek function in work\n");
      switch(whence) {
          case 0 : /*seek set*/
              new_pos = off;
              break;
          case 1 : /*seek cur*/
              new_pos = filep->f_pos + off;
              break;
          case 2 : /*seek end*/
              new_pos = DEVICE_SIZE - off;
              break;
      }
      if(new_pos > DEVICE_SIZE)
          new_pos = DEVICE_SIZE;
      if(new_pos < 0)
          new_pos = 0;
      filep->f_pos = new_pos;
      printk(KERN_INFO "new_pos is %d\n", new_pos);
      return new_pos;
}

long ioctl_example (struct file *filp, unsigned int cmd, unsigned long arg) {
      int err = 0, tmp;
      int retval = 0;
      /*
      * extract the type and number bitfields, and don't decode
      * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok() */
      if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
      if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;
      /*
      * the direction is a bitmask, and VERIFY_WRITE catches R/W * transfers. `Type' is user‐oriented, while
      * access_ok is kernel‐oriented, so the concept of "read" and * "write" is reversed
      */
      if (_IOC_DIR(cmd) & _IOC_READ)
          err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
      else if (_IOC_DIR(cmd) & _IOC_WRITE)
          err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
      if (err) return -EFAULT;
      switch(cmd) {
          case SCULL_HELLO:
              printk(KERN_WARNING "hello\n");
              break;
          case SCULL_SETMSG:
              printk(KERN_INFO "SCULL_SETMSG is called\n");
              printk(KERN_INFO "SETMSG arg is: %s", arg);
              dev_msg_size = strlen((char __user *) arg);
              retval = copy_from_user(dev_msg, (char __user *) arg, dev_msg_size);
              printk(KERN_INFO "dev_msg is: %s\n", dev_msg);
              break;
          case SCULL_GETMSG:
              printk(KERN_INFO "SCULL_GETMSG is called\n");
              printk(KERN_INFO "dev_msg_size %d\n", dev_msg_size);
              printk(KERN_INFO "dev_msg is: %s\n", dev_msg);
              printk(KERN_INFO "Before calling copy_to_user GETMSG arg is: %s", arg);
              retval = copy_to_user((char __user *) arg, dev_msg, dev_msg_size);
              printk(KERN_INFO "After calling copy_to_user GETMSG arg is: %s", arg);
              break;
          default: /* redundant, as cmd was checked against MAXNR */
              return -ENOTTY;
     }
     return retval;
     }

static int scull_init(void)
{
     int result;
     // register the device
     result = register_chrdev(MAJOR_NUMBER, "scull", &scull_fops);
     if (result < 0) {
          return result;
     }
     // allocate one byte of memory for storage
     // kmalloc is just like malloc, the second parameter is
     // the type of memory to be allocated.
     // To release the memory allocated by kmalloc, use kfree.
     four_MB_data = kmalloc(DEVICE_SIZE, GFP_KERNEL);
     if (!four_MB_data) {
          scull_exit();
          // cannot allocate memory
          // return no memory error, negative signify a failure
          return -ENOMEM;
     }
     // initialize the value to be X
     *four_MB_data = 'X';
     printk(KERN_ALERT "This is a scull device module\n");
     return 0;
}

static void scull_exit(void)
{
     // if the pointer is pointing to something
     if (four_MB_data) {
          // free the memory and assign the pointer to NULL
          kfree(four_MB_data);
          four_MB_data = NULL;
     }
     // unregister the device
     unregister_chrdev(MAJOR_NUMBER, "scull");
     printk(KERN_ALERT "scull device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(scull_init);
module_exit(scull_exit);
