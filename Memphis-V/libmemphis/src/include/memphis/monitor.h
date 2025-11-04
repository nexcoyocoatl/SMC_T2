/**
 * libmemphis
 * @file monitor.h
 * 
 * @author Angelo Elias Dalzotto (angelo.dalzotto@edu.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date September 2021
 * 
 * @brief Monitoring API using the BrNoC
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MON_INTERVAL_QOS 50000

enum MONITOR_TYPE {
	MON_QOS,
	MON_SEC,
	MON_VOL,
	MON_MAX
};

/**
 * @brief Broadcast this task as a monitor
 * 
 * @param type Type of monitoring offered by this task
 */
void mon_announce(enum MONITOR_TYPE type);
