#include "interface.h"

int rtl8186_handler(void *param)
{
	int test_mode;

	if (!param) {
		printf("Test mode not found\n");
		exit(-EINVAL);
	}
	test_mode = *(int*)param;	
	printf("test_mode %d\n", test_mode);

	return 0;	
}

struct driver rtl8186_driver = {
	.name = "rtl8186",
	.driver_handler = rtl8186_handler,
};
