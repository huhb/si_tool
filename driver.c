#include "interface.h"

struct pci_access *pacc;
struct device *first_dev;

#if 0
struct driver drivers_list[] = {
	&nec_driver,
	&cs5536_ide_driver,
	&cs5536_usb_driver,
	&sb600_sata_driver,
	&sb600_usb_driver,
	&sb710_sata_driver,
	&sb710_usb_driver,
	NULL,
};
#endif

#if 0
int driver_register(drivers_list[index])
{
	int fd;

	fd = open(CONFIG_FILE, O_RDONLY);
	parse driver name
	if (match) {
		malloc_device_list
		fill config vendor id and deivce id in the driver id_tables[]; 
	}
}
#endif

struct driver *pci_find_driver(int vendor_id, int device_id)
{
//	walk through the driver_list and find match(vendor, device) driver
//	return driver
}

int type_netcard_handler(void)
{
	char	devbuf[128];
	int	dev_index = 0, num, ret;
	struct pci_dev *p;
	struct device *d;
	struct device_id net_devices[3];
	struct driver *selected_driver;

	memset((void*)&net_devices[0], 0, 3 * sizeof(struct device_id));

	for (d = first_dev; d; d = d->next) {
		p = d->dev;
		if (p->device_class == PCI_CLASS_NETWORK_ETHERNET) {
			printf("[%d] : %s\n", dev_index,
				pci_lookup_name(pacc, devbuf, sizeof(devbuf),
                         	PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
                         	p->vendor_id, p->device_id));
			net_devices[dev_index].vendor_id =  p->vendor_id;
			net_devices[dev_index].device_id = p ->device_id;
		}
	}
	printf("Please type device number to select test devices: ");
	scanf("%d", &num);
	selected_driver = pci_find_driver(net_devices[num].vendor_id,
					net_devices[num].device_id);

	if (!selected_driver) {
		printf("found no driver match\n");
		exit(-EINVAL);
	}

	printf("select device num %d\n", num);
	printf("[0] 10M mode\n");
	printf("[1] 100M mode\n");
	printf("[2] 1000M mode\n");
	printf("Please type test mode number to select: ");
	scanf("%d", &num);
	printf("select test mode %d\n", num);

	printf("start to setup test_mode.....");
	ret = 0;
	//selected_drivers.test_mode_handler(nmu);

	if (!ret)
		printf("succesfully\n");
	else 
		printf("failed\n");

	return ret;
}

int type_usb_handler(void)
{
#if 0	
	list same catgeory device
	user select device
	find  driver for device
	user select usb port 
	user select test_mode
	run test_mode
#endif	
}

int type_sata_handler(void)
{
#if 0
	list same catgeory device
	user select device
	find  driver for device
	user select slot
	user select test_mode
	run test_mode
#endif	
}

struct device *
scan_device(struct pci_dev *p)
{
  struct device *d;

  d = malloc(sizeof(struct device));
  memset(d, 0, sizeof(*d));
  d->dev = p;
  d->config_cached = d->config_bufsize = 64;
  d->config = malloc(64);
  d->present = malloc(64);
  memset(d->present, 1, 64);
  if (!pci_read_block(p, 0, d->config, 64))
    {
      fprintf(stderr, "lspci: Unable to read the standard configuration space header of device %04x:%02x:%02x.%d\n",
              p->domain, p->bus, p->dev, p->func);
    }
  pci_setup_cache(p, d->config, d->config_cached);
  pci_fill_info(p, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES | PCI_FILL_PHYS_SLOT);
  return d;
}

static void scan_devices(void)
{
  struct device *d;
  struct pci_dev *p;

  pci_scan_bus(pacc);
  for (p=pacc->devices; p; p=p->next)
    if (d = scan_device(p))
      {
        d->next = first_dev;
        first_dev = d;
      }
}

int drivers_init(void)
{
	/* scan all pci device and create devices_list */
	pacc = pci_alloc();
	pci_init(pacc);
	scan_devices();

//	for (arrary_size(drivers_list))
//		driver_register();	

	return 0;
}
