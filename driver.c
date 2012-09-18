#include "interface.h"

struct pci_access *pacc;
struct device *first_dev;
struct driver *first_drv;

extern char config_path[LINE_LEN];
extern struct driver rtl8186_driver;
extern struct driver sb7xx_usb_driver;

struct driver *drivers_list[] = {
	&rtl8186_driver,
	&sb7xx_usb_driver,
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

static int register_driver(struct driver *drv)
{
	FILE *file;
	char drv_name[DRV_NAME_LEN], buf[LINE_LEN], *pos;
	int id_num = 0, drv_match = 0;

	if (!drv)
		return -EINVAL;

	memset(drv_name, 0, DRV_NAME_LEN);
	memcpy(drv_name, drv->name, strlen(drv->name));
	DEBUG("drv_name %s\n", drv_name);

#define CONFIG_FILE	"si_config"
	if (config_path[0] != '\0')
		file = fopen(config_path, "r");
	else
		file = fopen(CONFIG_FILE, "r");

	if (file == NULL) {
		printf("No config file exist\n");
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

		DEBUG("post_tmp %s\n", pos_tmp);
		DEBUG("pos2 %s\n", pos);
		if (!strncmp(drv_name, pos, \
					(pos_tmp - pos) > strlen(drv_name) ? \
					(pos_tmp - pos) : strlen(drv_name))) {
			DEBUG("driver match\n");
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

static struct driver *find_driver(u16 vendor, u16 device)
{
	struct driver *d;
	struct device_id *ids;

	if (first_drv) {
		DEBUG("first_drv ok\n");
		DEBUG("first_drv name %s\n", first_drv->name);
		DEBUG("fist_drv vendor id 0x%x, device 0x%x\n", first_drv->id_tables[0].vendor_id,
				first_drv->id_tables[0].device_id);
	}
	if (!first_drv) {
		printf("fist_drv is null\n");
	}

	for (d = first_drv; d; d = d->next) {
		for (ids = d->id_tables; (ids->vendor_id && ids->device_id); ids++) {
			DEBUG("ids->vendor 0x%x, 0x%x\n", ids->vendor_id, ids->device_id);
			if ((ids->vendor_id == vendor) &&
				ids->device_id == device) {
				DEBUG("find driver\n");
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

static int run_driver_handler(struct driver* driver)
{
	int ret;

	printf("start to setup test_mode.....");
	DEBUG("driver_handler %p\n", driver->driver_handler);
	if (driver->fixup)
		driver->fixup();

	if (driver->driver_handler)
		ret = driver->driver_handler(driver->drv_data);

	if (!ret) printf("successfully \n");
	else printf("faild\n");

	return ret;
}

static int handle_user_input(char *u_in, int *array, int max_num)
{
	int index = 0;
	char *p;

	fgets(u_in, INPUT_LENGTH, stdin);
	while (*u_in == '\0') {
		printf("Input error\n");
		return -EINVAL;
	}
	for (p = u_in; p; p = strchr(p, ' '), index++) {
		while (*p == ' ')
			p++;
		array[index] = strtol(p, NULL, 10);
		if (array[index] > max_num) {
			printf("Input number is larger than max_num\n");
			return -EINVAL;
		}
	}
	return index;
}

int type_netcard_handler(void)
{
#define NET_CONTROLER_NUM 3
	int	dev_num, dev_array[NET_CONTROLER_NUM], index, ret, i, drv_num = 0;
	char	user_input[INPUT_LENGTH], *p;
	struct net_param net[NET_CONTROLER_NUM];
	struct driver *drivers[NET_CONTROLER_NUM];
	struct pci_dev *net_devices[NET_CONTROLER_NUM];

	dev_num = query_devices(PCI_CLASS_NETWORK_ETHERNET, net_devices);
	if (dev_num == 0) {
		printf("can't query USB_CONTROLER device\n");
		return -ENODEV;
	}
	if (dev_num == 1)
		printf("Please select device [0]: ");
	else
		printf("Please select device [0 - %d]: ", dev_num-1);

	memset(user_input, 0, INPUT_LENGTH);
	memset(dev_array, 0, NET_CONTROLER_NUM);
	ret = handle_user_input(user_input, dev_array, dev_num);
	if (ret <= 0)
	       return ret;

	for (i = 0; i < ret; i++) {
		int handled_dev_num;

		handled_dev_num = dev_array[i];
		drivers[i] = find_driver(net_devices[handled_dev_num]->vendor_id,
					net_devices[handled_dev_num]->device_id);
		if (drivers[i] == NULL) {
			printf("Device [%d] no driver match\n", handled_dev_num);
			continue;
		}
		memset(user_input, 0, INPUT_LENGTH);
		printf("[0] 10M mode\n"
			"[1] 100M mode\n"
			"[2] 1000M mode\n");
		printf("Please select test mode: ");
		scanf("%d", &net[i].mode);

		net[i].dev = net_devices[handled_dev_num];
		drivers[i]->drv_data = &net[i];
		drv_num++;
	}
	for (i = 0; i < drv_num; i++) {
		ret = run_driver_handler(drivers[i]);
		if (ret != 0)
			return ret;
	}
	return ret;
}

int type_usb_handler(void)
{
#define USB_CONTROLER_NUM 7
	int	dev_num, dev_array[USB_CONTROLER_NUM], index, ret, i, drv_num = 0;
	char	user_input[INPUT_LENGTH], *p;
	struct usb_param usb[USB_CONTROLER_NUM];
	struct driver *drivers[USB_CONTROLER_NUM];
	struct pci_dev *usb_devices[USB_CONTROLER_NUM];

	dev_num = query_devices(PCI_CLASS_SERIAL_USB, usb_devices);
	if (dev_num == 0) {
		printf("can't query USB_CONTROLER device\n");
		return -ENODEV;
	}
	if (dev_num == 1)
		printf("Please select device [0]: ");
	else
		printf("Please select device [0 - %d]: ", dev_num-1);

	memset(user_input, 0, INPUT_LENGTH);
	memset(dev_array, 0, USB_CONTROLER_NUM);
	ret = handle_user_input(user_input, dev_array, dev_num);
	if (ret <= 0)
	       return ret;

	for (i = 0; i < ret; i++) {
		int handled_dev_num;

		handled_dev_num = dev_array[i];
		drivers[i] = find_driver(usb_devices[handled_dev_num]->vendor_id,
					usb_devices[handled_dev_num]->device_id);
		if (drivers[i] == NULL) {
			printf("Device [%d] no driver match\n", handled_dev_num);
			continue;
		}
		memset(user_input, 0, INPUT_LENGTH);
PORT_INPUT:
		printf("Please select usb port for Device[%d]: ", dev_array[i]);
		usb[i].port_nr = handle_user_input(user_input, usb[i].port_num, MAX_USB_PORT);
		if (usb[i].port_nr <= 0) /* input error */
			goto PORT_INPUT;
		printf("[1] J_STATE mode\n"
			"[2] K_STATE mode\n"
			"[3] SE0_NAK mode\n"
			"[4] Packet\n");
MODE_SELECT:
		printf("Please select test mode: ");
		if (scanf("%d", &usb[i].mode) == 1) {
			if ((usb[i].mode > 4) || (usb[i].mode < 1)) {
				printf("Input error. should be 1 ~ 4\n");
				goto MODE_SELECT;
			}
		} else {
			int garbage[128];
			/* work around that input stream can't be flushed */ 
			scanf("%s", garbage);
			printf("Input error. should be 1 ~ 4\n");
			goto MODE_SELECT;
		}
		DEBUG("select mode is %d\n", usb[i].mode);
		usb[i].dev = usb_devices[handled_dev_num];
		drivers[i]->drv_data = &usb[i];
		drv_num++;
	}
	for (i = 0; i < drv_num; i++) {
		ret = run_driver_handler(drivers[i]);
		if (ret != 0)
			return ret;
	}
	return ret;
}

int type_sata_handler(void)
{
#define SATA_CONTROLER_NUM 7
	int	dev_num, dev_array[SATA_CONTROLER_NUM], index, ret, i, drv_num = 0;
	char	user_input[INPUT_LENGTH], *p;
	struct sata_param sata[SATA_CONTROLER_NUM];
	struct driver *drivers[SATA_CONTROLER_NUM];
	struct pci_dev *sata_devices[SATA_CONTROLER_NUM];

	dev_num = query_devices(PCI_CLASS_STORAGE_SATA, sata_devices);
	if (dev_num == 0) {
		printf("can't query STAT_CONTROLER device\n");
		return -ENODEV;
	}
	if (dev_num == 1)
		printf("Please select device [0]: ");
	else
		printf("Please select device [0 - %d]: ", dev_num-1);

	memset(user_input, 0, INPUT_LENGTH);
	memset(dev_array, 0, SATA_CONTROLER_NUM);
	ret = handle_user_input(user_input, dev_array, dev_num);
	if (ret <= 0)
	       return ret;

	for (i = 0; i < ret; i++) {
		int handled_dev_num;

		handled_dev_num = dev_array[i];
		drivers[i] = find_driver(sata_devices[handled_dev_num]->vendor_id,
					sata_devices[handled_dev_num]->device_id);
		if (drivers[i] == NULL) {
			printf("Device [%d] no driver match\n", handled_dev_num);
			continue;
		}
		memset(user_input, 0, INPUT_LENGTH);
		printf("Please select sata slot for Device[%d]: ", dev_array[i]);
		sata[i].slot_nr = handle_user_input(user_input, sata[i].slot, MAX_STAT_SLOT);
		printf("[0] SATA1 mode\n"
			"[1] SATA2 mode\n");
MODE_SELECT:
		printf("Please select test mode: ");
		if (scanf("%d", &sata[i].mode) == 1) {
			if ((sata[i].mode > 4) || (sata[i].mode < 1)) {
				printf("Input error. should be 1 ~ 4\n");
				goto MODE_SELECT;
			}
		} else {
			int garbage[128];
			/* work around that input stream can't be flushed */
			scanf("%s", garbage);
			printf("Input error. should be 1 ~ 4\n");
			goto MODE_SELECT;
		}

		sata[i].dev = sata_devices[handled_dev_num];
		drivers[i]->drv_data = &sata[i];
		drv_num++;
	}
	for (i = 0; i < drv_num; i++) {
		ret = run_driver_handler(drivers[i]);
		if (ret != 0)
			return ret;
	}
	return ret;
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
	    fprintf(stderr, "lspci: Unable to read the standard \
			    configuration space header of device %04x:%02x:%02x.%d\n",
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

int init_drivers(void)
{
	int drv_num;

	/* scan all pci device and create devices_list */
	pacc = pci_alloc();
	pci_init(pacc);
	scan_devices();

	for (drv_num = 0; drivers_list[drv_num]; drv_num++) {
		if (register_driver(drivers_list[drv_num])) {
			drivers_list[drv_num]->next = first_drv;
			first_drv = drivers_list[drv_num];
		}
	}

	return 0;
}
