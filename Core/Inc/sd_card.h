/**
 * sd_card.h - microSD card driver (SPI2) with FatFs
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include "ff.h"       /* FatFs */
#include "diskio.h"   /* FatFs disk I/O */

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*  Initialization                                                   */
/*-------------------------------------------------------------------*/

/**
 * Initialize SPI2 for SD card, check card presence, mount FatFs.
 * @return 0 on success (FR_OK), error code on failure
 */
int SD_Init(void);

/**
 * Unmount and deinitialize SD card.
 */
void SD_DeInit(void);

/*-------------------------------------------------------------------*/
/*  File operations (convenience wrappers)                           */
/*-------------------------------------------------------------------*/

/**
 * Check if a file exists on the SD card.
 * @return 1 if exists, 0 otherwise
 */
int SD_FileExists(const char *path);

/**
 * Get the size of a file.
 * @return file size in bytes, or -1 on error
 */
long SD_FileSize(const char *path);

/*-------------------------------------------------------------------*/
/*  Power control                                                    */
/*-------------------------------------------------------------------*/

void SD_PowerOn(void);
void SD_PowerOff(void);

/**
 * Low-level disk I/O functions (called by FatFs internally)
 * Implemented in sd_card.c
 */
DSTATUS disk_initialize(BYTE pdrv);
DSTATUS disk_status(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);

#ifdef __cplusplus
}
#endif

#endif /* SD_CARD_H */
