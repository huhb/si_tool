#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pci/pci.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DRV_NAME_LEN 128
#define LINE_LEN	1024
#define DEV_NUM	10

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
