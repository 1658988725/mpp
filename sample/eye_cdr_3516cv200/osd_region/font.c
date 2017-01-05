#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "asc16.h"
#include "asc24.h" 
#include "asc32.h" 
//#include "hzk12.h"
//#include "hzk16.h"		 
//#include "hzk24.h" 
#include "font.h" 

#define MAX_ICHAR_INDEX  120+20

unsigned char ucBitMask[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};	 

extern unsigned char *g_pASC48FontLib;
extern unsigned char *g_pASC32FontLib;
extern unsigned char *g_pASC24FontLib;
extern unsigned char *g_pASC16FontLib;

void *MemMallocD( const unsigned int ulSize, const unsigned int ulLineNo, const char *strFileName)
{
    unsigned int LineNo ;
    const char *filename;
    filename = strFileName;
    filename = filename;
    LineNo= ulLineNo;
    LineNo= LineNo;
    return malloc(ulSize);  
}



void Memset16(unsigned short*uspTemp, unsigned short usValue, unsigned short usMem_Len)
{
	register unsigned short usTemp;

	for(usTemp = 0; usTemp < usMem_Len; usTemp++)
		*uspTemp++ = usValue;
}


int OSD_Rgb1555_8x16(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData)
{
	int nICharIndex;
	unsigned char *ucpCharData;
	unsigned short *uspDst;	
	int nStride = nIWidth;
	register int nXPos;
	register int nYPos;
	unsigned short *uspDot;
	unsigned char ucCodel,ucWidth;
	int nOffset,nMyFlag = 0;

	unsigned char*ucpDarkdot, ucDarkBuf[16*20]={1};
		
	unsigned char *ucpchar = (unsigned char *)chpStrData;

	if(NULL == chpStrData || NULL == uspPicData)/*没有显示的字符*/
	{
		return -1;
	}
	 
	uspDst = uspPicData;
	

	uspDst += (nStride * 2);
	uspDst += 2;
	nICharIndex = 0;
	
	//while(*ucpchar != '\0' && nICharIndex < 32)
    	while(*ucpchar != '\0' && nICharIndex < 64)
	{
	#if 0
		if( *ucpchar >= 0xa1 )/*中文字符*/
	  	{
			nMyFlag = 1;
              		ucCodel = *ucpchar++;/*获取区码*/
			ucCodeh = *ucpchar;/*获取位码*/
			if((ucCodeh < 0xa1) )/*无法显示的字符用?代替*/
			{
				ucCodel = 0xa3;
				ucCodeh = 0xbf;
			}
			nOffset = (94 * (ucCodel - 0xa1) + (ucCodeh -0xa1)) * 32;/*计算偏移量*/
			ucpCharData = ucHZK16x16 + nOffset;/*查表找出对应的值*/
			memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 22;

			for( nYPos = 0; nYPos < 16; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 10;
				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_LIGHT;
					}
					
					if (ucpCharData[nYPos * 2 + 1] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_LIGHT;
					}
					
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/
				ucpDarkdot += 16;
			}/*endfor( nYPos = 0; nYPos < 16; nYPos++ ) */
			ucWidth = 16;
		
		}/*end if( *ucpchar >= 0xa1 )*/
	  
		else /*英文字符*/
	#endif
		{
			nMyFlag = 0;
             #if(0)
			if((*ucpchar >= 0x20) && (*ucpchar <= 0x7F))
			{
				ucCodel = *ucpchar;
			}
			else/*无法显示的字符用?代替*/
			{
				ucCodel = 0x3F;
			}
              #endif
              
			ucCodel = *ucpchar;

#if(0)
             nOffset = ucCodel * 16;	
             ucpCharData = (unsigned char*)(&ucAscii16x8[nOffset]);
#else
             nOffset = (ucCodel-32) * 16; 
             ucpCharData = (unsigned char*)(&g_pASC16FontLib[nOffset]);	
#endif
			//memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 22;
			for( nYPos = 0; nYPos < 16; nYPos++ )
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 10;			
				for( nXPos = 0; nXPos < 8; nXPos ++ ) 
			  	{
		             if (ucpCharData[nYPos] & ucBitMask[nXPos])
					{					
						*(uspDot + nXPos) = OSD_DARK_2;//OSD_LIGHT_2;	
						ucpDarkdot[nXPos] = OSD_DARK;//OSD_LIGHT;
					}
			      }
			  	ucpDarkdot += 16;
		       }/*endfor( nYPos = 0; nYPos < 16; nYPos++ ) */
			ucWidth = 8;
		
		}/*end else*/
         #if(0)		
	       /*勾边*/
		ucpDarkdot = ucDarkBuf + 22;
		for( nYPos = 0; nYPos < 16; nYPos++ )
		{		
			uspDot = uspDst + nYPos * nStride + nICharIndex * 10;
			for( nXPos = 0; nXPos < ucWidth; nXPos ++ ) 
			{				
				if( ucpDarkdot[nXPos] !=  OSD_LIGHT ) 
				{
					continue;
				}
		
				/* 对字符前面加黑 */
				if ( *(ucpDarkdot + nXPos - 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos - 1) = OSD_DARK_2;
				}	

				/* 对字符后面加黑 */
				if ( *(ucpDarkdot + nXPos + 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos + 1) = OSD_DARK_2;
				}

				/* 对字符上面加黑 */
				if (*(ucpDarkdot + nXPos - 16) != OSD_LIGHT)
				{
					*(uspDot + nXPos - nStride) = OSD_DARK_2;
				}

				/* 字符下面加黑 */
				if ( *(ucpDarkdot + nXPos + 16) != OSD_LIGHT)
				{
					*(uspDot + nXPos + nStride) = OSD_DARK_2;				
				}
			}/*end for(nXPos = 0;nXPos < ucWidth;nXPos++)*/
			ucpDarkdot += 16;
		}/*endfor( nYPos = 0; nYPos < 16; nYPos++ ) */
         #endif
		if(nMyFlag)
		{
			nICharIndex += 2;
		}
		else
		{
			nICharIndex += 1;
		}
		ucpchar++;
			
	}/*end while(*ucpchar != '\0' && nICharIndex < 48)*/
	
	return 0;
}


int OSD_Rgb1555_16x24(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData)
{
	int nICharIndex;
	unsigned char *ucpCharData;
	unsigned short *uspDst;	
	int nStride = nIWidth;
	register int nXPos;
	register int nYPos;
	unsigned short *uspDot;
	unsigned char ucCodel,ucWidth;
	int nOffset,nMyFlag = 0;

	unsigned char*ucpDarkdot, ucDarkBuf[24*30]={1};
		
	unsigned char *ucpchar = (unsigned char *)chpStrData;

	 if(NULL == chpStrData || NULL == uspPicData)/*没有显示的字符*/
	{
		return -1;
	}
	
	uspDst = uspPicData;
	
	uspDst += (nStride * 2);
	uspDst += 2;
	nICharIndex = 0;
	//while(*ucpchar != '\0' && nICharIndex < 64)
     while(*ucpchar != '\0' && nICharIndex < MAX_ICHAR_INDEX)
	{
	#if 0
		if( *ucpchar >= 0xa1 )/*中文字符*/
	  	{
			nMyFlag = 1;
              		ucCodel = *ucpchar++;/*获取区码*/
			ucCodeh = *ucpchar;/*获取位码*/
			if((ucCodeh < 0xa1) ||(ucCodel > 0xf7))/*无法显示的字符用?代替*/
			{
				ucCodel = 0xa3;
				ucCodeh = 0xbf;
			}
			nOffset = (94 * (ucCodel - 0xa1) + (ucCodeh -0xa1)) * 3 *24;/*计算偏移量*/
			ucpCharData = ucHzk24x24 + nOffset;/*查表找出对应的值*/
			memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 26;

			for( nYPos = 0; nYPos < 24; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;

				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 3] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_LIGHT;
					}
					
					if (ucpCharData[nYPos * 3 + 1] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_LIGHT;
					}

					if (ucpCharData[nYPos * 3 + 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 16)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 16]=OSD_LIGHT;
					}
					
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/

				ucpDarkdot += 24;
			}/*endfor( nYPos = 0; nYPos < 24; nYPos++ ) */

			ucWidth = 24;
		
		}/*end if( *ucpchar >= 0xa1 )*/
	  
		else/*英文字符*/
	#endif
		{
             #if(0)
			nMyFlag = 0;
			if((*ucpchar >= 0x20) && (*ucpchar <= 0x7F))
			{
				ucCodel = *ucpchar;
			}
			else/*无法显示的字符用?代替*/
			{
				ucCodel = 0x3F;
			}
             #endif 
             ucCodel = *ucpchar;
             
             #if(0)
			nOffset = ucCodel * 48;	
			ucpCharData = (unsigned char*)(&ucAscii24x12[nOffset]);	
            #else
             nOffset = (ucCodel-32) * 48;
             //ucpCharData = (unsigned char*)(&g_pASC48FontLib[nOffset]);	
             ucpCharData = (unsigned char*)(&g_pASC24FontLib[nOffset]);	
            #endif
			//memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 26;
			for( nYPos = 0; nYPos < 24; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_DARK;//OSD_LIGHT;
					}

					if ((ucpCharData[nYPos * 2 + 1] &0xf0)& ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_DARK;//OSD_LIGHT;
					}
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/
				ucpDarkdot += 24;
			}/*endfor( nYPos = 0; nYPos < 24; nYPos++ ) */
			ucWidth = 16;
		
		}/*end else*/
			
#if 0		
	       /*勾边*/
		ucpDarkdot = ucDarkBuf + 26;
		for( nYPos = 0; nYPos < 24; nYPos++ )
		{		
			uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
			for( nXPos = 0; nXPos < ucWidth; nXPos ++ ) 
			{				
				if( ucpDarkdot[nXPos] !=  OSD_LIGHT ) 
				{
					continue;
				}
		
				/* 对字符前面加黑 */
				if ( *(ucpDarkdot + nXPos - 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos - 1) = OSD_DARK_2;
				}	

				/* 对字符后面加黑 */
				if ( *(ucpDarkdot + nXPos + 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos + 1) = OSD_DARK_2;
				}

				/* 对字符上面加黑 */
				if (*(ucpDarkdot + nXPos - 24) != OSD_LIGHT)
				{
					*(uspDot + nXPos - nStride) = OSD_DARK_2;
				}

				/* 字符下面加黑 */
				if ( *(ucpDarkdot + nXPos + 24) != OSD_LIGHT)
				{
					*(uspDot + nXPos + nStride) = OSD_DARK_2;				
				}
			}/*end for(nXPos = 0;nXPos < ucWidth;nXPos++)*/
			ucpDarkdot += 24;
		}/*end for( nYPos = 0; nYPos < 24; nYPos++ ) */
#endif
		if(nMyFlag)
		{
			nICharIndex += 3;
		}
		else
		{
			nICharIndex +=2;
		}
		ucpchar++;
			
	}/*end while(*ucpchar != '\0' && nICharIndex < 48)*/
	
	return 0;
}


int OSD_Rgb1555_16x32(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData)
{
	int nICharIndex;
	unsigned char *ucpCharData;
	unsigned short *uspDst;	
	int nStride = nIWidth;
	register int nXPos;
	register int nYPos;
	unsigned short *uspDot;
	unsigned char ucCodel,ucWidth;
	int nOffset,nMyFlag = 0;

	unsigned char*ucpDarkdot, ucDarkBuf[32*40]={1};
		
	unsigned char *ucpchar = (unsigned char *)chpStrData;

	 if(NULL == chpStrData || NULL == uspPicData)/*没有显示的字符*/
	{
		return -1;
	}
	
	uspDst = uspPicData;
	
	uspDst += (nStride * 2);
	uspDst += 2;
	nICharIndex = 0;
	//while(*ucpchar != '\0' && nICharIndex < 64)
    	while(*ucpchar != '\0' && nICharIndex < MAX_ICHAR_INDEX)
	{
	#if 0
		if( *ucpchar >= 0xa1 )/*中文字符*/
	  	{
			nMyFlag = 1;
              		ucCodel = *ucpchar++;/*获取区码*/
			ucCodeh = *ucpchar;/*获取位码*/
			if((ucCodeh < 0xa1) ||(ucCodel > 0xf7))/*无法显示的字符用?代替*/
			{
				ucCodel = 0xa3;
				ucCodeh = 0xbf;
			}
			nOffset = (94 * (ucCodel - 0xa1) + (ucCodeh -0xa1)) * 3 *24;/*计算偏移量*/
			ucpCharData = ucHzk24x24 + nOffset;/*查表找出对应的值*/
			memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 26;

			for( nYPos = 0; nYPos < 24; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;

				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 3] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_LIGHT;
					}
					
					if (ucpCharData[nYPos * 3 + 1] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_LIGHT;
					}

					if (ucpCharData[nYPos * 3 + 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 16)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 16]=OSD_LIGHT;
					}
					
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/

				ucpDarkdot += 24;
			}/*endfor( nYPos = 0; nYPos < 24; nYPos++ ) */

			ucWidth = 24;
		
		}/*end if( *ucpchar >= 0xa1 )*/
	  
		else/*英文字符*/
	#endif
		{
			nMyFlag = 0;
			if((*ucpchar >= 0x20) && (*ucpchar <= 0x7F))
			{
				ucCodel = *ucpchar;
			}
			else/*无法显示的字符用?代替*/
			{
				ucCodel = 0x3F;
			}
             #if(0)
			nOffset = ucCodel * 64;	
			ucpCharData = (unsigned char*)(&ucAscii32x16[nOffset]);	
            #else
             nOffset = (ucCodel-32) * 64;
             //ucpCharData = (unsigned char*)(&g_pASC48FontLib[nOffset]);	
             ucpCharData = (unsigned char*)(&g_pASC32FontLib[nOffset]);	
            #endif
			//memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 34;
			for( nYPos = 0; nYPos < 32; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_DARK;//OSD_LIGHT;
					}

					//if ((ucpCharData[nYPos * 2 + 1] &0xf0)& ucBitMask[nXPos])
					if ((ucpCharData[nYPos * 2 + 1] )& ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_DARK;//OSD_LIGHT;
					}
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/
				ucpDarkdot += 32;
			}/*endfor( nYPos = 0; nYPos < 32; nYPos++ ) */
			ucWidth = 16;
		
		}/*end else*/
			
#if 0		
	       /*勾边*/
		ucpDarkdot = ucDarkBuf + 34;
		for( nYPos = 0; nYPos < 32; nYPos++ )
		{		
			uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
			for( nXPos = 0; nXPos < ucWidth; nXPos ++ ) 
			{				
				if( ucpDarkdot[nXPos] !=  OSD_LIGHT ) 
				{
					continue;
				}
		
				/* 对字符前面加黑 */
				if ( *(ucpDarkdot + nXPos - 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos - 1) = OSD_DARK_2;
				}	

				/* 对字符后面加黑 */
				if ( *(ucpDarkdot + nXPos + 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos + 1) = OSD_DARK_2;
				}

				/* 对字符上面加黑 */
				if (*(ucpDarkdot + nXPos - 32) != OSD_LIGHT)
				{
					*(uspDot + nXPos - nStride) = OSD_DARK_2;
				}

				/* 字符下面加黑 */
				if ( *(ucpDarkdot + nXPos + 32) != OSD_LIGHT)
				{
					*(uspDot + nXPos + nStride) = OSD_DARK_2;				
				}
			}/*end for(nXPos = 0;nXPos < ucWidth;nXPos++)*/
			ucpDarkdot += 32;
		}/*end for( nYPos = 0; nYPos < 24; nYPos++ ) */
#endif
		if(nMyFlag)
		{
			nICharIndex +=  3;
		}
		else
		{
			nICharIndex += 3;// 2;
		}
		ucpchar++;
			
	}/*end while(*ucpchar != '\0' && nICharIndex < 48)*/
	
	return 0;
}


int OSD_Rgb1555_24x48(int nIWidth, int nIHeight, char *chpStrData, unsigned short* uspPicData)
{
	int nICharIndex;
	unsigned char *ucpCharData;
	unsigned short *uspDst;	
	int nStride = nIWidth;
	register int nXPos;
	register int nYPos;
	unsigned short *uspDot;
	unsigned char ucCodel,ucWidth;
	int nOffset,nMyFlag = 0;

	unsigned char*ucpDarkdot, ucDarkBuf[48*50];
		
      unsigned char *ucpchar = (unsigned char *)chpStrData;

	 if(NULL == chpStrData || NULL == uspPicData)/*没有显示的字符*/
	{
		return -1;
	}
	
	uspDst = uspPicData;
	
	uspDst += (nStride * 2);
	uspDst += 2;
	nICharIndex = 0;
	while(*ucpchar != '\0' && nICharIndex < 96)
	{
	#if 0
		if( *ucpchar >= 0xa1 )/*中文字符*/
	  	{
			nMyFlag = 1;
              		ucCodel = *ucpchar++;/*获取区码*/
			ucCodeh = *ucpchar;/*获取位码*/
			if((ucCodeh < 0xa1) ||(ucCodel > 0xf7))/*无法显示的字符用?代替*/
			{
				ucCodel = 0xa3;
				ucCodeh = 0xbf;
			}
			nOffset = (94 * (ucCodel - 0xa1) + (ucCodeh -0xa1)) * 3 *24;/*计算偏移量*/
			ucpCharData = ucHzk24x24 + nOffset;/*查表找出对应的值*/
			memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 26;

			for( nYPos = 0; nYPos < 24; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;

				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos * 3] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_LIGHT;
					}
					
					if (ucpCharData[nYPos * 3 + 1] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_LIGHT;
					}

					if (ucpCharData[nYPos * 3 + 2] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 16)=OSD_LIGHT_2;
						ucpDarkdot[nXPos + 16]=OSD_LIGHT;
					}
					
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/

				ucpDarkdot += 24;
			}/*endfor( nYPos = 0; nYPos < 24; nYPos++ ) */

			ucWidth = 24;
		
		}/*end if( *ucpchar >= 0xa1 )*/
	  
		else/*英文字符*/
	#endif
		{
			nMyFlag = 0;
			if((*ucpchar >= 0x20) && (*ucpchar <= 0x7F))
			{
				ucCodel = *ucpchar;
			}
			else/*无法显示的字符用?代替*/
			{
				ucCodel = 0x3F;
			}
			nOffset = (ucCodel-32) * 144;	//ucCodel * 96;	//-32 = -0x20
			ucpCharData = (unsigned char*)(&g_pASC48FontLib[nOffset]);	
			memset( ucDarkBuf, OSD_DARK, sizeof( ucDarkBuf )) ;	
			ucpDarkdot = ucDarkBuf + 50;
			for( nYPos = 0; nYPos < 48; nYPos++ ) 
			{
				uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
				for(nXPos = 0;nXPos < 8;nXPos++)
				{
					if (ucpCharData[nYPos *3] & ucBitMask[nXPos])
					{
						*(uspDot + nXPos)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos]=OSD_DARK;//OSD_LIGHT;
					}

					if ((ucpCharData[nYPos * 3 + 1] )& ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 8)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos + 8]=OSD_DARK;//OSD_LIGHT;
					}

					if ((ucpCharData[nYPos * 3 + 2] )& ucBitMask[nXPos])
					{
						*(uspDot + nXPos + 16)=OSD_DARK_2;//OSD_LIGHT_2;
						ucpDarkdot[nXPos + 16]=OSD_DARK;//OSD_LIGHT;
					}
				}/*end for(nXPos = 0;nXPos < 8;nXPos++)*/
				ucpDarkdot += 48;
			}/*endfor( nYPos = 0; nYPos < 32; nYPos++ ) */
			ucWidth = 24;
		
		}/*end else*/
			

	#if 0
	       /*勾边*/
		ucpDarkdot = ucDarkBuf + 50;
		for( nYPos = 0; nYPos < 48; nYPos++ )
		{		
			uspDot = uspDst + nYPos * nStride + nICharIndex * 7;
			for( nXPos = 0; nXPos < ucWidth; nXPos ++ ) 
			{				
				if( ucpDarkdot[nXPos] !=  OSD_LIGHT ) 
				{
					continue;
				}
		
				/* 对字符前面加黑 */
				if ( *(ucpDarkdot + nXPos - 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos - 1) = OSD_DARK_2;
				}	

				/* 对字符后面加黑 */
				if ( *(ucpDarkdot + nXPos + 1) != OSD_LIGHT)
				{
					*(uspDot + nXPos + 1) = OSD_DARK_2;
				}

				/* 对字符上面加黑 */
				if (*(ucpDarkdot + nXPos - 48) != OSD_LIGHT)
				{
					*(uspDot + nXPos - nStride) = OSD_DARK_2;
				}

				/* 字符下面加黑 */
				if ( *(ucpDarkdot + nXPos + 48) != OSD_LIGHT)
				{
					*(uspDot + nXPos + nStride) = OSD_DARK_2;				
				}
			}/*end for(nXPos = 0;nXPos < ucWidth;nXPos++)*/
			ucpDarkdot += 48;
		}/*end for( nYPos = 0; nYPos < 24; nYPos++ ) */
	#endif

		if(nMyFlag)
		{
			nICharIndex +=  3;
		}
		else
		{
			nICharIndex += 4;// 2;
		}
		ucpchar++;
			
	}/*end while(*ucpchar != '\0' && nICharIndex < 48)*/
	
	return 0;
}

