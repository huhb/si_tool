#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pci/pci.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SI_VERSION  "0.0.1"
#define DEV_NUM	10
#define LINE_LEN	1024
#define DRV_NAME_LEN 128
#define CONFIG_FILE_LEN 256

struct device_id {
	u16 vendor_id, device_id;
};

struct device {
  struct device *next;
  struct pci_dev *dev;
  unsigned int config_cached, config_bufsize;
  unsigned char *config;                         /* Cached configuration space data */
  unsigned char *present;                        /* Maps which configuration bytes are present */
};

struct driver {
	char name[DRV_NAME_LEN];
	struct device_id id_tables[DEV_NUM];
	int (*fixup)(void);
	int (*driver_handler)(void* parames);
	struct driver *next;
};

struct net_param {
	int mode;
	struct pci_dev *dev;
};

struct usb_param {
	int port_num;
	int mode;
	struct pci_dev *dev;
};

struct sata_param {
	int slot;
	int mode;
	struct pci_dev *dev;
};

#define DEBUG(format...) do {if (debug) printf(format);} while(0)
//extern int read_device_regs(struct pci_dev *dev, int base_num, int reg, int size);
//extern void wirte_device_regs(struct pci_dev *dev, int base_num, int reg, unsigned int value);
