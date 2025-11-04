/**
 * MA-Memphis
 * @file main.c
 *
 * @date October 2025
 * 
 * @brief Main volume observer file
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <memphis.h>
#include <memphis/messaging.h>
#include <memphis/monitor.h>
#include <memphis/services.h>
#include <memphis/oda.h>
#include <memphis/messaging.h>

#define MAX_HOPS_SIZE 256

int main()
{
	printf("Volume monitor started at %d\n", memphis_get_tick());

	static oda_t observer;
	oda_init(&observer);

	int ret = memphis_mkfifo(sizeof(memphis_vol_monitor_t), 64);
	if (ret != 0)
		return ret;

	mon_announce(MON_VOL);

	uint16_t flits_hop[MAX_HOPS_SIZE];
	for (uint16_t array_index = 0; array_index < MAX_HOPS_SIZE; array_index++)
	{
		flits_hop[array_index] = 0;
	}

	while (true) {
		static memphis_vol_monitor_t message;
		memphis_receive_any(&message, sizeof(memphis_vol_monitor_t));
		switch (message.service) {
			case VOL_MONITOR:
				flits_hop[message.hops] += message.size;
				break;
			case TERMINATE_ODA:
				printf("(VOL_MON) Flits transit:\n");

				for (uint16_t hops_index = 0; hops_index < MAX_HOPS_SIZE; hops_index++)
				{
					if (flits_hop[hops_index] > 0)
					{
						printf("(VOL_MON) 	Hops[%u]=%u\n", hops_index, flits_hop[hops_index]);
					}
				}
				return 0;
			default:
				break;
		}
	}

	return 0;
}
