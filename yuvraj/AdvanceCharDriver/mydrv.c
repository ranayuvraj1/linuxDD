#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "mychardev"
#define CLASS_NAME  "mychardev_class"


#define MY_MAGIC        'M'
#define IOCTL_GET_VAL   _IOR(MY_MAGIC, 1, int)  
#define IOCTL_SET_VAL   _IOW(MY_MAGIC, 2, int)  
#define IOCTL_CLR_VAL   _IO(MY_MAGIC,  3)       

static dev_t dev_num;
static struct cdev my_cdev;
static struct class  *my_class;
static struct device *my_device;


static int dev_value = 0;



static int my_open(struct inode *inode, struct file *filp)
{
    pr_info("mychardev: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    pr_info("mychardev: release\n");
    return 0;
}


static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int val;

    if (cmd == IOCTL_GET_VAL) {
        val = dev_value;

        if (copy_to_user((int __user *)arg, &val, sizeof(val)))
            return -EFAULT;

        pr_info("mychardev: ioctl GET_VAL=%d\n", val);
        return 0;
    }

    if (cmd == IOCTL_SET_VAL) {
        if (copy_from_user(&val, (int __user *)arg, sizeof(val)))
            return -EFAULT;

        dev_value = val;
        pr_info("mychardev: ioctl SET_VAL=%d\n", dev_value);
        return 0;
    }

    if (cmd == IOCTL_CLR_VAL) {
        dev_value = 0;
        pr_info("mychardev: ioctl CLR_VAL\n");
        return 0;
    }

    return -ENOTTY; 
}

static const struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .unlocked_ioctl = my_ioctl,
};



static int __init mychardev_init(void)
{
    int ret;

    pr_info("mychardev: init\n");

   
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("mychardev: alloc_chrdev_region failed\n");
        return ret;
    }

  
    cdev_init(&my_cdev, &my_fops);
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("mychardev: cdev_add failed\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

   
    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        ret = PTR_ERR(my_class);
        pr_err("mychardev: class_create failed\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    my_device = device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        ret = PTR_ERR(my_device);
        pr_err("mychardev: device_create failed\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    pr_info("mychardev: loaded major=%d minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

static void __exit mychardev_exit(void)
{
    pr_info("mychardev: exit\n");

    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("mychardev: unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rana Yuvraj");
MODULE_DESCRIPTION("Char driver with entry points + ioctl + class/device");
