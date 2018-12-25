#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/param.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/delay.h>	
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
#include <linux/kthread.h>

#define DRIVER_NAME "spidev"
#define DEVICE_NAME "spidev1.34"
#define DEVICE_CLASS_NAME "spidev"
#define MINOR_NUMBER 0
#define MAJOR_NUMBER 154 
#define CONFIG 1

static struct spi_data *dat;
static struct class *spiclass;
static struct spi_message ms;
//static unsigned int busy=0;
unsigned int pattern[16];
uint8_t s_arr[10][24];
uint8_t tx[2];
struct spi_transfer tr = {

	.tx_buf = tx,
	.rx_buf = 0,
	.len = 2,
	.bits_per_word = 8,
	.speed_hz = 10000000,
	.delay_usecs = 1,
	.cs_change = 1,
};

struct spi_data {
	dev_t  devt;
	struct spi_device *spid;
};
/***************Thread function***************************/
int thread_func(void *data)
{
int i,j,k,l;
for(i=0;i<16;i++)
{
l=pattern[i];
k=i+1;
if(pattern[i]==0 && pattern[i+1]==0)
{
tx[0]=0x0c;
tx[1]=0x00;
spi_message_init(&ms);
spi_message_add_tail((void *)&tr, &ms);
gpio_set_value(15,0);
spi_sync(dat->spid, &ms);
gpio_set_value(15,1);
}
else
{
for(j=0;j<24;j=j+2)
{
tx[0]=s_arr[l][j];
tx[1]=s_arr[l][j+1];
spi_message_init(&ms);
spi_message_add_tail((void *)&tr, &ms);
gpio_set_value(15,0);
spi_sync(dat->spid, &ms);
gpio_set_value(15,1);
}
} 
i = i+1;
msleep(k * 300);
}

return 0;
}

int open_spi(struct inode *i,struct file *f) 
{
	gpio_export(24,true);
	gpio_export(25,true);
	gpio_export(30,true);
	gpio_export(31,true);
	gpio_export(42,true);
	gpio_export(43,true);
	gpio_export(46,true);
	gpio_export(44,true);
	gpio_export(72,true);
	gpio_export(15,true);
	gpio_export(5,true);
	gpio_export(7,true);
	gpio_direction_output(5,1);
	gpio_set_value(5,1);
	gpio_direction_output(7,1);
	gpio_set_value(7,1);
	gpio_direction_output(46,1);
	gpio_set_value(46,1);
	gpio_direction_output(30,1);
	gpio_set_value(30,0);
	gpio_direction_output(31,1);
	gpio_set_value(31,0);
	gpio_set_value(72,0);
	gpio_direction_output(24,1);
	gpio_set_value(24,0);
	gpio_direction_output(25,1);
	gpio_set_value(25,0);
	gpio_direction_output(44,1);
	gpio_set_value(44,1);
	gpio_direction_output(42,1);
	gpio_set_value(42,0);
	gpio_direction_output(43,1);
	gpio_set_value(43,0);
	gpio_direction_output(15,1);
	gpio_set_value(15,0);
	return 0;
}


int release_spi(struct inode *i, struct file *flpptr){
	gpio_unexport(24);
	gpio_free(24);
	gpio_unexport(25);
	gpio_free(25);
	gpio_unexport(30);
	gpio_free(30);
	gpio_unexport(31);
	gpio_free(31);
	gpio_unexport(42);
	gpio_free(42);
	gpio_unexport(43);
	gpio_free(43);
	gpio_unexport(46);
	gpio_free(46);
	gpio_unexport(44);
	gpio_free(44);
	gpio_unexport(72);
	gpio_free(72);
	gpio_unexport(15);
	gpio_free(15);
	gpio_unexport(5);
	gpio_free(5);
	gpio_unexport(7);
	gpio_free(7);
	return 0;
}
/************************Write function**************************************/

ssize_t write_spi(struct file *f,const char *spim, size_t sz, loff_t *loffptr)
{
int ret;
uint8_t array[16];
struct task_struct *taskspi;
copy_from_user((void *)&array, (void * __user)spim, sizeof(array));
for(ret=0;ret<16;ret++)
{
	pattern[ret] = array[ret];
	printk(" %d ",pattern[ret]);
}
taskspi = kthread_run(&thread_func, (void *)pattern,"kthread_spi_led");
msleep(1000);
printk("Terminating");
return 0;	
}

/*************************IOCTL function************************************/

static long spi_ioctl(struct file *f,unsigned int cmd, unsigned long spicnt)  
{

	int i=0, j=0,ret;
	uint8_t r_arr[10][24];

	
	ret = copy_from_user((void *)&r_arr,(void *)spicnt, sizeof(r_arr));
	if(ret != 0)
	{
		printk("Failure : %d number of bytes that could not be copied.\n",ret);
	}
	for(i=0;i<10;i++)
	{
	for(j=0;j<24;j++)
	{
			s_arr[i][j] = r_arr[i][j];
			printk(" %d ",s_arr[i][j]);
	}
	}
	printk("ioctl End\n");
	return 0;
}


static struct file_operations spi_ops = {
	.open 	 = open_spi,
	.release = release_spi,
	.write	 = write_spi,
	.unlocked_ioctl = spi_ioctl,
};

/**********************************Probe function******************************************/
static int probe_spi(struct spi_device *spid){
	
	int status = 0;
	struct device *dev;

	/* Allocate driver data */
	dat = kzalloc(sizeof(*dat), GFP_KERNEL);
	if(!dat)
	{
		return -ENOMEM;
	}
	dat->spid = spid;

	dat->devt = MKDEV(MAJOR_NUMBER, MINOR_NUMBER);

    	dev = device_create(spiclass, &spid->dev, dat->devt, dat, DEVICE_NAME);

    if(dev == NULL)
    {
		printk(" Failed while creating device\n");
		kfree(dat);
		return -1;
	}
	printk("Led matrix probed.\n");
	return status;
}

/***********************************spi remove*******************************/
static int spi_remove(struct spi_device *spid)
{
	device_destroy(spiclass, dat->devt);
	kfree(dat);
	printk(" Driver Removed.\n");
	return 0;
}

struct spi_device_id Leddeviceid[] = {
{"spidev",0},
{}
};


static struct spi_driver driverspimat = {
         .driver = {
         .name =         "spidev",
         .owner =        THIS_MODULE,
         },
	 .id_table =   Leddeviceid,
         .probe =        probe_spi,
         .remove =       spi_remove,
		
};
/***************************************INIT function***************************************/
static int __init spi_init(void)
{
	
	int ret;
	
	//Register the Device
	ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &spi_ops);
	if(ret < 0)
	{
		printk(" Registration of the device failed\n");
		return -1;
	}
	
	//Create the class
	spiclass = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
	if(spiclass == NULL)
	{
		printk("Class Creation Failed\n");
		unregister_chrdev(MAJOR_NUMBER, driverspimat.driver.name);
		return -1;
	}
	
	//Register the Driver
	ret = spi_register_driver(&driverspimat);
	if(ret < 0)
	{
		printk("Driver Registraion Failed\n");
		class_destroy(spiclass);
		unregister_chrdev(MAJOR_NUMBER, driverspimat.driver.name);
		return -1;
	}
	
	printk(" LED driver Initialized.\n");
	return 0;
}

/*******************************************EXIT function******************************************/
void __exit spi_exit(void)
{
	spi_unregister_driver(&driverspimat);	//Unregister the SPI device
	class_destroy(spiclass);	//Destroy the driver class
	unregister_chrdev(MAJOR_NUMBER, driverspimat.driver.name);	//Unregister the major and minor number associated
	printk("SPI LED Driver Exit.\n");
}

MODULE_LICENSE("GPL v2");
module_init(spi_init);
module_exit(spi_exit);

