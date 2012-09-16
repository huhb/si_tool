#include "interface.h"

static void useages(void)
{
	printf("programe useage: \n \
		si_tool netcard #Test netcard\n \
		si_tool sata #Test sata\n \
		si_tool usb #Test usb\n");
}

int main(int argc, char **argv)
{
	/* parse user argument */
	if (argc < 2)
		goto USEAGES;
	drivers_init();

	if (!strcmp(argv[1], "netcard")) {
		return type_netcard_handler();
	}
	if (!strcmp(argv[1], "sata")) {
		return type_sata_handler();
	}
	if (!strcmp(argv[1], "usb")) {
		return type_usb_handler();
	}
USEAGES:
	useages();
	return -EINVAL;
}
