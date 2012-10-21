#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pci/pci.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SI_VERSION  "0.0.2"
#define DEV_NUM	10
#define LINE_LEN	1024
#define DRV_NAME_LEN 128
#define CONFIG_FILE_LEN 256
#define INPUT_LENGTH 256

#define MAX_USB_PORT 32
#define MAX_SATA_SLOT 32

#define NET_CONTROLER_NUM 3
#define USB_CONTROLER_NUM 7
#define SATA_CONTROLER_NUM 7

enum {
	DEVICE_SELECT	= 0,
	PORT_INPUT,
	USB_MODE_SELECT,
	NET_MODE_SELECT,
	SATA_MODE_SELECT,
	HANDLER_RUN,
};

extern int debug;
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
	void *drv_data;
};

struct net_param {
	int mode;
	struct pci_dev *dev;
};

struct usb_param {
	int port_num[MAX_USB_PORT];
	int port_nr;
	int mode;
	struct pci_dev *dev;
};

struct sata_param {
	int slot[MAX_SATA_SLOT];
	int slot_nr;
	int mode;
	struct pci_dev *dev;
};

#define DEBUG(format...) do {if (debug) printf(format);} while(0)
//extern int read_device_regs(struct pci_dev *dev, int base_num, int reg, int size);
//extern void wirte_device_regs(struct pci_dev *dev, int base_num, int reg, unsigned int value);
