/**
 * @file fault_log_mng.h
 *
 * $Author: benjamin.kelenyi $
 *
 * $Rev:  $
 *
 * $Date: 14.07.2020 $
 *
 * @par Systeme : STM32
 *
 * @par Langage : C
 *
 * @mainpage fault log management
 *
 * @section INTRODUCTION
 * This component will allow to manage the fault logs
 *
 *
 * @section COPYRIGHT
 * Copyright 2020 Wabtec Corp.
 */
#ifndef FAULT_LOG_MNG_H
#define FAULT_LOG_MNG_H
/**************************************************************************************************
 * Includes
 *************************************************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"

#include "assert.h"
#include "fault_log_mng_conf.h"
#include "flash.h"
#include "flash_h7.h"

/**************************************************************************************************
 * Definitions
**************************************************************************************************/
/* maximum number of sectors in flash */
#define MAX_NB_SECTOR                 16
/* number of previous events */
#define PREVIOUS_EVENTS               30
/* number of lsat events */
#define LAST_EVENTS                   20
/* empty macro used to check the EMPTY status for sectors */
#define EMPTY                         0xFFFFFFFF
/* empty macro used to check the WORKING status for sectors */
#define WORKING                       0x4B524F57  /* WORK */
/* empty macro used to check the FULL status for sectors  */
#define FULL                          0x4C4C5546  /* FULL */
/* used to read the 50 last faults record in FLASH memory*/
#define READ_FAULTS_NUMBER            25
/* define flash timeout*/
#define FLASH_TIMEOUT 10000U
/* sum of the size in bytes for theme and code from the event_struct */
#define SIZE_OF_THEME_CODE            2
/* macro used to calculate the number of 32 bytes in the flash_buff */
#define BYTES_32                      32
/* increment the flash address with 2 bytes */
#define BTYES_2_INCREMENT             (uint32_t)2
/* macro used interate the flash address to write 32 bytes */
#define NR_OF_FLASH_ADDR_INTERATION   15
/* macro used to check the EMPTY status for faults */
#define FAULT_EMPTY                   (uint16_t)0xFFFF
/*  macro used to check the WORKING status for faults */
#define FAULT_WORKING                 (uint16_t)0xFF01
/* macro used to check the FULL status for  faults */
#define FAULT_FULL                    (uint16_t)0x01FF

/**************************************************************************************************
 * Enums
**************************************************************************************************/
enum e_return
{
    OK = 0,
    KO = 1
};

typedef enum
{
    WORKING_FAULT = 0,
    FULL_FAULT = 1
}e_fault_status;

typedef enum
{
    EMPTY_SECTOR = 0,
    WORKING_SECTOR = 1,
    FULL_SECTOR = 2
} e_sector_status;

typedef enum
{
    SECTOR_0_BANK1 = 0,
    SECTOR_1_BANK1,
    SECTOR_2_BANK1,
    SECTOR_3_BANK1,
    SECTOR_4_BANK1,
    SECTOR_5_BANK1,
    SECTOR_6_BANK1,
    SECTOR_7_BANK1,
    SECTOR_0_BANK2,
    SECTOR_1_BANK2,
    SECTOR_2_BANK2,
    SECTOR_3_BANK2,
    SECTOR_4_BANK2,
    SECTOR_5_BANK2,
    SECTOR_6_BANK2,
    SECTOR_7_BANK2,
} e_sector_nr;

/**************************************************************************************************
 * Callback definitions
**************************************************************************************************/
typedef uint16_t (*fault_counter_callback)(uint16_t);

/**************************************************************************************************
 * Structures
**************************************************************************************************/
/** \brief _context struct */
#pragma pack(1)

typedef struct _context_struct
{
    /** \brief context data */
    uint16_t data[CONTEXT_DATA_SIZE];
} context_struct;

/** \brief event struct (theme, code, context) */
typedef struct _event_struct
{
    /** \brief event theme */
    uint16_t theme;
    /** \brief event code*/
    uint16_t code;
    /** \brief event context */
    context_struct context;
} event_struct;

/** \brief fault struct */
typedef struct _fault_struct
{
    /** \brief working fault status*/
    uint16_t working_fault_status;
    /** \brief full fault status*/
    uint16_t full_fault_status;
    /** \brief fault code */
    uint16_t fault_code;
    /** \brief fault events */
    event_struct events[EVENT_NUMBER_BY_FAULT];
} fault_struct;

/** \brief fault log struct */
typedef struct _fault_log_struct
{
    /** \brief working sector status*/
    uint32_t working_sector_status;
    /** \brief full sector status*/
    uint32_t full_sector_status;
    /** \brief fault log*/
    fault_struct faults[FAULT_NUMBER_BY_SECTOR];
} fault_log_struct;

/** \brief event buffer struct */
typedef struct _event_buffer_struct
{
    /** \brief current event index */
    uint32_t recording_ram_fault_index;
    /** \brief current event index */
    uint32_t reading_ram_fault_index;
    /** \brief fault events */
    event_struct events[EVENT_NUMBER_IN_RAM];
} event_buffer_struct;

/**************************************************************************************************
 * API
**************************************************************************************************/
/** \brief context fault_log_mng_init */
uint8_t fault_log_mng_init(fault_counter_callback callback);

/** \brief context fault_log_mng_create_event */
uint8_t fault_log_mng_create_event(uint16_t theme_number, uint16_t code_number);

/** \brief context fault_log_mng_clear_fault_log */
uint8_t fault_log_mng_clear_fault_log(void);

/** \brief read 50 last faults recorded in FLASH memory */
uint8_t fault_log_mng_read_faults_back(event_struct *event_buff);

/** \brief background task function for fault log manager */
uint8_t fault_log_mng_background_task(void);

#endif /* FAULT_LOG_MNG_H */
