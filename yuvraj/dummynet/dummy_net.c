#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

#define IOC_SEND _IOW('H', 1, struct ioc_req)

static struct net_device *devs[2];
static struct kobject *kobj;
static int major;
static struct class *hyd_class;
static struct device *hyd_device;

// sysfs logs
static char eth0_log[4096];
static char eth1_log[4096];
static int eth0_pos, eth1_pos;

struct ioc_req
{
    int if_id;
    int len;
};

// SYSFS

static ssize_t eth0_show(struct kobject *k,
                         struct kobj_attribute *a, char *b)
{
    return sprintf(b, "%s", eth0_log);
}

static ssize_t eth1_show(struct kobject *k,
                         struct kobj_attribute *a, char *b)
{
    return sprintf(b, "%s", eth1_log);
}

static struct kobj_attribute e0 = __ATTR(eth0, 0444, eth0_show, NULL);
static struct kobj_attribute e1 = __ATTR(eth1, 0444, eth1_show, NULL);

// NET OPS

static int hyd_open(struct net_device *d)
{
    netif_start_queue(d);
    printk("open %s\n", d->name);
    return 0;
}

static netdev_tx_t hyd_xmit(struct sk_buff *skb, struct net_device *d)
{
    int id = (d == devs[0]) ? 0 : 1;

    printk("xmit %s len=%u\n", d->name, skb->len);

    skb->dev = devs[1 - id];
    skb->protocol = eth_type_trans(skb, skb->dev);
    netif_rx(skb);

    return NETDEV_TX_OK;
}

static const struct net_device_ops ops = {
    .ndo_open = hyd_open,
    .ndo_start_xmit = hyd_xmit,
};

// IOCTL

static long hyd_ioctl(struct file *f, unsigned int c, unsigned long a)
{
    struct ioc_req req;
    struct timespec64 ts;
    struct tm tm;
    char line[64];
    int n;
    struct sk_buff *skb;

    if (copy_from_user(&req, (void *)a, sizeof(req)))
        return -EFAULT;

    if (req.if_id < 0 || req.if_id > 1)
        return -EINVAL;

    if (req.len < 1 || req.len > 1024)
        return -EINVAL;

    ktime_get_real_ts64(&ts);
    time64_to_tm(ts.tv_sec, 0, &tm);

    n = sprintf(line, "%02d:%02d:%02d Len:%d\n",
                tm.tm_hour, tm.tm_min, tm.tm_sec, req.len);

    if (req.if_id == 0)
    {
        memcpy(eth0_log + eth0_pos, line, n);
        eth0_pos += n;
        eth0_log[eth0_pos] = 0;
    }
    else
    {
        memcpy(eth1_log + eth1_pos, line, n);
        eth1_pos += n;
        eth1_log[eth1_pos] = 0;
    }

    skb = alloc_skb(req.len + ETH_HLEN, GFP_KERNEL);
    skb_reserve(skb, ETH_HLEN);
    skb_put(skb, req.len);

    skb->dev = devs[req.if_id];
    skb->protocol = eth_type_trans(skb, skb->dev);

    netif_rx(skb);

    return 0;
}

static struct file_operations fops = {
    .unlocked_ioctl = hyd_ioctl,
};

// INIT / EXIT

static int __init hyd_init(void)
{
    major = register_chrdev(0, "hyd", &fops);

    hyd_class = class_create(THIS_MODULE, "hyd");
    hyd_device = device_create(hyd_class, NULL,
                               MKDEV(major, 0), NULL, "hyd");

    devs[0] = alloc_etherdev(0);
    devs[1] = alloc_etherdev(0);

    devs[0]->netdev_ops = &ops;
    devs[1]->netdev_ops = &ops;

    strcpy(devs[0]->name, "hyd0");
    strcpy(devs[1]->name, "hyd1");

    register_netdev(devs[0]);
    register_netdev(devs[1]);

    kobj = kobject_create_and_add("hyd01", kernel_kobj);
    sysfs_create_file(kobj, &e0.attr);
    sysfs_create_file(kobj, &e1.attr);

    major = register_chrdev(0, "hyd", &fops);

    printk("hyd loaded\n");
    return 0;
}

static void hyd_exit(void)
{
    device_destroy(hyd_class, MKDEV(major, 0));
    class_destroy(hyd_class);

    unregister_chrdev(major, "hyd");
    unregister_netdev(devs[0]);
    unregister_netdev(devs[1]);
    kobject_put(kobj);
    printk("hyd removed\n");
}

module_init(hyd_init);
module_exit(hyd_exit);
MODULE_LICENSE("GPL");
