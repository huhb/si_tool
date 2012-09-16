#include <getopt.h>
#include <unistd.h>
#include "interface.h"

int debug = 0;  /* 0 off, 1 on */

struct option longopts[] = {
	{ "usb", no_argument, NULL, 'u'},
	{ "net", no_argument, NULL, 'n'},
	{ "sata", no_argument, NULL, 's'},
	{ "debug", no_argument, NULL, 'd'},
	{ "help", no_argument, NULL, 'h'},
	{ "version", no_argument, NULL, 'v'},
	{ 0, 0, 0, 0},
};

static void usage(void)
{
	printf("Usage: si_tool [options]...\n" 
		"Valid options.\n"
		"  --usb        Test usb\n"
		"  --sata       Test sata\n"
		"  --netcard    Test netcard\n"
		"  -d, --debug  print debug info\n"
		"  --help       display this help and exit\n"
		"  --version    output version and exit\n");
}

int main(int argc, char **argv)
{
	char c;
	int ret, handled = 0;

	/* parse user argument */
	if (argc < 2)
		goto USAGE;
	drivers_init();

	while ((c = getopt_long(argc, argv, "d", longopts, NULL)) != -1) {
		switch (c) {
		case 'u':
			handled = 1;
			ret = type_usb_handler();
			break;
		case 'n':
			handled = 1;
			ret = type_netcard_handler();
			break;
		case 's':
			handled = 1;
			ret = type_sata_handler();
			break;
		case 'd':
			debug = 1;
			break;
		case '?':
		case 'h':
			goto USAGE;
			break;
		case 'v':
			printf("version %s\n", SI_VERSION);
			return 0;
		}
	}

	if (!handled)
	  goto USAGE;

	return ret;
USAGE:
	usage();
	return -EINVAL;
}
