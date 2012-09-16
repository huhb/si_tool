#include "interface.h"

struct pci_access *pacc;
struct device *first_dev;
struct driver *first_drv;

extern struct driver rtl8186_driver;
extern struct driver sb710_usb_driver;
#if 1
struct driver *drivers_list[] = {
	&rtl8186_driver,
	&sb710_usb_driver,
#if 0	
	&nec_driver,
	&cs5536_ide_driver,
	&cs5536_usb_driver,
	&sb600_sata_driver,
	&sb600_usb_driver,
	&sb710_sata_driver,
#endif	
	NULL,
};
#endif

#if 1
static int driver_register(struct driver *drv)
{
	FILE *file;
	char drv_name[DRV_NAME_LEN], buf[LINE_LEN], *pos;
	int id_num = 0, drv_match = 0;

	if (!drv)
		return -EINVAL;

	memset(drv_name, 0, DRV_NAME_LEN);
	memcpy(drv_name, drv->name, strlen(drv->name));
#ifdef xx
	printf("drv_name %s\n", drv_name);
#endif

#define CONFIG_FILE	"si_config"
	file = fopen(CONFIG_FILE, "r");
	if (file == NULL) {
		printf("not config file si_config");
		exit(-EINVAL);
	}

	while (fgets(buf, LINE_LEN, file)) {
		char *pos_tmp, *endptr[2];

		buf[LINE_LEN - 1] = '\0';
		pos = buf;

                /* Skip white space from the beginning of line. */
                while (*pos == ' ' || *pos == '\t' || *pos == '\r')
                        pos++;

                /* Skip comment lines and empty lines */
                if (*pos == '#' || *pos == '\n' || *pos == '\0')
                        continue;

		pos_tmp = strchr(pos, ' ');
		if (!pos_tmp) /* only driver name */
			continue;

		printf("post_tmp %s\n", pos_tmp);
		printf("pos2 %s\n", pos);
		if (!strncmp(drv_name, pos, \
					(pos_tmp - pos) > strlen(drv_name) ? \
					(pos_tmp - pos) : strlen(drv_name))) {
			printf("driver match\n");
			drv_match = 1;
			pos =  pos_tmp;
			while (*pos != '\n' || *pos != '\0') {
				while (*pos == ' ') {
					pos++;
				}
				if (*pos == '\n' || *pos == '\0')
					break;
				pos_tmp = strchr(pos, ':');
				if (!pos_tmp) {
					printf("format error\n");
					break;
				}
				drv->id_tables[id_num].vendor_id = strtol(pos, &endptr[0], 16);
				drv->id_tables[id_num].device_id = strtol(endptr[0] + 1, NULL, 16);
				id_num++;
				while (*pos != ' ') {
					pos++;
					if (*pos == '\n' || *pos == '\0')
						break;
				}
			}
		}
	}
	return drv_match;
}
#endif

static struct driver *find_driver(u16 vendor, u16 device)
{
	struct driver *d;
	struct device_id *ids;

	if (first_drv) {
		printf("first_drv ok\n");
		printf("first_drv name %s\n", first_drv->name);
		printf("fist_drv vendor id 0x%x, device 0x%x\n", first_drv->id_tables[0].vendor_id,
				first_drv->id_tables[0].device_id);
	}
	if (!first_drv) {
		printf("fist_drv is null\n");
	}

	for (d = first_drv; d; d = d->next) {
		for (ids = d->id_tables; (ids->vendor_id && ids->device_id); ids++) {
			printf("ids->vendor 0x%x, 0x%x\n", ids->vendor_id, ids->device_id);
			if ((ids->vendor_id == vendor) &&
				ids->device_id == device) {
				printf("find driver\n");
				return d;
			}
		}
	}
	return NULL;
}

static int query_devices(u16 dev_type, struct pci_dev **dev)
{
	struct device *d;
	struct pci_dev *p;
	char	devbuf[128];
	int	dev_index = 0;

	for (d = first_dev; d; d = d->next) {
		p = d->dev;
		if (p->device_class == dev_type) {
			if (dev_type == PCI_CLASS_SERIAL_USB) {
				if (p->cache[9] == 0x20) { /* only list ehci controler */
					printf("[%d] : %s\n", dev_index,
						pci_lookup_name(pacc, devbuf, sizeof(devbuf),
								PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
								p->vendor_id, p->device_id));
						dev[dev_index] = p;
						dev_index++;
				}
			} else {
				printf("[%d] : %s\n", dev_index,
					pci_lookup_name(pacc, devbuf, sizeof(devbuf),
                         	PCI_LOOKUP_VENDOR | PCI_LOOKUP_DEVICE,
                         	p->vendor_id, p->device_id));
				dev[dev_index] = p;
				dev_index++;
			}
		}
	}
	return dev_index;
}


static int run_driver_handler(struct driver* driver, void *param)
{
	int ret;

	printf("start to setup test_mode.....");
	printf("driver _handler %p\n", driver->driver_handler);
	if (driver->fixup)
		driver->fixup();

	if (driver->driver_handler)
		ret = driver->driver_handler(param);

	if (!ret) printf("successfully \n");
	else printf("faild\n");

	return ret;

}

int type_netcard_handler(void)
{
	int	num;
	struct net_param net_param;
	struct driver *driver;
	struct device_id net_device_ids[4];
	struct pci_dev *devices[4];

	memset((void*)net_device_ids, 0, 4 * sizeof(struct device_id));
	if (!query_devices(PCI_CLASS_NETWORK_ETHERNET, devices)) {
		printf("can't query NETWORK_ETHERNET device\n");
		return -ENODEV;
	}
	printf("Please select device: ");
	scanf("%d", &num);

	printf("select device 0x%x:0x%x\n", devices[num]->vendor_id,
					devices[num]->device_id);
	net_param.dev = devices[num];

	driver = find_driver(devices[num]->vendor_id,
					devices[num]->device_id);
	if (!driver) {
		printf("found no driver match\n");
		exit(-EINVAL);
	}

	printf("[0] 10M mode\n"
		"[1] 100M mode\n"
		"[2] 1000M mode\n");

	printf("Please select test mode: ");
	scanf("%d", &num);
	net_param.mode = num;
	return run_driver_handler(driver, &net_param);
}

int type_usb_handler(void)
{
#define USB_CONTROLER_NUM 7
	int	num;
	struct usb_param usb;
	struct driver *driver;
	struct pci_dev *usb_devices[USB_CONTROLER_NUM];

	if (!query_devices(PCI_CLASS_SERIAL_USB, usb_devices)) {
		printf("can't query USB_CONTROLER device\n");
		return -ENODEV;
	}
	printf("Please select device: ");
	scanf("%d", &num);
	usb.dev = usb_devices[num];

	driver = find_driver(usb_devices[num]->vendor_id,
					usb_devices[num]->device_id);
	if (!driver) {
		printf("found no driver match\n");
		exit(-EINVAL);
	}

	printf("Please select usb port: ");
	scanf("%d", &usb.port_num);
	printf("[1] J_STATE mode\n"
		"[2] K_STATE mode\n"
		"[3] SE0_NAK mode\n"
		"[4] Packet\n");

	printf("Please select test mode: ");
	scanf("%d", &usb.mode);

	return run_driver_handler(driver, &usb);

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

struct device *scan_device(struct pci_dev *p)
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
	pci_fill_info(p, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_IRQ | \
			PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES | PCI_FILL_PHYS_SLOT);
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
	int drv_num;

	/* scan all pci device and create devices_list */
	pacc = pci_alloc();
	pci_init(pacc);
	scan_devices();

	for (drv_num = 0; drivers_list[drv_num]; drv_num++) {
		if (driver_register(drivers_list[drv_num])) {
			drivers_list[drv_num]->next = first_drv;
			first_drv = drivers_list[drv_num];
		}
	}

	return 0;
}
