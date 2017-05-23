/* 
 * new_char.c: holds a buffer of 100 characters as device file.
 *             prints out contents of buffer on read.
 *             writes over buffer values on write.
 */
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <stdbool.h>
#include "HBridge.h"

/********************* INITIALIZATION FUNCTIONS *********************/

int gpios[5] = {68,67,44,26,46};

// Creates GPIOs and sets direction
bool setupGPIOs(void) {
   char label[8] = "BBGPIOS";
   int i = 0, ret;
   for (; i < 5; i++) {
     ret = gpio_request(gpios[i], label);
     if (ret != 0) {
       printk(KERN_ALERT "setupGPIOs: Failed to allocate for GPIO %d\n", gpios[i]);
       return false;
     }
     ret = gpio_direction_output(gpios[i], 0);
     if (ret != 0) {
       printk(KERN_ALERT "setupGPIOs: Failed to set direction for GPIO %d\n", gpios[i]);
       return false;
     }
   }
   return true;
}

void init(void) {
  if (setupGPIOs())
    printk(KERN_INFO "...GPIOs are set\n");
  else
    printk(KERN_INFO "...GPIOs have failed to set\n");
}

/********************* MOTOR CONTROL FUNCTIONS *********************/

void ChannelA(int in1, int in2, int stdby) {
  gpio_set_value(68, in1); // AIN1
  gpio_set_value(67, in2); // AIN2
  gpio_set_value(44, stdby); // STBY
}

void ChannelB(int in1, int in2, int stdby) {
  gpio_set_value(26, in1); // BIN1
  gpio_set_value(46, in2); // BIN2
  gpio_set_value(44, stdby); // STBY
}

void SetChannel(char *indata) {
	char ch = indata[0];
	int in1 = indata[1] - '0', in2 = indata[2] - '0', stdby = indata[3] - '0';
	if (ch == 'A') ChannelA(in1, in2, stdby);
	else if (ch == 'B') ChannelB(in1, in2, stdby);
	else printk(KERN_ALERT "HBridge: Wrong Input in SetChannel");
}

/********************* FILE OPERATION FUNCTIONS *********************/

// runs on startup
// intializes module space and declares major number.
// assigns device structure data.
static int __init driver_entry(void) {
	// REGISTERIONG OUR DEVICE WITH THE SYSTEM
	// (1) ALLOCATE DINAMICALLY TO ASSIGN OUR DEVICE
	int ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ALERT "HBridge: Failed to allocate a major number\n");
		return ret;
	}
	printk(KERN_INFO "HBridge: major number is %d\n", MAJOR(dev_num));
	printk(KERN_INFO "Use mknod /dev/%s c %d 0 for device file\n", DEVICE_NAME, MAJOR(dev_num));

	// (2) CREATE CDEV STRUCTURE, INITIALIZING CDEV
	mcdev = cdev_alloc();
	mcdev->ops = &fops;
	mcdev->owner = THIS_MODULE;

	// After creating cdev, add it to kernel
	ret = cdev_add(mcdev, dev_num, 1);
	if (ret < 0) {
		printk(KERN_ALERT "HBridge: unable to add cdev to kernel\n");
		return ret;
	}
	// Initialize SEMAPHORE
	sema_init(&virtual_device.sem, 1);
    init();
	return 0;
}

// called up exit.
// unregisters the device and all associated gpios with it.
static void __exit driver_exit(void) {
	cdev_del(mcdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "HBridge: successfully unloaded\n");
}

// Called on device file open
//	inode reference to file on disk, struct file represents an abstract
// checks to see if file is already open (semaphore is in use)
// prints error message if device is busy.
int device_open(struct inode *inode, struct file* filp) {
	if (down_interruptible(&virtual_device.sem) != 0) {
		printk(KERN_ALERT "HBridge: could not lock device during open\n");
		return -1;
	}
	printk(KERN_INFO "HBridge: device opened\n");
	return 0;
}

// Called upon close
// closes device and returns access to semaphore.
int device_close(struct inode* inode, struct  file *filp) {
	up(&virtual_device.sem);
	printk(KERN_INFO "HBridge, closing device\n");
	return 0;	
}

// Called when user wants to get info from device file
ssize_t device_read(struct file* filp, char* bufStoreData, size_t bufCount, loff_t* curOffset) {
	printk(KERN_INFO "HBridge: Reading from device...\n");
	return copy_to_user(bufStoreData, virtual_device.data, bufCount);
}

// Called when user wants to send info to device
ssize_t device_write(struct file* filp, const char* bufSource, size_t bufCount, loff_t* curOffset) {
	ssize_t ret;
	printk(KERN_INFO "HBridge: writing to device...\n");
	ret = copy_from_user(virtual_device.data, bufSource, bufCount);
	SetChannel(virtual_device.data);
	return ret;
}


MODULE_LICENSE("GPL"); // module license: required to use some functionalities.
module_init(driver_entry); // declares which function runs on init.
module_exit(driver_exit);  // declares which function runs on exit.
