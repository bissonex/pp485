#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              // open 
#include <unistd.h>             // exit 
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include "pmc485.h"

#define PCI_CLK  		((1L<<14) + (1L<<13))
#define CLOCK_ENABLE 	(1L<<16)

int main(void) {

	int fd;
	int i;
	unsigned int portval;
	unsigned int retval;
	
	fd = open("/dev/pp485pci0",  O_RDONLY);
	if (fd < 0) 
	{
		printf("Can't open device file %s\n", "/dev/pp485pci0");
		exit(-1);
	}
	
	// enable PCI clock, no dividor
	portval = PCI_CLK | CLOCK_ENABLE;
	retval = ioctl(fd, PP485_IOCS_BASE, &portval);

	// Readback user dip switches
	retval = ioctl(fd, PP485_IOCG_SW, &portval); 

	//printf("retval = %d, data = %X\n", retval, data);
	
	printf("\n[pmc_par485_sw]\n");
	printf("\t1\t2\t3\t4\t5\t6\t7\t8\n");
	for(i = 7; i >= 0; i--)
	{
		if (portval & (1 << i))
		{
			printf("\tOn");
		}
		else
		{
			printf("\tOff");
		}
	}
	printf("\n");
	
	portval = 0x0001;
	retval = ioctl(fd, PP485_IOCS_DIR, &portval); 

	portval = 0x0000;
	retval = ioctl(fd, PP485_IOCS_DATAOUT, &portval); 

	retval = ioctl(fd, PP485_IOCG_DATAIN, &portval); 
	printf ("\n[pmc_par485_datain]\n\t0x%08X\n", portval);

	retval = ioctl(fd, PP485_IOCG_STAT, &portval); 
	printf ("\n[pmc_par485_stat]\n\t0x%01X\n", portval & 0x00000001);


	printf("\n");
	close(fd);
		
	return(0);
}
