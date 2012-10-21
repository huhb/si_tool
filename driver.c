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
		if (!pos_tmp)	/* only driver name */
			continue;

		DEBUG("post_tmp %s\n", pos_tmp);
		DEBUG("pos2 %s\n", pos);
		if (!strncmp(drv_name, pos,
			     (pos_tmp - pos) > strlen(drv_name) ?
			     (pos_tmp - pos) : strlen(drv_name))) {
			DEBUG("driver match\n");
			drv_match = 1;
			pos = pos_tmp;
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
				drv->id_tables[id_num].vendor_id =
				    strtol(pos, &endptr[0], 16);
				drv->id_tables[id_num].device_id =
				    strtol(endptr[0] + 1, NULL, 16);
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
		DEBUG("fist_drv vendor id 0x%x, device 0x%x\n",
		      first_drv->id_tables[0].vendor_id,
		      first_drv->id_tables[0].device_id);
	}
	if (!first_drv) {
		printf("fist_drv is null\n");
	}

	for (d = first_drv; d; d = d->next) {
		for (ids = d->id_tables; (ids->vendor_id && ids->device_id);
		     ids++) {
			DEBUG("ids->vendor 0x%x, 0x%x\n", ids->vendor_id,
			      ids->device_id);
			if ((ids->vendor_id == vendor)
			    && ids->device_id == device) {
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
	char devbuf[128];
	int dev_index = 0;

	for (d = first_dev; d; d = d->next) {
		p = d->dev;
		if (p->device_class == dev_type) {
			if (dev_type == PCI_CLASS_SERIAL_USB) {
				if (p->cache[9] == 0x20) {	/* only list ehci controler */
					printf("[%d] : %s\n", dev_index,
					       pci_lookup_name(pacc, devbuf,
							       sizeof(devbuf),
							       PCI_LOOKUP_VENDOR
							       |
							       PCI_LOOKUP_DEVICE,
							       p->vendor_id,
							       p->device_id));
					dev[dev_index] = p;
					dev_index++;
				}
			} else {
				printf("[%d] : %s\n", dev_index,
				       pci_lookup_name(pacc, devbuf,
						       sizeof(devbuf),
						       PCI_LOOKUP_VENDOR |
						       PCI_LOOKUP_DEVICE,
						       p->vendor_id,
						       p->device_id));
				dev[dev_index] = p;
				dev_index++;
			}
		}
	}
	return dev_index;
}

static int run_driver_handler(struct driver *driver)
{
	int ret;

	printf("start to setup test_mode.....");
	DEBUG("driver_handler %p\n", driver->driver_handler);
	if (driver->fixup)
		driver->fixup();

	if (driver->driver_handler)
		ret = driver->driver_handler(driver->drv_data);

	if (!ret)
		printf("successfully \n");
	else
		printf("faild\n");

	return ret;
}

static void check_exit(char *str)
{
	if (str && !strncmp(str, "quit", 4)) {
		printf("Programe exit\n");
		exit(0);
	}
}

static int handle_user_input(char *u_in, int *dev_num, int max_num)
{
	char *p;

	fgets(u_in, INPUT_LENGTH, stdin);
	check_exit(u_in);
	while (*u_in == '\0') {
		printf("Input error\n");
		return -EINVAL;
	}
	*dev_num = strtol(u_in, &p, 0);
	if (*dev_num >= max_num || u_in == p) {
		printf("Input error\n");
		return -EINVAL;
	}
	return 0;
}

static int type_all_handler(u16 device_type)
{
	int num, ret, dev_num, handle_step;
	char user_input[INPUT_LENGTH], *p, *temp_p;
	struct usb_param usb;
	struct net_param net;
	struct sata_param sata;
	struct driver *driver;
	struct pci_dev *devices[USB_CONTROLER_NUM];

	num = query_devices(device_type, devices);
	if (num == 0) {
		printf("can't query device\n");
		return -ENODEV;
	}

	handle_step = DEVICE_SELECT;
	while (1) {	/* check_exit() terminate the programe */
		memset(user_input, 0, INPUT_LENGTH);
		switch (handle_step) {
		case DEVICE_SELECT:
			if (num == 1)
				printf("Please select device [0]: ");
			else
				printf("Please select device [0 - %d]: ",
				       num - 1);

			ret = handle_user_input(user_input, &dev_num, num);
			if (ret < 0)
				break;

			driver = find_driver(devices[dev_num]->vendor_id,
					     devices[dev_num]->device_id);
			if (driver == NULL) {
				printf("Device [%d], no driver match\n",
				       dev_num);
				break;
			}
			if (device_type == PCI_CLASS_NETWORK_ETHERNET)
				handle_step = NET_MODE_SELECT;
			else
				handle_step = PORT_INPUT;
			break;
		case PORT_INPUT:
			printf("Please select %s for Device[%d]: ",
			       device_type == PCI_CLASS_SERIAL_USB
			       ? "usb port" : "sata slot", dev_num);
			fgets(user_input, INPUT_LENGTH, stdin);
			check_exit(user_input);
			ret = strtol(user_input, &p, 0);
			if (user_input == p || ret >= MAX_USB_PORT) {
				printf("input error\n");
				break;
			}
			usb.port_nr = 1;
			sata.slot_nr = 1;
			usb.port_num[0] = ret;
			sata.slot[0] = ret;
			for (p; *p == ' '; p = temp_p) {
				ret = strtol(p, &temp_p, 0);
				if (p == temp_p || ret >= MAX_USB_PORT) {
					printf("input error\n");
					break;
				}
				usb.port_num[usb.port_nr++] = ret;
				sata.slot[sata.slot_nr++] = ret;
			}
			if (device_type == PCI_CLASS_SERIAL_USB)
				handle_step = USB_MODE_SELECT;
			if (device_type == PCI_CLASS_STORAGE_SATA)
				handle_step = SATA_MODE_SELECT;
			break;
		case USB_MODE_SELECT:
			printf("[1] J_STATE mode\n"
			       "[2] K_STATE mode\n"
			       "[3] SE0_NAK mode\n"
			       "[4] Packet\n");
			printf("Please select test mode: ");
			ret = scanf("%d", &usb.mode);
			/* work around that input stream can't be flushed '\n' or error input */
			fgets(user_input, 5, stdin);
			check_exit(user_input);
			if (ret == 1) {
				if ((usb.mode > 4) || (usb.mode < 1)) {
					printf("Input error. should be 1 ~ 4\n");
					break;
				}
			} else {
				printf("Input error. should be 1 ~ 4\n");
				break;
			}
			DEBUG("select mode is %d\n", usb.mode);
			usb.dev = devices[dev_num];
			driver->drv_data = &usb;
			handle_step = HANDLER_RUN;
			break;
		case NET_MODE_SELECT:
			printf("[0] 10M mode\n"
			       "[1] 100M mode\n"
			       "[2] 1000M mode\n");
			printf("Please select test mode: ");
			ret = scanf("%d", &net.mode);
			/* work around that input stream can't be flushed '\n' or error input */
			fgets(user_input, INPUT_LENGTH, stdin);
			check_exit(user_input);
			if (ret == 1) {
				if (net.mode < 0 || net.mode > 2) {
					printf("input error, should be 0 ~ 2\n");
					break;
				}
			} else {
				printf("input error, should be 0 ~ 2\n");
				break;
			}
			net.dev = devices[dev_num];
			driver->drv_data = &net;
			handle_step = HANDLER_RUN;
			break;
		case SATA_MODE_SELECT:
			printf("[0] SATA1 mode\n"
			       "[1] SATA2 mode\n");
			printf("Please select test mode: ");
			ret = scanf("%d", &sata.mode);
			/* work around that input stream can't be flushed */
			fgets(user_input, INPUT_LENGTH, stdin);
			check_exit(user_input);
			if (ret == 1) {
				if ((sata.mode > 4) || (sata.mode < 1)) {
					printf("Input error. should be 0 ~ 1\n");
					break;
				}
			} else {
				printf("Input error. should be 0 ~ 1\n");
				break;
			}
			sata.dev = devices[dev_num];
			driver->drv_data = &sata;
			handle_step = HANDLER_RUN;
			break;
		case HANDLER_RUN:
			ret = run_driver_handler(driver);
			if (ret != 0)
				printf("run handler error\n");
			handle_step = DEVICE_SELECT;
		}
	}
	return ret;
}

int type_netcard_handler(void)
{
	return type_all_handler(PCI_CLASS_NETWORK_ETHERNET);
}

int type_usb_handler(void)
{
	return type_all_handler(PCI_CLASS_SERIAL_USB);
}

int type_sata_handler(void)
{
	return type_all_handler(PCI_CLASS_STORAGE_SATA);
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
	if (!pci_read_block(p, 0, d->config, 64)) {
		fprintf(stderr, "lspci: Unable to read the standard \
			    configuration space header of device %04x:%02x:%02x.%d\n", p->domain, p->bus, p->dev, p->func);
	}
	pci_setup_cache(p, d->config, d->config_cached);
	pci_fill_info(p, PCI_FILL_IDENT | PCI_FILL_CLASS | PCI_FILL_IRQ |
		      PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES |
		      PCI_FILL_PHYS_SLOT);
	return d;
}

static void scan_devices(void)
{
	struct device *d;
	struct pci_dev *p;

	pci_scan_bus(pacc);
	for (p = pacc->devices; p; p = p->next)
		if (d = scan_device(p)) {
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
