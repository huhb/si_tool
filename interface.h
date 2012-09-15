#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pci/pci.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct device_id {
	unsigned int vendor_id, device_id;
};

struct device {
  struct device *next;
  struct pci_dev *dev;
  unsigned int config_cached, config_bufsize;
  unsigned char *config;                         /* Cached configuration space data */
  unsigned char *present;                        /* Maps which configuration bytes are present */
};

struct driver {
	struct device_id *id_tables;
	int *genric_handler;
	int own_handler;
	int fixup;
};
