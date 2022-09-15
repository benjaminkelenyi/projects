/**
 * @file fault_log_mng.c
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
 */

/*
***************************************************************************************************
 * Includes
 **************************************************************************************************
 */
#include "fault_log_mng.h"

/*
***************************************************************************************************
 * Structure declarations
 **************************************************************************************************
 */
/* structure used to store the information in the RAM */
static event_buffer_struct s_event_buffer_struct;
/* fault_counter_cb: function returns the number of occurences for the given fault code */
static fault_counter_callback fault_counter_cb;
/* pointer to fault_log_struct - used to assign the flash address */
static fault_log_struct *p_fault_log_struct;

/**************************************************************************************************
 * Prototypes
 **************************************************************************************************
*/
static bool is_sector_found_by_status(e_sector_status sector_status, uint8_t *sector_found);
static uint8_t erase_next_sector(e_sector_nr e_current_fault_sector);
static uint8_t set_sector_status(e_sector_nr sector_number, e_sector_status sector_status);
static uint8_t set_fault_status(e_fault_status fault_status);
static uint8_t fault_log_mng_flash_full_handler(void);

static void *flash_sector_address[MAX_NB_SECTOR] =
{
    /* polyspace-begin MISRA2012:11.6 [Not a Defect:Low]
    "Cast shall be performed for address management" */

    /* Base address of the Flash sectors Bank 1 */
    (void *)ADDR_FLASH_SECTOR_0_BANK1, /* Base @ of Sector 0, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_1_BANK1, /* Base @ of Sector 1, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_2_BANK1, /* Base @ of Sector 2, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_3_BANK1, /* Base @ of Sector 3, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_4_BANK1, /* Base @ of Sector 4, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_5_BANK1, /* Base @ of Sector 5, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_6_BANK1, /* Base @ of Sector 6, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_7_BANK1, /* Base @ of Sector 7, 128 Kbytes */

    /* Base address of the Flash sectors Bank 2 */
    (void *)ADDR_FLASH_SECTOR_0_BANK2, /* Base @ of Sector 0, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_1_BANK2, /* Base @ of Sector 1, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_2_BANK2, /* Base @ of Sector 2, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_3_BANK2, /* Base @ of Sector 3, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_4_BANK2, /* Base @ of Sector 4, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_5_BANK2, /* Base @ of Sector 5, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_6_BANK2, /* Base @ of Sector 6, 128 Kbytes */
    (void *)ADDR_FLASH_SECTOR_7_BANK2  /* Base @ of Sector 7, 128 Kbytes */

    /* polyspace-end MISRA2012:11.6 [Not a Defect:Low] */
};

/*
***************************************************************************************************
 * Global variables
 **************************************************************************************************
 */
static uint32_t current_fault_index;
/* initiate the current sector with the first flash sector address */
static uint8_t current_fault_sector = FIRST_FAULT_SECTOR;
/* flag used to check if the ram buffer is full */
static bool buff_is_full = false;

/***************************** PUBLIC FUNCTION ***************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-16418 function goal
 * @wi.implements atess_4g/ATESS_4G-16419 function name
 * @wi.implements atess_4g/ATESS_4G-16420 function return value
 * @wi.implements atess_4g/ATESS_4G-16421 function parameters
 * @wi.implements atess_4g/ATESS_4G-16926 function behavior
 *
 * @param[in] callback: function returns the number of occurences for the given fault code
 *
 * @return init status
 */
uint8_t fault_log_mng_init(fault_counter_callback callback)
{
    /* check if the callback is provided */
    assert(callback != NULL);

    uint8_t ret = FLASH_OK;
    bool sector_found;

    /* register the fault counter callback, then use it during the background thread */
    fault_counter_cb = callback;

    /* search for WORKING sectors */
    sector_found = is_sector_found_by_status(WORKING_SECTOR, &current_fault_sector);

    /* if no working sectors in flash, search for the first empty sector */
    if (sector_found == true)
    {
        /* polyspace-begin MISRA2012:10.5 [Not a Defect:Low]
        "Cast shall be performed, as the function require a "e_sector_nr" type" */

        ret = erase_next_sector((e_sector_nr)current_fault_sector);

        /* polyspace-end MISRA2012:10.5 [Not a Defect:Low] */
    }
    else
    {
        /* start empty sector searching from the beginning of FLASH */
        current_fault_sector = FIRST_FAULT_SECTOR;

        /* if no empty sectors in flash, the flash is FULL */
        sector_found = is_sector_found_by_status(EMPTY_SECTOR, &current_fault_sector);

        if (sector_found == true)
        {
            /* polyspace-begin MISRA2012:10.5 [Not a Defect:Low]
            "Cast shall be performed, as the function require a "e_sector_nr" type" */

            /* change sector status to working */
            ret = set_sector_status((e_sector_nr)current_fault_sector, WORKING_SECTOR);

            if (ret == FLASH_OK)
            {
                /* erase next sector if is not empty */
                ret = erase_next_sector((e_sector_nr)current_fault_sector);
            }

            /* polyspace-end MISRA2012:10.5 [Not a Defect:Low] */
        }
        else
        {
            /* flash is full -> erase the first two sectors */
            ret = fault_log_mng_flash_full_handler();
        }
    }

    if (ret == FLASH_OK)
    {
        /* search for last log */
        while (
            (p_fault_log_struct->faults[current_fault_index].working_fault_status !=
             (uint16_t)FAULT_EMPTY) &&
            (p_fault_log_struct->faults[current_fault_index].full_fault_status !=
             (uint16_t)FAULT_EMPTY) &&
            (current_fault_index != (uint32_t)MAX_FAULT_RECORDING_IN_FLASH))
        {
            /* increment the fault log index */
            current_fault_index++;
        }
    }

    return ret;
}

/***************************** PUBLIC FUNCTION ***************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-17462 function goal
 * @wi.implements atess_4g/ATESS_4G-17463 function name
 * @wi.implements atess_4g/ATESS_4G-17478 function return value
 * @wi.implements atess_4g/ATESS_4G-17477 function parameters
 * @wi.implements atess_4g/ATESS_4G-17497 function behavior
 *
 * @param[in] theme_number
 * @param[in] code_number
 *
 * @return creation status
 */
uint8_t fault_log_mng_create_event(uint16_t theme_number, uint16_t code_number)
{
    uint32_t i = 0;
    static uint32_t current_ram_fault_index = 0, cursor = 0;
    uint8_t ret_val = (uint8_t)KO, *context_data = NULL;

    s_event_buffer_struct.recording_ram_fault_index = current_ram_fault_index;
    s_event_buffer_struct.events[current_ram_fault_index].theme = theme_number;
    s_event_buffer_struct.events[current_ram_fault_index].code = code_number;

    for (i = 0; i < CONTEXT_NB; i++)
    {
        /* get the context data from callbacks */
        contextConfig[i].pcontextCbTypeDef(context_data, contextConfig[i].context_size);

        /* copy the context to the context structure */
        memcpy(&s_event_buffer_struct.events[current_ram_fault_index].context.data[cursor],
               context_data, contextConfig[i].context_size);

        /* increment the cursor */
        /* the context size should be divided by 2,
        because in flash memory we write on each flash memory position 2 bytes */
        cursor += (uint32_t)(contextConfig[i].context_size / (uint32_t)2);
    }

    /* increment fault index */
    current_ram_fault_index++;

    /* if the RAM is full, overwrite the data -> ring buffer */
    if (current_ram_fault_index == MAX_EVENT_IN_RAM)
    {
        buff_is_full = true;
        current_ram_fault_index = 0;
        cursor = 0;
    }

    ret_val = (uint8_t)OK;

    return ret_val;
}

/***************************** PUBLIC FUNCTION ***************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-17501 function goal
 * @wi.implements atess_4g/ATESS_4G-17502 function name
 * @wi.implements atess_4g/ATESS_4G-17499 function return value
 * @wi.implements atess_4g/ATESS_4G-17500 function parameters
 * @wi.implements atess_4g/ATESS_4G-17498 function behavior
 *
 * @return clear status
 */
uint8_t fault_log_mng_clear_fault_log(void)
{
    uint8_t ret = FLASH_OK;

    /* erase the whole FALSH memory starting from first fault log sector */
    ret = Flash_Erase(FIRST_FAULT_SECTOR, LAST_FAULT_SECTOR);

    return ret;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-17505 function goal
 * @wi.implements atess_4g/ATESS_4G-17508 function name
 * @wi.implements atess_4g/ATESS_4G-17509 function return value
 * @wi.implements atess_4g/ATESS_4G-17506 function parameters
 * @wi.implements atess_4g/ATESS_4G-17507 function behavior
 *
 * @param[in] fault_status
 *
 * @return write status to flash
 */
static uint8_t set_fault_status(e_fault_status fault_status)
{
    uint8_t ret_val = FLASH_OK;

    /*buffer used to store the read data from flash*/
    uint8_t data_buffer[32];

    /* buffer to write back the data with the working sector flag modified */
    uint16_t write_buffer[16];

    static uint32_t addr = 0;

    uint16_t data_buff_size = sizeof(data_buffer);

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* read the first 32 bytes from fault[current_fault_index] */
    ret_val = Flash_Read(
        (uint32_t)&p_fault_log_struct->faults[current_fault_index],
        data_buffer, sizeof(data_buffer));

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    if (fault_status == WORKING_FAULT)
    {
        /* set fault status as working */
        p_fault_log_struct->faults[current_fault_index].working_fault_status = (uint16_t) true;

        /* set the working flag */
        data_buffer[0] = (uint8_t)p_fault_log_struct->faults[current_fault_index].working_fault_status;
    }
    else
    {
        /* set fault status as full */
        p_fault_log_struct->faults[current_fault_index].full_fault_status = (uint16_t) true;

        /* set the full flag */
        data_buffer[1] = (uint8_t)p_fault_log_struct->faults[current_fault_index].full_fault_status;
    }

    /* copy the modified buffer to the buffer witch will be written back to flash */
    memcpy(write_buffer, data_buffer, data_buff_size);

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* Start a program session */
    Flash_Program_Begin((uint32_t)p_fault_log_struct);

    addr = (uint32_t)p_fault_log_struct;

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    for (uint32_t j = (uint32_t)0; j <= NR_OF_FLASH_ADDR_INTERATION; j++)
    {
        /* increment for the next Flash word*/
        ret_val = Flash_Program(addr, &write_buffer[j]);

        addr = addr + BTYES_2_INCREMENT;
    }

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* End a flash program session -lock flash- */
    Flash_Program_End((uint32_t)p_fault_log_struct);

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-16982 function goal
 * @wi.implements atess_4g/ATESS_4G-16983 function name
 * @wi.implements atess_4g/ATESS_4G-16984 function return value
 * @wi.implements atess_4g/ATESS_4G-16985 function parameters
 * @wi.implements atess_4g/ATESS_4G-16986 function behavior
 *
 * @param[in] sector_number
 * @param[in] sector_status
 *
 * @return no return
 */
static uint8_t set_sector_status(e_sector_nr sector_number, e_sector_status sector_status)
{
    /*buffer used to store the read data from flash*/
    uint8_t data_buffer[32], ret_val = FLASH_OK;
    /* buffer to write back the data with the working sector flag modified */
    uint16_t write_buffer[16];

    uint16_t data_buff_size = sizeof(data_buffer);

    static uint32_t addr = 0;

    /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
    "Conversion should be performed from poiter to poiter object
    because the flash_adress contain the FLASH sector address */

    /* initiate the faut log struct with the "sector_number" address */
    p_fault_log_struct = (fault_log_struct *)flash_sector_address[sector_number];

    /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* read the first 32 bytes from the sector */
    ret_val = Flash_Read((uint32_t)p_fault_log_struct, data_buffer, sizeof(data_buffer));

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    if (sector_status == WORKING_SECTOR)
    {
        /* set the working flag */
        data_buffer[0] = 0x57;  /* W */
        data_buffer[1] = 0x4F;  /* O */
        data_buffer[2] = 0x52;  /* R */
        data_buffer[3] = 0x4B;  /* K */
    }
    else
    {
        /* set the full flag */
        data_buffer[4] = 0x46;  /* F */
        data_buffer[5] = 0x55;  /* U */
        data_buffer[6] = 0x4C;  /* L */
        data_buffer[7] = 0x4C;  /* L */
    }

    /* copy the modified buffer to the buffer witch will be written back to flash */
    memcpy(write_buffer, data_buffer, data_buff_size);

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* Start a program session */
    Flash_Program_Begin((uint32_t)p_fault_log_struct);

    addr = (uint32_t)p_fault_log_struct;

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    for (uint32_t j = (uint32_t)0; j <= NR_OF_FLASH_ADDR_INTERATION; j++)
    {
        /* increment for the next Flash word*/
        ret_val = Flash_Program(addr, &write_buffer[j]);

        /* increment the address with 2 bytes */
        addr = addr + BTYES_2_INCREMENT;
    }

    /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
    "Cast shall be performed for because the function is require an uint32_t
    and the pointer to fault log struct store the flash address" */

    /* End a flash program session -lock flash- */
    Flash_Program_End((uint32_t)p_fault_log_struct);

    /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21452 function goal
 * @wi.implements atess_4g/ATESS_4G-21450 function name
 * @wi.implements atess_4g/ATESS_4G-21451 function return value
 * @wi.implements atess_4g/ATESS_4G-21448 function parameters
 * @wi.implements atess_4g/ATESS_4G-21449 function behavior
 *
 * @param[in] sector_status
 * @param[in] sector_found
 *
 * @return return true if the sector status was found
 */
static bool is_sector_found_by_status(e_sector_status sector_status, uint8_t *sector_found)
{
    bool ret_sector_found = false;
    uint8_t *start_sector = sector_found;

    /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
    "Conversion should be performed from poiter to poiter object
    because the flash_adress contain the FLASH sector address */

    /* initiate the fault_log_struct with the start log sector */
    p_fault_log_struct = (fault_log_struct *)flash_sector_address[*start_sector];

    /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */

    /* search for the first empty sector */
    if (sector_status == EMPTY_SECTOR)
    {
        while ((uint8_t)*start_sector != (uint8_t)LAST_FAULT_SECTOR)
        {
            /* check for empty sector */
            if ((p_fault_log_struct->working_sector_status == EMPTY) &&
                (p_fault_log_struct->full_sector_status == EMPTY))
            {
                *sector_found = *start_sector;
                ret_sector_found = true;

                break;
            }
            else
            {
                /* increment sector address */
                *start_sector = *start_sector + 1;

                /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
                "Conversion should be performed from poiter to poiter object
                because the flash_adress contain the FLASH sector address */

                /* select the next sector address */
                p_fault_log_struct = (fault_log_struct *)flash_sector_address[*start_sector];

                /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */
            }
        }
    }
    else
    {
        /* search for working sector */
        while ((uint8_t)*start_sector != (uint8_t)LAST_FAULT_SECTOR)
        {
            /* check for working sector */
            if ((p_fault_log_struct->working_sector_status == WORKING) &&
                (p_fault_log_struct->full_sector_status != FULL))
            {
                *sector_found = *start_sector;
                ret_sector_found = true;

                break;
            }
            else
            {
                /* increment sector address */
                *start_sector = *start_sector + 1;

                /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
                "Conversion should be performed from poiter to poiter object
                because the flash_adress contain the FLASH sector address */

                /* select the next sector address */
                p_fault_log_struct = (fault_log_struct *)flash_sector_address[*start_sector];

                /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */
            }
        }
    }

    return ret_sector_found;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21503 function goal
 * @wi.implements atess_4g/ATESS_4G-21498 function name
 * @wi.implements atess_4g/ATESS_4G-21500 function return value
 * @wi.implements atess_4g/ATESS_4G-21496 function parameters
 * @wi.implements atess_4g/ATESS_4G-21497 function behavior
 *
 * @param[in] e_current_fault_sector
 *
 * @return flash erase status
 */
static uint8_t erase_next_sector(e_sector_nr e_current_fault_sector)
{
    /* structure used to check the next sector status -> EMPTY/FULL */
    static fault_log_struct *p_fault_log_struct_next;

    uint8_t ret_val = FLASH_OK;

    uint8_t start_addr, end_addr;
    start_addr = (uint8_t)e_current_fault_sector + 1;
    end_addr = (uint8_t)e_current_fault_sector + 1;

    /* if current sector is the last sector, then first sector will be erased */
    if ((uint8_t)e_current_fault_sector == LAST_FAULT_SECTOR)
    {
        start_addr = FIRST_FAULT_SECTOR;
        end_addr = FIRST_FAULT_SECTOR;
    }

    /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
    "Conversion should be performed from poiter to poiter object
    because the flash_adress contain the FLASH sector address */

    /* initiate the fault_log_struct with the next sector address */
    p_fault_log_struct_next = (fault_log_struct *)flash_sector_address[start_addr];

    /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */

    /* check if the next sector is empty, to not delete unnecessarily */
    if ((p_fault_log_struct_next->working_sector_status != EMPTY) &&
       (p_fault_log_struct_next->full_sector_status != EMPTY))
    {
        /* polyspace-begin MISRA2012:11.6 [Not a Defect:Low]
        "Cast shall be performed because the function is require an uint32_t
        and flash address is a void pointer -> to be able to assign the address
        to fault log structure " */
        ret_val = Flash_Erase(
            (uint32_t)flash_sector_address[start_addr],
            (uint32_t)flash_sector_address[end_addr + 1] - 1);

        /* polyspace-end MISRA2012:11.6 [Not a Defect:Low] */
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21494 function goal
 * @wi.implements atess_4g/ATESS_4G-21495 function name
 * @wi.implements atess_4g/ATESS_4G-21492 function return value
 * @wi.implements atess_4g/ATESS_4G-21493 function parameters
 * @wi.implements atess_4g/ATESS_4G-21489 function behavior
 *
 * @return return true if a new event was recorded
 */
static bool fault_log_mng_new_event_recorded(void)
{
    bool ret_val = false;

    /* if a new event was recorded in RAM */
    if (s_event_buffer_struct.recording_ram_fault_index !=
        s_event_buffer_struct.reading_ram_fault_index)
    {
        ret_val = true;
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21484 function goal
 * @wi.implements atess_4g/ATESS_4G-21486 function name
 * @wi.implements atess_4g/ATESS_4G-21480 function return value
 * @wi.implements atess_4g/ATESS_4G-21482 function parameters
 * @wi.implements atess_4g/ATESS_4G-21476 function behavior
 * @return no return
 */
static void fault_log_mng_read_new_event(void)
{
    /* check if the maximum events in the RAM is not reached */
    if (s_event_buffer_struct.reading_ram_fault_index == MAX_EVENT_IN_RAM)
    {
        /* reset the RAM fault index, as ring buffer */
        s_event_buffer_struct.reading_ram_fault_index = 0;
    }
    else
    {
        /* increment the fault reading index,
                until all the events are processed by the background task */
        s_event_buffer_struct.reading_ram_fault_index++;
    }
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21478 function goal
 * @wi.implements atess_4g/ATESS_4G-21473 function name
 * @wi.implements atess_4g/ATESS_4G-21475 function return value
 * @wi.implements atess_4g/ATESS_4G-21472 function parameters
 * @wi.implements atess_4g/ATESS_4G-21466 function behavior
 *
 * @param[in] index_events
 *
 * @return write status to flash
 */
static uint8_t fault_log_mng_write_events_to_flash(uint32_t index_events)
{
    uint8_t ret_val = FLASH_OK;

    /* check if the index event is lower than maximum events allowed in RAM */
    if (index_events < EVENT_NUMBER_IN_RAM)
    {
        /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
        "Cast shall be performed for because the function is require an uint32_t
        and the pointer to fault log struct store the flash address" */

        /* Start a program session */
        Flash_Program_Begin((uint32_t)p_fault_log_struct);

        /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

        uint16_t i = 0;
        uint8_t offset = 0, nr_of_32_bytes_in_buff = 0, nr_of_remaining_bytes = 0;
        uint16_t flash_buff[CONTEXT_DATA_SIZE +
                            sizeof(s_event_buffer_struct.events[index_events].code) +
                            sizeof(s_event_buffer_struct.events[index_events].theme)];

        /* copy the code to flash_buff */
        flash_buff[0] = s_event_buffer_struct.events[index_events].code;

        /* copy the theme to flash_buff */
        flash_buff[1] = s_event_buffer_struct.events[index_events].theme;

        /* copy the data to flash_buff */
        memcpy(&flash_buff[2], s_event_buffer_struct.events[index_events].context.data,
               sizeof(s_event_buffer_struct.events[index_events].context.data));

        /* calculate the number of 32 bytes in the flash_buff */
        nr_of_32_bytes_in_buff = (uint8_t)sizeof(flash_buff) / BYTES_32;

        /* calculate the remaining bytes from the flash_buff */
        nr_of_remaining_bytes = (uint8_t)sizeof(flash_buff) % BYTES_32;

        /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
        "Cast shall be performed for because the function is require an uint32_t
        and the pointer to fault log struct store the flash address" */

        /* write the number of 32 bytes to the flash */
        for (i = 0; i < nr_of_32_bytes_in_buff; i++)
        {
            ret_val = Flash_Program((uint32_t)&p_fault_log_struct->faults[current_fault_index] + offset,
                                    &flash_buff[offset / 2]);
            /* increment the address, to write to the next position */
            offset += BYTES_32;
        }

        if (ret_val == FLASH_OK)
        {
            /* write the remaining bytes */
            ret_val = Flash_Program((uint32_t)&p_fault_log_struct->faults[
                                    current_fault_index] + offset,
                                    &flash_buff[CONTEXT_DATA_SIZE - nr_of_remaining_bytes]);
        }

        /* End a flash program session -lock flash- */
        Flash_Program_End((uint32_t)p_fault_log_struct);

        /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21468 function goal
 * @wi.implements atess_4g/ATESS_4G-21462 function name
 * @wi.implements atess_4g/ATESS_4G-21464 function return value
 * @wi.implements atess_4g/ATESS_4G-21505 function parameters
 * @wi.implements atess_4g/ATESS_4G-21506 function behavior
 *
 * @return erase status
 */
static uint8_t fault_log_mng_flash_full_handler(void)
{
    uint8_t ret = FLASH_OK;

    /* flash is full -> set current fault sector to last flash sector */
    current_fault_sector = LAST_FAULT_SECTOR;

    /* polyspace-begin MISRA2012:10.5 [Not a Defect:Low]
    "Cast shall be performed, as the function require a "e_sector_nr" type" */

    /* clear next sector */
    ret = erase_next_sector((e_sector_nr)current_fault_sector);

    if (ret == FLASH_OK)
    {
        /* set current fault sector to first flash sector */
        current_fault_sector = FIRST_FAULT_SECTOR;
        /* erase next sector if is not empty */
        ret = erase_next_sector((e_sector_nr)current_fault_sector);

        if (ret == FLASH_OK)
        {
            /* set first flash sector as working sector */
            ret = set_sector_status((e_sector_nr)current_fault_sector, WORKING_SECTOR);
        }
    }
    /* polyspace-end MISRA2012:10.5 [Not a Defect:Low] */

    return ret;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21502 function goal
 * @wi.implements atess_4g/ATESS_4G-21504 function name
 * @wi.implements atess_4g/ATESS_4G-21499 function return value
 * @wi.implements atess_4g/ATESS_4G-21501 function parameters
 * @wi.implements atess_4g/ATESS_4G-21491 function behavior
 *
 * @return erase status
 */
static uint8_t fault_log_mng_max_fault_sector_handler(void)
{
    uint8_t ret_val = FLASH_OK;

    /* if the number of maximum faults from the sector is reached */
    if (current_fault_index == FAULT_NUMBER_BY_SECTOR)
    {
        /* reset fault index */
        current_fault_index = 0;

        /* polyspace-begin MISRA2012:10.5 [Not a Defect:Low]
        "Cast shall be performed, as the function require a "e_sector_nr" type" */

        /* set and write the "full" sector status to the flash */
        ret_val = set_sector_status((e_sector_nr)current_fault_sector, FULL_SECTOR);

        /* polyspace-end MISRA2012:10.5 [Not a Defect:Low] */

        if (ret_val == FLASH_OK)
        {
            /* flash is full */
            if (current_fault_sector == LAST_FAULT_SECTOR)
            {
                /* erase the first two sectors */
                ret_val = fault_log_mng_flash_full_handler();

                /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
                "Conversion should be performed from poiter to poiter object
                because the flash_adress contain the FLASH sector address */

                /* select first sector address */
                p_fault_log_struct = (fault_log_struct *)flash_sector_address[FIRST_FAULT_SECTOR];

                /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */
            }
            else
            {
                /* polyspace-begin MISRA2012:10.5 [Not a Defect:Low]
                "Cast shall be performed, as the function require a "e_sector_nr" type" */

                /* erase next sector */
                ret_val = erase_next_sector((e_sector_nr)current_fault_sector);

                /* polyspace-end MISRA2012:10.5 [Not a Defect:Low] */

                /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
                "Conversion should be performed from poiter to poiter object
                because the flash_adress contain the FLASH sector address */

                current_fault_sector++;
                p_fault_log_struct = (fault_log_struct *)flash_sector_address[current_fault_sector];

                /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */
            }
        }
    }
    else
    {
        /* select next fault log */
        current_fault_index = current_fault_index + 1;
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21488 function goal
 * @wi.implements atess_4g/ATESS_4G-21490 function name
 * @wi.implements atess_4g/ATESS_4G-21485 function return value
 * @wi.implements atess_4g/ATESS_4G-21487 function parameters
 * @wi.implements atess_4g/ATESS_4G-21481 function behavior
 *
 * @return write status to flash
 */
static uint8_t fault_log_mng_new_fault_received_handler(void)
{
    uint8_t ret_val = FLASH_ERR;
    uint32_t remaining_events = 0, i, index_event = 0;

    /* if new fault received */
    if (s_event_buffer_struct.
            events[s_event_buffer_struct.reading_ram_fault_index].theme == FAULT_THEME)
    {
        /* if before de reading_ram_fault_index doesn't exists 30 events */
        if (s_event_buffer_struct.reading_ram_fault_index < PREVIOUS_EVENTS)
        {
            /* calculate the number of events starting from index 0 to reading_ram_fault_index */
            remaining_events = PREVIOUS_EVENTS - s_event_buffer_struct.reading_ram_fault_index;

            /* if the buffer is full -> retrieve the events form the bottom of the ram buffer */
            if (buff_is_full)
            {
                /* calculate the starting index from the buttom of buffer */
                index_event = MAX_EVENT_IN_RAM - remaining_events;

                /* write the events from bottom of the buffer to the flash */
                for (i = 0; i < remaining_events; i++)
                {
                    ret_val = fault_log_mng_write_events_to_flash(index_event + i);
                }
            }

            /* reset index event */
            index_event = 0;
        }
        else
        {
            index_event = s_event_buffer_struct.reading_ram_fault_index - PREVIOUS_EVENTS;
        }

        /* write the events from the top of the buffer */
        for (i = 0; i < (PREVIOUS_EVENTS - remaining_events); i++)
        {
            ret_val = fault_log_mng_write_events_to_flash(index_event + i);
        }
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21483 function goal
 * @wi.implements atess_4g/ATESS_4G-21477 function name
 * @wi.implements atess_4g/ATESS_4G-21479 function return value
 * @wi.implements atess_4g/ATESS_4G-21474 function parameters
 * @wi.implements atess_4g/ATESS_4G-21470 function behavior
 *
 * @param[in] nr_of_last_events to store after the fault
 *
 * @return read/write status
 */
static uint8_t fault_log_mng_last_events_in_flash(uint8_t nr_of_last_events)
{
    static uint8_t last_events = 0, ret_val = FLASH_OK;

    /* read the events, until 20 events are recorded in the flash */
    last_events++;

    /* if 20 events are recorded to the flash */
    if (last_events == nr_of_last_events)
    {
        /* set fault status to full */
       ret_val = set_fault_status(FULL_FAULT);

        /* preparing for the next fault */
        current_fault_index++;

        /* reset the 20 events */
        last_events = 0;
    }

    return ret_val;
}

/***************************** PRIVATE FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21471 function goal
 * @wi.implements atess_4g/ATESS_4G-21467 function name
 * @wi.implements atess_4g/ATESS_4G-21469 function return value
 * @wi.implements atess_4g/ATESS_4G-21463 function parameters
 * @wi.implements atess_4g/ATESS_4G-21465 function behavior
 *
 * @param[in] event_buff array of event_struct
 *
 * @return read status from flash
 */
uint8_t fault_log_mng_read_faults_back(event_struct *event_buff)
{
    uint8_t i, j, ret_val = FLASH_ERR;
    uint8_t read_buff[2 * (CONTEXT_DATA_SIZE + SIZE_OF_THEME_CODE)];
    uint32_t from_index_read = 0, remaining_faults = 0, offset_index = 0;
    fault_log_struct *p_fault_log_struct_previous = NULL;
    /* index used to read back the events and to store the events in the array of struct */
    static uint8_t index = 0;

    /* check if in the current sector we have 50 faults recorded*/
    if (current_fault_index >= READ_FAULTS_NUMBER)
    {
        /* calculate the index from the faults will be read */
        from_index_read = current_fault_index - READ_FAULTS_NUMBER;
    }
    else
    {
        /* calculate the remaining faults from the previous sector */
        remaining_faults = READ_FAULTS_NUMBER - current_fault_index;
        from_index_read = MAX_FAULT_RECORDING_IN_FLASH - remaining_faults;

        /* polyspace-begin MISRA2012:11.5 [Not a Defect:Low]
        "Conversion should be performed from poiter to poiter object
        because the flash_adress contain the FLASH sector address */

        /* if the first sector is selected and there is no 50 events, go back to last sector */
        if (current_fault_sector == FIRST_FAULT_SECTOR)
        {
            p_fault_log_struct_previous = (fault_log_struct *)flash_sector_address[LAST_FAULT_SECTOR];
        }
        else
        {
            p_fault_log_struct_previous =
                (fault_log_struct *)flash_sector_address[current_fault_sector - 1];
        }

        /* polyspace-end MISRA2012:11.5 [Not a Defect:Low] */
    }

    /* read back 50 faults => 50 faults = 50 * 50 events */
    for (i = 0; i < READ_FAULTS_NUMBER; i++)
    {
        for (j = 0; j < EVENT_NUMBER_BY_FAULT; j++)
        {
            if (remaining_faults == 0)
            {
                /* polyspace-begin MISRA2012:11.4 [Not a Defect:Low]
                "Cast shall be performed for because the function is require an uint32_t
                and the pointer to fault log struct store the flash address" */

                ret_val = Flash_Read(
                    (uint32_t)&p_fault_log_struct->faults[from_index_read + i - offset_index].events[j],
                    read_buff,
                    sizeof(read_buff));
            }
            else
            {
                ret_val = Flash_Read(
                    (uint32_t)&p_fault_log_struct_previous->faults[from_index_read + i].events[j],
                    read_buff,
                    sizeof(read_buff));

                /* polyspace-end MISRA2012:11.4 [Not a Defect:Low] */

                if (remaining_faults == i)
                {
                    remaining_faults = 0;
                    from_index_read = 0;
                    offset_index = i;
                }
            }

            if (ret_val != FLASH_OK)
            {
                break;
            }
            else
            {
                /* copy the data to the event struct array */
                event_buff[index].theme = (uint16_t)(((uint16_t)read_buff[0] << (uint16_t)8U) |
                                                     (uint16_t)read_buff[1]);
                event_buff[index].code = (uint16_t)(((uint16_t)read_buff[2] << (uint16_t)8U) |
                                                    (uint16_t)read_buff[3]);
                memcpy(&event_buff[index].context.data, &read_buff[4], 2 * CONTEXT_DATA_SIZE);

                /* increment index, to be able to store the next event */
                index++;
            }
        }
    }

    return ret_val;
}

static uint8_t fault_record(void)
{
    uint8_t ret_val = FLASH_OK;

    /* if a fault recording is already is in progress */
    if ((p_fault_log_struct->faults[current_fault_index].working_fault_status == FAULT_WORKING) &&
        (p_fault_log_struct->faults[current_fault_index].full_fault_status != FAULT_FULL))
    {
        /* write the next 20 events in flash */
        ret_val = fault_log_mng_write_events_to_flash(
            s_event_buffer_struct.reading_ram_fault_index);

        if (ret_val == FLASH_OK)
        {
            /* handler to check if 20 events was written to flash */
            ret_val = fault_log_mng_last_events_in_flash(LAST_EVENTS);
        }

        if (ret_val == FLASH_OK)
        {
            /* maximum faults from the sector is reached */
            ret_val = fault_log_mng_max_fault_sector_handler();
        }
    }
    else
    {
        /* check if the fault recording si allowed by max fault recording */
        if (fault_counter_cb(FAULT_THEME) <= MAX_FAULT_RECORDING_IN_FLASH)
        {
            /* if a new fault is received, store the 30 previous events in FLASH */
            ret_val = fault_log_mng_new_fault_received_handler();

            if (ret_val == FLASH_OK)
            {
                /* set fault writing in progress */
                ret_val = set_fault_status(WORKING_FAULT);
            }
        }
    }

    return ret_val;
}

/***************************** PUBLIC FUNCTION **************************************************/
/**
 * @wi.implements atess_4g/ATESS_4G-21513 function goal
 * @wi.implements atess_4g/ATESS_4G-21514 function name
 * @wi.implements atess_4g/ATESS_4G-21511 function return value
 * @wi.implements atess_4g/ATESS_4G-21512 function parameters
 * @wi.implements atess_4g/ATESS_4G-21510 function behavior
 *
 * @return read/write/clear status from flash
 */
uint8_t fault_log_mng_background_task(void)
{
    uint8_t ret_val = FLASH_OK;

    /* if clear next FLASH sector is finished */
    if (Flash_Get_Status(FLASH_TIMEOUT, (uint32_t)1) != FLASH_ERR)
    {
        /* if a new event was recorded in RAM */
        if (fault_log_mng_new_event_recorded())
        {
            /* reading new event in RAM */
            fault_log_mng_read_new_event();

            ret_val = fault_record();
        }
    }

    return ret_val;
}
