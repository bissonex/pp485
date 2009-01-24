#ifndef _PMC485_H
#define _PMC485_H

#define PP485_IOC_MAGIC			'p'

/* ioctl commands. This is probably overkill, but it's "right"...*/

#define PP485_IOCG_SW		_IOR(PP485_IOC_MAGIC, 1, unsigned int)
#define PP485_IOCG_DATAIN	_IOR(PP485_IOC_MAGIC, 2, unsigned int)
#define PP485_IOCG_STAT		_IOR(PP485_IOC_MAGIC, 3, unsigned int)
#define PP485_IOCS_DATAOUT	_IOW(PP485_IOC_MAGIC, 4, unsigned int)
#define PP485_IOCS_DIR		_IOW(PP485_IOC_MAGIC, 5, unsigned int)
#define PP485_IOCG_DIR		_IOR(PP485_IOC_MAGIC, 6, unsigned int)
#define PP485_IOCS_TERM		_IOW(PP485_IOC_MAGIC, 7, unsigned int)
#define PP485_IOCG_TERM		_IOR(PP485_IOC_MAGIC, 8, unsigned int)
#define PP485_IOCS_BASE		_IOW(PP485_IOC_MAGIC, 9, unsigned int)
#define PP485_IOCG_BASE		_IOR(PP485_IOC_MAGIC, 10, unsigned int)

#define PP485_IOC_MAXNR 5

#endif /* _PMC485_H */
