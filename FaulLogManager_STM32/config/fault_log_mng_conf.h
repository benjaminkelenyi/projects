/**
 * @file fault_log_mng.h
 *
 * $Author: benjamin.kelenyi $
 *
 * $Rev:  $
 *
 * $Date: 15.07.2020 $
 *
 * @par Systeme : STM32
 *
 */
#ifndef CONFIG_H
#define CONFIG_H

/**************************************************************************************************
 * Includes
 *************************************************************************************************/
#include "fault_log_mng.h"

/**************************************************************************************************
 * Definitions
 *************************************************************************************************/
#define FIRST_FAULT_SECTOR           (uint8_t) SECTOR_3_BANK1
#define LAST_FAULT_SECTOR            (uint8_t) SECTOR_7_BANK2
#define CONTEXT_DATA_SIZE            20
#define EVENT_NUMBER_BY_FAULT        50
#define EVENT_NUMBER_IN_RAM          30
#define MAX_FAULT_RECORDING_IN_FLASH 50
#define CONTEXT_NB                   10
#define MAX_EVENT_IN_RAM             65535
#define FAULT_THEME                  0x1212
#define FAULT_NUMBER_BY_SECTOR       (uint32_t)50

#pragma pack(1)
/** \brief context configuration */
typedef struct _context_config_struct
{
   /** \brief context callback function type */
   void (*pcontextCbTypeDef)(uint8_t *buff, uint8_t contextSize);
   /** \brief context size*/
   uint8_t context_size;
} context_config_struct;

/* this function is used for test */
void test_callback(uint8_t *buff, uint8_t size);

static context_config_struct contextConfig[CONTEXT_NB] =
{
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8},
   {test_callback, 8}
};

#endif /* CONFIG_H */
