#include <getopt.h>
#include "interface.h"

int debug = 0;  /* 0 off, 1 on */
int handled_device = 0;
char config_path[CONFIG_FILE_LEN];

struct option longopts[] = {
	{ "usb", no_argument, &handled_device, 1},
	{ "netcard", no_argument, &handled_device, 2},
	{ "sata", no_argument, &handled_device, 3},
	{ "debug", no_argument, NULL, 'd'},
	{ "help", no_argument, NULL, 'h'},
	{ "version", no_argument, NULL, 'v'},
	{ "config", required_argument, NULL, 'c'},
	{ 0, 0, 0, 0},
};

static void usage(void)
{
	printf("Usage: si_tool [options]...\n" 
		"Valid options.\n"
		"  --usb                   Test usb\n"
		"  --sata                  Test sata\n"
		"  --netcard               Test netcard\n"
		"  -d, --debug             print debug info\n"
		"  --config=config_path    change config file (default: `PWD`/si_config)\n"
		"  --help                  display this help and exit\n"
		"  --version               output version and exit\n"
		"\n"
		"  If you have problem or some bugs to report, "
		"contact HuHongBing <huhb@lemote.com>;\n");
}

int main(int argc, char **argv)
{
	char c;
	int ret;

	/* parse user argument */
	if (argc < 2)
		goto USAGE;
	while ((c = getopt_long(argc, argv, "d", longopts, NULL)) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case '?':
		case 'h':
		case ':':
			goto USAGE;
			break;
		case 'v':
			printf("version %s\n", SI_VERSION);
			return 0;
		case 'c':
			memset(config_path, 0, CONFIG_FILE_LEN);
			strncpy(config_path, optarg, strlen(optarg));
			break;
		case 0:
			break;
		}
	}

	if (handled_device)
		init_drivers();

	switch(handled_device) {
	case 1:
		ret = type_usb_handler();
		break;
	case 2:
		ret = type_netcard_handler();
		break;
	case 3:
		ret = type_sata_handler();
		break;
	default:
		printf("No device handle\n"
				"Arguments should have \"--usb\" or \"--netcard\" or \"--sata\"\n");
		return -EINVAL;
	}

	return ret;
USAGE:
	usage();
	return -EINVAL;
}
