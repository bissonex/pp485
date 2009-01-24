/* Dynamic Engineering									
   Copyright 2006										
   PCI driver for PMC-PARALLEL-485-NG1.  				
     Note: printk output shows up in /var/log/kern.log 	*/

#include <linux/module.h>		/* Needed by all modules 		*/
#include <linux/kernel.h>		/* Needed for KERN_INFO 		*/
#include <linux/init.h>			/* Needed for the macros 		*/
#include <linux/proc_fs.h>
#include <linux/fs.h>         	/* struct file, struct inode 	*/
#include <linux/pci.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>      	/* copy_to_user(), copy_from_user() */
#include <linux/interrupt.h>  	/* request_irq() */
 
#include "pmc485.h"

#define DRIVER_AUTHOR "Dynamic Engineering <sales@dyneng.com>"
#define DRIVER_DESC   "PMC-PARALLEL-485 device driver"

#define DRIVERNAME "pp485pci"  	/* name shows up in /dev and /sys/bus/pci/drivers */
#define MAX_DEVICES 4         	/* max number of PMC-PARALLEL-485 PCI boards in a single PC */
#define MAX_INTERRUPTS 33     	/* the maximum number of interrupt status bytes that this driver will queue + 1.  Old status bytes will be discarded if the buffer is full. */

#define SUCCESS 0

/* contains date private to each pp485 board installed in the PC */
struct board_private 
{
	struct miscdevice  pp485pci_miscdev;		/* contains the minor number */
	void __iomem *RegistersMap;      			/* from BAR0.  This is 512 bytes of I/O memory.*/

	unsigned int  interrupt; //irq number (e.g. 11).  This is stored only to call free_irq upon rmmod
	unsigned char IRQByte[MAX_INTERRUPTS]; //interrupt status bytes read from N+8 (e.g. f0 results from UI0)
};

static struct board_private  pp485card[MAX_DEVICES]; //array of pp485 board structure that hold the IO address
static int card = -1;        //total pp485 boards found minus 1 (matching PLX vendor and device IDs)
static char name[10]; 		//will contain name in /dev/ (e.g. pp485pci0)

//___________________________________________________________________________________________
// Purpose  :  Handle all ioctrl 
// Returns  :  EINVAL if invalid ioctl


static int pp485pci_ioctl(struct inode*  inode, struct file*  filep, unsigned int cmd, unsigned long arg)
{
	struct board_private *gp = filep->private_data;
	//unsigned long SW = gp->RegistersMap + 0x44; //control register
	unsigned int val = 0;
	unsigned int temp = 0;
	int retval = 0;

	switch(cmd)
	{
		case PP485_IOCG_SW:
			//printk("BAR = %lX\n", (unsigned long)gp->RegistersMap);
			//printk("SW = %X\n", readl( (gp->RegistersMap) + 0x44));
			val = readl((gp->RegistersMap) + 0x44);
			//return ((unsigned char) bytesRead & 0x000000FF);
			retval = __put_user((val& 0x000000FF), (unsigned char __user *)arg);
			break;

		case PP485_IOCG_DATAIN:
			val = readl((gp->RegistersMap) + 0x24);
			retval = __put_user(val, (unsigned int __user *)arg);
			break;
			
		case PP485_IOCG_STAT:
			val = readl((gp->RegistersMap) + 0x04);
			retval = __put_user(val, (unsigned int __user *)arg);
			break;			

		case PP485_IOCS_DATAOUT:	
			retval = __get_user(val, (unsigned int __user *)arg);
			if (retval == 0) {
				writel(val, (gp->RegistersMap) + 0x18);
			}
			break;	
						
		case PP485_IOCS_DIR:
			retval = __get_user(val, (unsigned int __user *)arg);
			temp = readl((gp->RegistersMap) + 0x14);
			temp = temp & 0xFFFF8000; 	/* clear dir setting */
			val = val & 0x000007FF;		/* DIR is using bits 10-0 */
			val = temp | val; 
			if (retval == 0) {
				writel(val, (gp->RegistersMap) + 0x14);
			}		
			break;

		case PP485_IOCS_BASE:
			retval = __get_user(val, (unsigned int __user *)arg);
			if (retval == 0) {
				writel(val, (gp->RegistersMap) + 0x00);
			}		
			break;
									
		default: /* redundant, as cmd was checked against MAXNR */
			retval = -ENOTTY;			
	}

	return(retval);
}

//___________________________________________________________________________________________
//called when the open() system call is invoked from user space

int pp485_open(struct inode *inode, struct file *filep) 
{
	int i = 0;

	printk("pp485_open %i %i\n", imajor(inode), iminor(inode));
   
	//search for the minor number
	for(i = 0; i <= card; i++)
		if(pp485card[i].pp485pci_miscdev.minor == iminor(inode))
			break; //found it

	filep->private_data = &pp485card[i];	//initialize the private data for this file descriptor.  This will be accessed in read() and write()
	
	try_module_get(THIS_MODULE);
	return SUCCESS;;
}
//___________________________________________________________________________________________
//called when the close() system call is invoked from user space

int pp485_release(struct inode *inode, struct file *filep) 
{
	module_put(THIS_MODULE);
	return SUCCESS;
}

/*__________________________________________________________________*/
/* File operations structure										*/
/* Each field of the structure corresponds to the address of some 	*/
/* function defined by the driver to handle a requested operation.	*/
static struct file_operations  pp485pci_fops = {
	.owner      = THIS_MODULE,
	.open       = pp485_open,
	.release    = pp485_release,
	.ioctl      = pp485pci_ioctl, 
};


static struct pci_device_id pp485_pci_tbl[] __devinitdata = {
	{ PCI_DEVICE(0x10EE, 0x0015) },  //Xilinx vendor ID, device ID.  Should also check for subvendor
	{ 0 },
};

MODULE_DEVICE_TABLE(pci, pp485_pci_tbl); //let module loading system know what module works with what hardware devices

static int
pp485_read_proc(char *buffer, char **start, off_t offset, int size, int *eof,
                void *data)
{
	char *hello_str = "Hello, world!\n";
	int len = strlen(hello_str); /* Don't include the null byte. */
	/*
	* We only support reading the whole string at once.
	*/
	if (size < len)
		return -EINVAL;
	/*
	* If file position is non-zero, then assume the string has
	* been read and indicate there is no more data to be read.
	*/
	if (offset != 0)
		return 0;
	/*
	* We know the buffer is big enough to hold the string.
	*/
	strcpy(buffer, hello_str);
	/*
	* Signal EOF.
	*/
	*eof = 1;

	return len;
}

//___________________________________________________________________________________________
//PCI "probe" function called when the kernel finds a pp485 board (matching pp485_pci_tbl).
//0 is normal return.  Called once per board.  If the function doesn't want to control the device
//or an error occurs, returns a negative value.  This function stores the minor number 
//in pp485pci_miscdev.minor so that open() can later figure out which board is being opened,
//and set the correct filep->private_data (so that read() and write() know which ioaddr to access).

static int __devinit pp485_card_init(struct pci_dev  *pdev, const struct pci_device_id  *ent)
{
	int ret = -EIO;
	unsigned long start;
	unsigned long len;
   
	card++;        //incremented each time be find a pp485 board
   
	if (card >= MAX_DEVICES) {
		printk("pp485_card_init This driver only supports %i devices\n", MAX_DEVICES);
		card--; //update the card count
		return -ENODEV;
   }
   
	if (pci_enable_device(pdev)) {  //wake up the device
		printk("pp485_card_init Not possible to enable PCI Device\n");
		card--; //update the card count
		return -ENODEV;
	}

   	//Mark all PCI regions associated with PCI device pdev as being reserved by owner res_name.
   	//Do not access any address inside the PCI regions unless this call returns successfully.
   	//This will reverve both i/o ports and memory regions, and shows up in /proc/ioports and /proc/iomem
   	if (pci_request_regions(pdev, DRIVERNAME)) {
		//printk("pp485_card_init I/O address (0x%04x) already in use\n", (unsigned int)pp485card[card].MainFIFOAddress);
		//ret = -EIO;
		goto err_out_disable_device;
	}

	//I/O ADDRESSES
	start = pci_resource_start(pdev, /*BAR*/ 0);
	len = pci_resource_len(pdev, /*BAR*/ 0);
	printk("Start = %lx, len = %lx\n", start, len);
	pp485card[card].RegistersMap =   ioremap_nocache(start, len);
	printk("IORemap to %lx\n", (unsigned long)pp485card[card].RegistersMap);

   //INTERRUPTS
   	pp485card[card].interrupt = pdev->irq; //store the IRQ number so we can call free-irq in the cleanup function when the module us removed with rmmod
	printk("Interrupt is %d\r\n",  pp485card[card].interrupt);  

/*
   if (request_irq(pp485card[card].interrupt, pci_interrupt, SA_SHIRQ, DRIVERNAME, &pp485card[card])) //register the interrupt handler.  This should happen before we enable interrupts on the controller.
   {
      printk( KERN_ERR "IRQ %x is not free\n", pp485card[card].interrupt);
      goto err_out_irq;
   }
*/
	//register the device under major number 10.  Minor number will show up in /proc/misc
	pp485card[card].pp485pci_miscdev.minor  = MISC_DYNAMIC_MINOR, //assign the minor number dynamically (ask for a free one).  This field will eventually contain the actual minor number.
	sprintf(name, DRIVERNAME "%i", card);
	pp485card[card].pp485pci_miscdev.name   = name,               //the name for this device, meant for human consumption: users will find the name in the /proc/misc file.
	pp485card[card].pp485pci_miscdev.fops   = &pp485pci_fops,     //the file operations

	ret = misc_register (&(pp485card[card].pp485pci_miscdev));
	if (ret) {
		printk ("pp485_card_init cannot register miscdev (err=%d)\n", ret);
		goto err_out_misc;
	}

	/*
	 * Create an entry in /proc named "hello_world" that calls
	 * hello_read_proc() when the file is read.
	 */
	if (create_proc_read_entry("pp485_id", 0, NULL, pp485_read_proc,
								NULL) == 0) {
		printk(KERN_ERR
				   "Unable to register \"pp485_id\" proc file\n");
		return -ENOMEM;
	}
        
	return ret;

	err_out_misc:
		//misc_deregister(&(pp485card[card].pp485pci_miscdev));
		free_irq(pp485card[card].interrupt, &pp485card[card]);
	err_out_irq:
		pci_release_regions(pdev);
	err_out_disable_device:
		pci_disable_device(pdev);

	card--; //update the card count

	return ret;
}

//___________________________________________________________________________________________
//Called once per board upon rmmod

static void __devexit pp485_card_exit(struct pci_dev *pdev)
{
	printk("pp485_card_exit\n");

	remove_proc_entry("hello_world", NULL);
   
	misc_deregister(&(pp485card[card].pp485pci_miscdev));  //unregister the device with major number 10

//   outb(~0x40 & inb(pp485card[card].MainFIFOAddress + 4), pp485card[card].MainFIFOAddress + 4); //disable interrupts on the controller.  They will stay disabled until the module is loaded again.
//   free_irq(pp485card[card].interrupt, &pp485card[card]); //should happen after interrupts are disabled on the controller

	pci_release_regions(pdev);				 //relase the I/O ports and memory regions
	pci_disable_device(pdev);
	card--;
}
//___________________________________________________________________________________________
//PCI driver structure

static struct pci_driver   pp485_driver = 
{
	.name     = DRIVERNAME,						//name of driver (must be unique among all PCI drivers).  Shows up under /sys/bus/pci/drivers
	.id_table = pp485_pci_tbl,					//list of PCI IDs this driver supports
	.probe    = pp485_card_init,				//called when a board is found
	.remove   = __devexit_p(pp485_card_exit),	//called on exit
};

//___________________________________________________________________________________________
//called when module is inserted into the kernel (insmod)

static int __init pp485_driver_init(void)
{
	printk("pp485_init\n");
	return pci_register_driver(&pp485_driver); //calls pp485_card_init.  pci_module_init() is obsolete
}

//___________________________________________________________________________________________
//called when module is removed from kernel (rmmod)
static void __exit pp485_driver_exit(void)
{
	//printk("pp485_exit, interrupts %i\n", interrupts);
	pci_unregister_driver(&pp485_driver); //calls pp485_card_exit
}
//___________________________________________________________________________________________

module_init(pp485_driver_init);
module_exit(pp485_driver_exit);

/* 
 * Get rid of taint message by declaring code as GPL. 
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */
