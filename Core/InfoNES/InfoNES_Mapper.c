/*===================================================================*/
/*                                                                   */
/*  InfoNES_Mapper.c : InfoNES Mapper Function                     */
/*                                                                   */
/*  2000/05/16  InfoNES Project ( based on NesterJ and pNesX )       */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_Mapper.h"
#include "K6502.h"

/*-------------------------------------------------------------------*/
/*  Mapper resources                                                 */
/*-------------------------------------------------------------------*/

/* Disk System RAM */
BYTE *DRAM = NULL;  /* Allocated in InfoNES_Init if mapper needs it */

/*-------------------------------------------------------------------*/
/*  Table of Mapper initialize function                              */
/*-------------------------------------------------------------------*/

struct MapperTable_tag MapperTable[] =
{
  {   0, Map0_Init   },
  /* Other mappers disabled in v1 to save flash/RAM.
   * Re-enable by uncommenting mapper includes above and
   * adding entries here. */
  {  -1, NULL }
};

/*-------------------------------------------------------------------*/
/*  body of Mapper functions                                         */
/*-------------------------------------------------------------------*/

/* v1: Only mapper 0 (NROM) — saves 382KB BSS. Add more mappers later. */
#include "mapper/InfoNES_Mapper_000.c"
#if 0  /* Disabled mappers — enable when implementing dynamic CHR/WRAM allocation */
#include "mapper/InfoNES_Mapper_001.c"
#include "mapper/InfoNES_Mapper_002.c"
#include "mapper/InfoNES_Mapper_003.c"
#include "mapper/InfoNES_Mapper_004.c"
#include "mapper/InfoNES_Mapper_005.c"
#include "mapper/InfoNES_Mapper_006.c"
#include "mapper/InfoNES_Mapper_007.c"
#include "mapper/InfoNES_Mapper_008.c"
#include "mapper/InfoNES_Mapper_009.c"
#include "mapper/InfoNES_Mapper_010.c"
#include "mapper/InfoNES_Mapper_011.c"
#include "mapper/InfoNES_Mapper_013.c"
#include "mapper/InfoNES_Mapper_015.c"
#include "mapper/InfoNES_Mapper_016.c"
#include "mapper/InfoNES_Mapper_017.c"
#include "mapper/InfoNES_Mapper_018.c"
#include "mapper/InfoNES_Mapper_019.c"
#include "mapper/InfoNES_Mapper_021.c"          
#include "mapper/InfoNES_Mapper_022.c"  
#include "mapper/InfoNES_Mapper_023.c"  
#include "mapper/InfoNES_Mapper_024.c"  
#include "mapper/InfoNES_Mapper_025.c"  
#include "mapper/InfoNES_Mapper_026.c"  
#include "mapper/InfoNES_Mapper_032.c"  
#include "mapper/InfoNES_Mapper_033.c" 
#include "mapper/InfoNES_Mapper_034.c" 
#include "mapper/InfoNES_Mapper_040.c"
#include "mapper/InfoNES_Mapper_041.c"
#include "mapper/InfoNES_Mapper_042.c"
#include "mapper/InfoNES_Mapper_043.c"
#include "mapper/InfoNES_Mapper_044.c"
#include "mapper/InfoNES_Mapper_045.c"
#include "mapper/InfoNES_Mapper_046.c"
#include "mapper/InfoNES_Mapper_047.c"
#include "mapper/InfoNES_Mapper_048.c"
#include "mapper/InfoNES_Mapper_049.c"
#include "mapper/InfoNES_Mapper_050.c"
#include "mapper/InfoNES_Mapper_051.c"
#include "mapper/InfoNES_Mapper_057.c"
#include "mapper/InfoNES_Mapper_058.c"
#include "mapper/InfoNES_Mapper_060.c"
#include "mapper/InfoNES_Mapper_061.c"
#include "mapper/InfoNES_Mapper_062.c"
#include "mapper/InfoNES_Mapper_064.c"
#include "mapper/InfoNES_Mapper_065.c"
#include "mapper/InfoNES_Mapper_066.c"
#include "mapper/InfoNES_Mapper_067.c"
#include "mapper/InfoNES_Mapper_068.c"
#include "mapper/InfoNES_Mapper_069.c"
#include "mapper/InfoNES_Mapper_070.c"
#include "mapper/InfoNES_Mapper_071.c"
#include "mapper/InfoNES_Mapper_072.c"
#include "mapper/InfoNES_Mapper_073.c"
#include "mapper/InfoNES_Mapper_074.c"
#include "mapper/InfoNES_Mapper_075.c"
#include "mapper/InfoNES_Mapper_076.c"
#include "mapper/InfoNES_Mapper_077.c"
#include "mapper/InfoNES_Mapper_078.c"
#include "mapper/InfoNES_Mapper_079.c"
#include "mapper/InfoNES_Mapper_080.c"
#include "mapper/InfoNES_Mapper_082.c"
#include "mapper/InfoNES_Mapper_083.c"
#include "mapper/InfoNES_Mapper_085.c"
#include "mapper/InfoNES_Mapper_086.c"
#include "mapper/InfoNES_Mapper_087.c"
#include "mapper/InfoNES_Mapper_088.c"
#include "mapper/InfoNES_Mapper_089.c"
#include "mapper/InfoNES_Mapper_090.c"
#include "mapper/InfoNES_Mapper_091.c"
#include "mapper/InfoNES_Mapper_092.c"
#include "mapper/InfoNES_Mapper_093.c"
#include "mapper/InfoNES_Mapper_094.c"
#include "mapper/InfoNES_Mapper_095.c"
#include "mapper/InfoNES_Mapper_096.c"
#include "mapper/InfoNES_Mapper_097.c"
#include "mapper/InfoNES_Mapper_099.c"
#include "mapper/InfoNES_Mapper_100.c"
#include "mapper/InfoNES_Mapper_101.c"
#include "mapper/InfoNES_Mapper_105.c"
#include "mapper/InfoNES_Mapper_107.c"
#include "mapper/InfoNES_Mapper_108.c"
#include "mapper/InfoNES_Mapper_109.c"
#include "mapper/InfoNES_Mapper_110.c"
#include "mapper/InfoNES_Mapper_112.c"
#include "mapper/InfoNES_Mapper_113.c"
#include "mapper/InfoNES_Mapper_114.c"
#include "mapper/InfoNES_Mapper_115.c"
#include "mapper/InfoNES_Mapper_116.c"
#include "mapper/InfoNES_Mapper_117.c"
#include "mapper/InfoNES_Mapper_118.c"
#include "mapper/InfoNES_Mapper_119.c"
#include "mapper/InfoNES_Mapper_122.c"
#include "mapper/InfoNES_Mapper_133.c"
#include "mapper/InfoNES_Mapper_134.c"
#include "mapper/InfoNES_Mapper_135.c"
#include "mapper/InfoNES_Mapper_140.c"
#include "mapper/InfoNES_Mapper_151.c"
#include "mapper/InfoNES_Mapper_160.c"
#include "mapper/InfoNES_Mapper_180.c"
#include "mapper/InfoNES_Mapper_181.c"
#include "mapper/InfoNES_Mapper_182.c"
#include "mapper/InfoNES_Mapper_183.c"
#include "mapper/InfoNES_Mapper_185.c"
#include "mapper/InfoNES_Mapper_187.c"
#include "mapper/InfoNES_Mapper_188.c"
#include "mapper/InfoNES_Mapper_189.c"
#include "mapper/InfoNES_Mapper_191.c"
#include "mapper/InfoNES_Mapper_193.c"
#include "mapper/InfoNES_Mapper_194.c"
#include "mapper/InfoNES_Mapper_200.c"
#include "mapper/InfoNES_Mapper_201.c"
#include "mapper/InfoNES_Mapper_202.c"
#include "mapper/InfoNES_Mapper_222.c"
#include "mapper/InfoNES_Mapper_225.c"
#include "mapper/InfoNES_Mapper_226.c"
#include "mapper/InfoNES_Mapper_227.c"
#include "mapper/InfoNES_Mapper_228.c"
#include "mapper/InfoNES_Mapper_229.c"
#include "mapper/InfoNES_Mapper_230.c"
#include "mapper/InfoNES_Mapper_231.c"
#include "mapper/InfoNES_Mapper_232.c"
#include "mapper/InfoNES_Mapper_233.c"
#include "mapper/InfoNES_Mapper_234.c"
#include "mapper/InfoNES_Mapper_235.c"
#include "mapper/InfoNES_Mapper_236.c"
#include "mapper/InfoNES_Mapper_240.c"
#include "mapper/InfoNES_Mapper_241.c"
#include "mapper/InfoNES_Mapper_242.c"
#include "mapper/InfoNES_Mapper_243.c"
#include "mapper/InfoNES_Mapper_244.c"
#include "mapper/InfoNES_Mapper_245.c"
#include "mapper/InfoNES_Mapper_246.c"
#include "mapper/InfoNES_Mapper_248.c"
#include "mapper/InfoNES_Mapper_249.c"
#include "mapper/InfoNES_Mapper_251.c"
#include "mapper/InfoNES_Mapper_252.c"
#include "mapper/InfoNES_Mapper_255.c"
#endif /* 0 — disabled mappers */

/* End of InfoNES_Mapper.c */
