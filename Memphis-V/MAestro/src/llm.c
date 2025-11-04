/**
 * MAestro
 * @file llm.c
 *
 * @author Angelo Elias Dalzotto (angelo.dalzotto@edu.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date March 2020
 * 
 * @brief Implements the Low-Level Monitor for Management Application support.
 */

#include <llm.h>

#include <stdlib.h>

#include <mmr.h>
#include <interrupts.h>
#include <broadcast.h>
#include <kernel_pipe.h>
#include <mpipe.h>

#include <memphis.h>
#include <memphis/monitor.h>
#include <memphis/services.h>
#include <memphis/messaging.h>

observer_t _observers[MON_MAX];

void llm_init()
{
	for(int i = 0; i < MON_MAX; i++)
		_observers[i].addr = -1;
}

void llm_set_observer(enum MONITOR_TYPE type, int task, int addr)
{
	int pe_addr = MMR_DMNI_INF_ADDRESS;
	uint8_t pe_x = pe_addr >> 8;
	uint8_t pe_y = pe_addr & 0xFF;
	uint8_t obs_x = addr >> 8;
	uint8_t obs_y = addr & 0xFF;
	uint16_t dist = abs(pe_x - obs_x) + abs(pe_y - obs_y);

	if(_observers[type].addr == -1 || _observers[type].dist > dist){
		_observers[type].task = task;
		_observers[type].addr = addr;
		_observers[type].dist = dist;
	}
}

bool llm_has_monitor(int mon_id)
{
	return (_observers[mon_id].addr != -1);
}

void llm_rt(unsigned *last_monitored, int id, unsigned slack_time, unsigned remaining_exec_time)
{
	unsigned now = MMR_RTC_MTIME;

	if (now - (*last_monitored) < MON_INTERVAL_QOS)
		return;

	memphis_qos_monitor_t monitor;
	monitor.task                = id;
	monitor.service             = QOS_MONITOR;
	monitor.slack_time          = slack_time;
	monitor.remaining_exec_time = remaining_exec_time;
	mpipe_write(&monitor, sizeof(memphis_qos_monitor_t), _observers[MON_QOS].addr);
	*last_monitored = now;
}

void llm_sec(unsigned timestamp, unsigned size, int src, int dst, int prod, int cons, unsigned now)
{
	const unsigned src_x = (src >> 8) & 0xFF;
	const unsigned src_y = (src & 0xFF);
	const unsigned dst_x = (dst >> 8) & 0xFF;
	const unsigned dst_y = (dst & 0xFF);

	memphis_sec_monitor_t monitor;
	monitor.prod      = prod;
	monitor.cons      = cons;
	monitor.service   = SEC_MONITOR;
	monitor.app       = (prod >> 8) & 0xFF;
	monitor.timestamp = timestamp;
	monitor.latency   = (now - timestamp);
	monitor.hops      = abs(src_x - dst_x) + abs(src_y - dst_y);
	monitor.size      = size;

	mpipe_write(&monitor, sizeof(memphis_sec_monitor_t), _observers[MON_SEC].addr);
}

void llm_vol(unsigned size, int src, int dst)
{
	const unsigned src_x = (src >> 8) & 0xFF;
	const unsigned src_y = (src & 0xFF);
	const unsigned dst_x = (dst >> 8) & 0xFF;
	const unsigned dst_y = (dst & 0xFF);

	memphis_vol_monitor_t monitor;

	monitor.service   = VOL_MONITOR;
	monitor.hops      = abs(src_x - dst_x) + abs(src_y - dst_y);
	monitor.size      = size;

	mpipe_write(&monitor, sizeof(memphis_vol_monitor_t), _observers[MON_VOL].addr);
}
