#ifndef _FONT_H
#define _FONT_H

#ifdef __cplusplus 
extern "C" { 
#endif 

unsigned short usRgb1555_24x48[34560];

#define OSD_BACKGROUNGHT	0x0000

//#define OSD_LIGHT 0xff
//#define OSD_DARK  0x00

//#define OSD_LIGHT_2 0xffff
//#define OSD_DARK_2  0x8000

#define OSD_LIGHT 0x00
#define OSD_DARK  0xff

#define OSD_LIGHT_2 0x8000
#define OSD_DARK_2  0xffff

void *MemMallocD( const unsigned int ulSize, const unsigned int ulLineNo, const char *strFileName);
#define MemMalloc(ulSize) MemMallocD((ulSize), __LINE__, __FILE__)    
void Memset16(unsigned short *uspTemp,  unsigned short usValue,  unsigned short usMem_Len);

int OSD_Rgb1555_8x16(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData);
int OSD_Rgb1555_16x24(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData);
int OSD_Rgb1555_16x32(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData);
int OSD_Rgb1555_24x48(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData);

#ifdef __cplusplus 
} 
#endif  

#endif 


