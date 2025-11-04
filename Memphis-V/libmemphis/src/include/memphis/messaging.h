/**
 * libmemphis
 * @file messaging.h
 *
 * @author Angelo Elias Dal Zotto (angelo.dalzotto@edu.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date June 2025
 * 
 * @brief Messaging definitions for management
 */

#pragma once

#include <stdint.h>

static const unsigned MEMPHIS_KERNEL_MSG = 0x10000000;
static const unsigned MEMPHIS_FORCE_PORT = 0x80000000;

static const int BR_SVC_ALL = 0;
static const int BR_SVC_TGT = 2;

typedef struct _memphis_info {
    /* {pad8/task_cnt, service, task/addr} */
    union
    {
        uint16_t task;
        uint16_t addr;
    };
    uint8_t  service;
    union
    {
        uint8_t  pad8;
        uint8_t  task_cnt;
        uint8_t  appid;
    };

    /* {task_cnt * sizeof(int) <- locations} for task release */
} memphis_info_t;

typedef struct _memphis_task_migration {
    /* {pad8, service, task} */
    uint16_t task;
    uint8_t  service;
    uint8_t  pad8;

    /* {pad16, address} */
    uint16_t address;
    uint16_t pad16;
} memphis_task_migration_t;

typedef struct _memphis_new_app {
    /* {task_cnt, service, pad16} */
    uint16_t pad16;
    uint8_t  service;
    uint8_t  task_cnt;

    uint32_t source;
    
    uint32_t hash;

    /* 2*task_cnt*sizeof(int32_t) descriptor */
    /* communication*sizeof(int32_t) */
} memphis_new_app_t;

typedef struct _memphis_qos_monitor {
    /* {pad8, service, task} */
    uint16_t task;
    uint8_t  service;
    uint8_t  pad8;

    uint32_t slack_time;

    uint32_t remaining_exec_time;
} memphis_qos_monitor_t;

typedef struct _memphis_sec_monitor {
    /* {app, service, task} */
    uint8_t  prod;
    uint8_t  cons;
    uint8_t  service;
    uint8_t  app;

    uint32_t timestamp;

    uint32_t latency;

    uint16_t hops;
    uint16_t size;    /* Theoretical max. 32 bits */
} memphis_sec_monitor_t;

typedef struct _memphis_vol_monitor {
	uint16_t pad16;
    uint8_t  service;
    uint8_t  pad8;

    uint16_t hops;
    uint16_t size;
} memphis_vol_monitor_t;
