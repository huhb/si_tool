#include "interface.h"

int rtl8186_handler(void *param)
{
	int test_mode;
	struct net_param *net_param = param;

	if (!param) {
		printf("net param is null\n");
		exit(-EINVAL);
	}
	test_mode = net_param->mode;

	DEBUG("rtl8186, test_mode %d\n", test_mode);

	return 0;
}

struct driver rtl8186_driver = {
	.name = "rtl8186",
	.driver_handler = rtl8186_handler,
};
