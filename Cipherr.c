#include <linux/module.h>	
#include <linux/kernel.h>	
#include <linux/init.h>	
#include <asm/uaccess.h>
#include <linux/processor.h>
#include <linux/buffer_head.h>
#include <linux/unistd.h>
#include <linux/proc_fs.h>	
#include <linux/module.h>	
#include <linux/fs.h> 		
#include <linux/slab.h>		
#include <asm/segment.h>	
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>

MODULE_LICENSE("GPL");
#define MAX_DEV 2
dev_t majornumber;

struct class *cipherdev;
static char *message; 	
static char *enckey;	
static int open=0;
static short messagesize;
unsigned char *c;
unsigned char* messagebuffer;
static struct cdev cipherdev[MAX_DEV];
void rc4(unsigned char * p, unsigned char * k, unsigned char * c,int l);

static struct file_operations cipher_fops = {
    .open       = cipher_open,
    .release    = cipher_release,
    .read       = seq_read,
    .write       = cipher_write
};

static int messageprint(struct seq_file *sf, void *x){
		seq_printf(sf, "%u\n", c);
		return 0;
}

static int cipher_open(struct inode* inode, struct file* file){
	open++;
	printk(KERN_INFO "Cipher Device was successfully opened: %d\n", open);
	message= kzalloc(4096, GFP_KERNEL); //allocate memory and set to 0
	if (!message) return -ENOMEM;
	c= kzalloc(4096, GFP_KERNEL); //allocate memory and set to 0
	if (!c) return -ENOMEM;	
	messagebuffer= kzalloc(4096, GFP_KERNEL); //allocate memory and set to 0
	if (!messagebuffer) return -ENOMEM;
	return single_open(file,messageprint, NULL); //already available in kernel
	return 0;
}

static ssize_t cipher_write (struct file * file, const char* buf, size_t count, loff_t * pos){
	char *buffer= kzalloc((count+1), GFP_KERNEL); //allocate memory and set to 0
	if (!buffer) return -ENOMEM;
	
	if (copy_from_user(buffer, buf, count)){
		kfree(mybuf);	
		return EFAULT;
	}
	kfree(message);
	buffer[count-1]='\0';
	message=buffer;
	memcpy(messagebuffer, message, 4096);
	messagesize= strlen(message);

	printk(KERN_INFO "successfully written %s\n", message);
	printk(KERN_INFO "successfully written %s\n", messagebuffer);

	return count;

}


static int cipher_release(struct inode* inode, struct file* file){

	rc4(message, enckey, c,100);
	printk(KERN_INFO "successfully released \n");
	return 0;
}

static struct file_operations cipher_enckey_fops = {
	.open= cipher_enckey_open,
	.read=seq_read,
	.write=cipher_enckey_write,
	.release=cipher_enckey_release,
};
static int keyprint(struct seq_file *sf, void *v){
		seq_printf(sf, "the encrypted can't be read!\n");
		return 0;
}
static int cipher_enckey_open(struct inode* inode, struct file* file){
	enckey= kzalloc(128, GFP_KERNEL); //allocate memory and set to 0
	if (!enckey) return -ENOMEM;
	
	printk(KERN_INFO "Cipher encrypted key was successfully opened\n");
	return single_open(file,keyprint, NULL); //already available in kernel
}

static ssize_t cipher_enckey_write (struct file * file, const char* buf, size_t count, loff_t * pos){

	char *buffer= kzalloc((count+1), GFP_KERNEL); //allocate memory and set to 0
	if (!buffer) return -ENOMEM;
	
	if (copy_from_user(buffer, buf, count)){
		kfree(buffer);	
		return EFAULT;
	}
	kfree(enckey);
	buffer[count-1]='\0';
	enckey=buffer;
	
	messagesize= strlen(enckey);

	printk(KERN_INFO "the key was written successfully: %s\n", key);
		return count;
}

static int cipher_enckey_release(struct inode* inode, struct file* file){
	printk(KERN_INFO "was successfully released \n");
	return 0;	
}

static const struct file_operations cipherkey_fops= {
	.open=cipherkey_open,
	.read=seq_read,
	.write=cipherkey_write,
	.llseek=seq_lseek,
	.release=single_release
};
static int printcipherkey(struct seq_file *sf, void *v){
	
	printk(KERN_INFO "the message is: %s\n", messagebuffer);
	if (strcmp(enckey,"ABCD")==0)
		seq_printf(sf, "the message is: %s\n", messagebuffer);
	else{ 
	seq_printf(sf, "%u\n", c);
	}
	kfree(messagebuffer);
	kfree(c);
	return 0;
}


int cipherkey_open(struct inode *inmode, struct file *file){
	printk(KERN_INFO"This is to open the cipher key \n");
	return single_open(file, printcipherkey, NULL); //already available in kernel
}
	
static ssize_t cipherkey_write(struct file* file, const char __user *buf, size_t count, loff_t *pos){
	printk("you can't write into it \n");
	return count;

}

static int __init cipherdev_init(void){

	int x;
	int y=0;

	x=alloc_chrdev_region(&majornumber, 0, MAX_DEV, "cipher");
	if (!x){
		int major = MAJOR(majornumber);
		dev_t thedevice;
		cipherdev= class_create(THIS_MODULE, "driver_class");
	
		for (y=0; y<MAX_DEV; y++){
			thedevice=MKDEV(major, y);
			if (!y) cdev_init(&mycdev[y],&cipher_fops);
			else if(y) cdev_init(&mycdev[y],&cipher_enckey_fops);
			x=cdev_add(&mycdev[y], thedevice, 1);
			if(x) printk(KERN_INFO "adding new device failed\n");
			else {
			if (!y) device_create(cipherdev, NULL, thedevice, NULL, "cipher");
			else if(y) device_create(cipherdev, NULL, thedevice, NULL, "cipher_key");
			}
		} 
	}
	else {
		printk(KERN_INFO "cannot allocate major number!\n");
		return x;
	}
	
struct proc_dir_entry *cipher_entry =proc_create("cipher",0x0644,NULL, &proc_cipher_fops);
if (cipher_entry == NULL) return -ENOMEM;	//check for error

struct proc_dir_entry *cipherkey_entry =proc_create("cipher_key",0x0644,NULL, &proc_cipherkey_fops);
if (cipherkey_entry == NULL) return -ENOMEM;	//check for error

	return 0; 		//0 for success
}


static void __exit cipherdev_exit(void){
	printk(KERN_INFO "Bye bye CSCE-3402 :)\n");

	int i=0;
	int major= MAJOR(majornumber);
	dev_t thedevice;
	for (i=0; i<MAX_DEV; i++){
		thedevice=MKDEV(major, i);
		cdev_del(&mycdev[i]);
		device_destroy (cipherdev, thedevice);
	}
	class_destroy(cipherdev);
	unregister_chrdev_region(majornumber, MAX_DEV);


	remove_proc_entry("cipher", NULL); //remove proc entry created
	remove_proc_entry("cipher_key", NULL); //remove proc entry created
}

module_init(cipherdev_init);
module_exit(cipherdev_exit);


void rc4(unsigned char * p, unsigned char * k, unsigned char * c,int l)
{
        unsigned char s [256];
        unsigned char t [256];
        unsigned char temp;
        unsigned char kk;
        int i,j,x;
        for ( i  = 0 ; i  < 256 ; i ++ )
        {
                s[i] = i;
                t[i]= k[i % 4];
        }
        j = 0 ;
        for ( i  = 0 ; i  < 256 ; i ++ )
        {
                j = (j+s[i]+t[i])%256;
                temp = s[i];
                s[i] = s[j];
                s[j] = temp;
        }

        i = j = -1;
        for ( x = 0 ; x < l ; x++ )
        {
                i = (i+1) % 256;
                j = (j+s[i]) % 256;
                temp = s[i];
                s[i] = s[j];
                s[j] = temp;
                kk = (s[i]+s[j]) % 256;
                c[x] = p[x] ^ s[kk];
        }
}

