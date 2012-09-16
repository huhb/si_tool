#include <sys/mman.h>
#include "interface.h"

#define PAGE_MASK (~(0x4000-1))  /* 16K pagesize */

void *phy_to_virt(unsigned long phy)
{
	int fd, size = 0x4000;
	void *va;

	fd = open("/dev/mem", O_RDWR);
	if(fd < 0) {
		printf("open /dev/mem error，maybe root，try `sudo su`\n");
		exit(-EINVAL);
	}

	va =  mmap((void *)0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, phy & PAGE_MASK);
	close(fd);

	return (void*)(va + (phy & ~PAGE_MASK)); /* real programe address */
}

unsigned int read_device_regs(struct pci_dev *dev, int base_num, int reg, int size)
{
	void *va;
	unsigned long phy;

	/* get phy address */
	phy = dev->base_addr[base_num] & ~1;
	DEBUG("base %d, addr 0x%lx\n", base_num, phy);
	va = phy_to_virt(phy);

	switch(size) {
	case 1:/* byte read */
		return *(volatile unsigned char*)(va + reg);
	case 2:/* half word */
		return *(volatile u16*)(va + reg);
	case 4: /* word */
		return *(volatile u32*)(va + reg);
	}
}

void write_device_regs(struct pci_dev *dev, int base_num, int reg, unsigned int value)
{
	void *va;
	unsigned long phy;

	phy = dev->base_addr[base_num] & ~1;
	va = phy_to_virt(phy);
	*(volatile unsigned int*)(va + reg) = value;
}
