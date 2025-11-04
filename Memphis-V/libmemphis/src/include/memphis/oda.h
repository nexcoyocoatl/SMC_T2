/**
 * libmemphis
 * @file oda.h
 *
 * @author Angelo Elias Dalzotto (angelo.dalzotto@edu.pucrs.br)
 * GAPH - Hardware Design Support Group (https://corfu.pucrs.br/)
 * PUCRS - Pontifical Catholic University of Rio Grande do Sul (http://pucrs.br/)
 * 
 * @date June 2021
 * 
 * @brief Standard function for OD(A) tasks
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ODA_USER	0x01
#define ODA_ACT		0x02
#define ODA_DECIDE	0x04
#define ODA_OBSERVE 0x08

#define O_QOS		0x0100
#define O_SEC       0x0200
#define O_VOL       0x0400

#define D_QOS		0x010000
#define D_SEC       0x020000

#define A_MIGRATION	0x01000000

#define SAFE_HASH_aes		  0x350AC39A
#define SAFE_HASH_audio_video 0x638E8D3B
#define SAFE_HASH_dijkstra    0x8C9F31AB
#define SAFE_HASH_dtw 	      0x15FD5B67
#define SAFE_HASH_management  0xD5DCC234
#define SAFE_HASH_mpeg 	      0x436351D5

typedef struct _oda {
	int id;
	int tag;
} oda_t;

typedef struct _oda_list {
	int tag;
	int cnt;
	int *ids;
} oda_list_t;

typedef struct _oda_provider
{
	/* {id, service, addr/pad16} */
	union
	{
		uint16_t addr;	/* REQUEST_NEAREST_SERVICE */
		uint16_t pad16;
	};
    uint8_t  service;
	union {
    	uint8_t  id;
		uint8_t  matches; /* ALL_SERVICE_PROVIDERS */
	};

	uint32_t tag;

	/* {int32_t * matches} <- ALL_SERVICE_PROVIDERS */
} oda_provider_t;

typedef struct _qos_analyze {
	/* {pad8, service, id} */
	uint16_t id;
    uint8_t  service;
	uint8_t  pad8;

	int32_t  rt_diff;
} qos_analyze_t;

/**
 * @brief Initializes a ODA
 * 
 * @param oda Pointer to the ODA
 */
void oda_init(oda_t *oda);

/**
 * @brief Initializes an ODA list
 * 
 * @param servers Pointer to the ODA list
 */
void oda_list_init(oda_list_t *servers);

/**
 * @brief Requests the nearest ODA service and handles SERVICE_PROVIDER
 * 
 * @param oda Pointer to the ODA
 * @param type_tag Flags of the desired ODA capabilities
 * 
 * @return
 * 	0 success
 *  1 request to terminate the app
 * -EINVAL received message != from SERVICE_PROVIDER
 * -ENODATA received message with different type_tag
 */
int oda_request_nearest_service(oda_t *oda, int type_tag);

/**
 * @brief Requests all servers of an ODA service and handles ALL_SERVICE_PROVIDERS
 * 
 * @param servers Pointer to the ODA list
 * @param type_tag Flags of the desired ODA capabilities
 * 
 * @return
 * 	0 success
 * 	1 request to terminate the app
 * -EINVAL received message != from ALL_SERVICE_PROVIDERS
 * -ENODATA received message with different type_tag
 * -ENOMEM could not allocate memory for the server IDs
 */
int oda_request_all_services(oda_list_t *servers, int type_tag);

/**
 * @brief Verifies if the ODA is enabled
 * 
 * @details A ODA is enabled if it has a valid ID
 * 
 * @param oda Pointer to the ODA
 * 
 * @return True if enabled
 */
bool oda_is_enabled(oda_t *oda);

/**
 * @brief Gets the ID of the ODA task
 * 
 * @param oda Pointer to the ODA structure
 * @return int ID of the task
 */
int oda_get_id(oda_t *oda);
