/**
 * libmemphis
 * @file services.h
 *
 * @author Marcelo Ruaro (marcelo.ruaro@acad.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date June 2016
 *
 * @brief Kernel services definitions used to identify a packet
 * 
 * @details
 * There is a bug in debugger parser that cannot accept hex values with letters.
 * So only numbers are allowes, despite the hexadecimal notation.
 */

#pragma once

/* "Raw" messages 0x40-0x7F */
#define DATA_AV						0x40
#define MESSAGE_REQUEST				0x41
#define TASK_ALLOCATION				0x42
#define MESSAGE_DELIVERY			0x43
#define MONITOR                     0x44

#define MIGRATION_TEXT				0x50
#define MIGRATION_DATA  			0x51
#define MIGRATION_STACK				0x52
#define MIGRATION_HDSHK 			0x53
#define MIGRATION_PIPE				0x54
#define MIGRATION_TASK_LOCATION		0x55
#define MIGRATION_TCB				0x56

/* Messages encapsulated inside MESSAGE_DELIVERY 0x00-0x3F */
#define NEW_APP						0x00
#define APP_ALLOCATION_REQUEST		0x01
#define APP_MAPPING_COMPLETE		0x02
#define ABORT_TASK					0x03
#define TASK_ALLOCATED				0x04
#define TASK_RELEASE				0x05
#define TASK_TERMINATED				0x06
#define TASK_ABORTED				0x07
#define TASK_MIGRATED				0x08

#define REQUEST_FINISH				0x10
#define PE_HALTED					0x11
#define TERMINATE_ODA               0x12
#define REQUEST_NEAREST_SERVICE 	0x13
#define REQUEST_ALL_SERVICES		0x14
#define SERVICE_PROVIDER			0x15
#define ALL_SERVICE_PROVIDERS		0x16

#define QOS_MONITOR 				0x20
#define TASK_MIGRATION				0x21
#define TASK_MIGRATION_MAP			0x22
#define QOS_ANALYZE 				0x23
#define SEC_INFER                   0x24
#define SEC_SAFE_REQ_APP            0x25
#define SEC_SAFE_APP_RESP           0x26
#define SEC_SAFE_MAP_REQ            0x27
#define SEC_SAFE_MAP_RESP           0x28
#define SEC_MONITOR					0x29
#define VOL_MONITOR		            0x30

/* Broadcast messages 0x80-0x8F */
#define RELEASE_PERIPHERAL          0x80
#define APP_TERMINATED				0x81
#define HALT_PE						0x82
#define ANNOUNCE_MONITOR			0x83
