#include "interface.h"

int sb710_usb_handler(void *param)
{
	u16 caplength;
	u32 portsc;
	int port_num, test_mode, reg, port_nr, index;
	struct pci_dev *dev;
	struct usb_param *sb710_usb_param;

	if (!param)
		goto ERROR;

	sb710_usb_param = param;
	port_nr = sb710_usb_param->port_nr;

	/* check parames */
	if (sb710_usb_param->mode <= 0)
		goto ERROR;
	if (!sb710_usb_param->dev)
		goto ERROR;
	for (index = 0; index < port_nr; index++)
		if (sb710_usb_param->port_num[index] < 0)
			goto ERROR;

	for (index = 0; index < port_nr; index++) {
		port_num = *(sb710_usb_param->port_num + index);
		test_mode = sb710_usb_param->mode;
		DEBUG("set sb710 usb port %d, test mode %d\n", port_num, test_mode);
		dev = sb710_usb_param->dev;
		caplength = read_device_regs(dev, 0, 0, 1);
		DEBUG("caplength 0x%x\n",caplength);
		reg = (caplength + 0x44 + 4 * port_num);
		portsc = read_device_regs(dev, 0, reg, 4);
		DEBUG("port %d, portsc 0x%x\n", port_num, portsc);
		//write_device_regs(dev, 0, reg, test_mode << 16);
	}
	return 0;
ERROR:
	printf("param is invalid\n");
	return -EINVAL;
}

struct driver sb7xx_usb_driver = {
	.name = "sb7xx_usb",
	.driver_handler = sb710_usb_handler,
};
