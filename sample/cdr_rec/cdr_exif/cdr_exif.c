/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>

#include "cdr_gps_data_analyze.h"

#include <sys/vfs.h>


#define FILE_NAME "write-exif.jpg"

#define FILE_BYTE_ORDER EXIF_BYTE_ORDER_INTEL

static const unsigned char exif_header[] = {
  0xff, 0xd8, 0xff, 0xe1,
};

/* length of data in exif_header */
static const unsigned int exif_header_len = sizeof(exif_header);


/* length of data in image_jpg */
static const unsigned int image_jpg_len ;//= sizeof(image_jpg);

/* start of JPEG image data section */
static const unsigned int image_data_offset = 20;
#define image_data_len (image_jpg_len - image_data_offset)




const char *pucMakerNote = "YA_CDR_EYE";//品牌信息

/* Get an existing tag, or create one if it doesn't exist */
static ExifEntry *init_tag(ExifData *exif, ExifIfd ifd, ExifTag tag)
{
	ExifEntry *entry;
	/* Return an existing tag if one exists */
	if (!((entry = exif_content_get_entry (exif->ifd[ifd], tag)))) {
	    /* Allocate a new entry */
	    entry = exif_entry_new ();
	    assert(entry != NULL); /* catch an out of memory condition */
	    entry->tag = tag; /* tag must be set before calling  exif_content_add_entry */

	    /* Attach the ExifEntry to an IFD */
	    exif_content_add_entry (exif->ifd[ifd], entry);

	    /* Allocate memory for the entry and fill with default data */
	    exif_entry_initialize (entry, tag);

	    /* Ownership of the ExifEntry has now been passed to the IFD.
	     * One must be very careful in accessing a structure after
	     * unref'ing it; in this case, we know "entry" won't be freed
	     * because the reference count was bumped when it was added to
	     * the IFD.
	     */
	    exif_entry_unref(entry);
	}
	return entry;
}

/* Create a brand-new tag with a data field of the given length, in the
 * given IFD. This is needed when exif_entry_initialize() isn't able to create
 * this type of tag itself, or the default data length it creates isn't the
 * correct length.
 */
static ExifEntry *create_tag(ExifData *exif, ExifIfd ifd, ExifTag tag, size_t len)
{
	void *buf;
	ExifEntry *entry;
	
	/* Create a memory allocator to manage this ExifEntry */
	ExifMem *mem = exif_mem_new_default();
	assert(mem != NULL); /* catch an out of memory condition */

	/* Create a new ExifEntry using our allocator */
	entry = exif_entry_new_mem (mem);
	assert(entry != NULL);

	/* Allocate memory to use for holding the tag data */
	buf = exif_mem_alloc(mem, len);
	assert(buf != NULL);

	/* Fill in the entry */
	entry->data = buf;
	entry->size = len;
	entry->tag = tag;
	entry->components = len;
	entry->format = EXIF_FORMAT_UNDEFINED;

	/* Attach the ExifEntry to an IFD */
	exif_content_add_entry (exif->ifd[ifd], entry);

	/* The ExifMem and ExifEntry are now owned elsewhere */
	exif_mem_unref(mem);
	exif_entry_unref(entry);

	return entry;
}

int cdr_write_exif_to_jpg(char *pSrcFileName, GPS_INFO sGpsInfo)
{
	int rc = 1;
	char versionid[5]={'1','2','3','4',0};
	FILE *f = NULL;
    
	unsigned char *exif_data;
	unsigned int exif_data_len;

    //FILE *f2 = fopen("19700101095721.jpg", "r");//write exif to this file pSrcFileName
    FILE *f2 = fopen(pSrcFileName, "r");//write exif to this file pSrcFileName
	if (!f2) {
		fprintf(stderr, "Error creating file \n");
		//exif_data_unref(exif);
		return -1;
	}
    fseek (f2, 0, SEEK_END);   ///将文件指针移动文件结尾
    int iFileSize = ftell (f2); ///求出当前文件指针距离文件开始的字节数
    fseek (f2, 0, SEEK_SET); 
    
    //char buf[1024*1024] = {0};
    char buf[1920*1080] = {0};

    int i;
    int pos;
    char temp;

    for(i=0; i<iFileSize-1; i++)  
    {  
        temp = fgetc(f2);  
        if(EOF == temp) break;  
        buf[pos++] = temp;  
    }  
    buf[pos] = 0; 
    fclose(f2);
    
	ExifEntry *entry;
    
	ExifData *exif = exif_data_new();
	if (!exif) {
		fprintf(stderr, "Out of memory\n");
		return 2;
	}
    
	/* Set the image options */
	exif_data_set_option(exif, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
	exif_data_set_data_type(exif, EXIF_DATA_TYPE_COMPRESSED);
	exif_data_set_byte_order(exif, FILE_BYTE_ORDER);

	/* Create the mandatory EXIF fields with default data */
	exif_data_fix(exif);

	entry = create_tag(exif, EXIF_IFD_GPS, 0x0000,4);    
	/* Write the special header needed for a comment tag */    
	entry->format = EXIF_FORMAT_BYTE;
	entry->components = 4;
	exif_set_sshort(entry->data, FILE_BYTE_ORDER, versionid[0]);
	exif_set_sshort(entry->data+1, FILE_BYTE_ORDER, versionid[1]);
	exif_set_sshort(entry->data+2, FILE_BYTE_ORDER, versionid[2]);
	exif_set_sshort(entry->data+3, FILE_BYTE_ORDER, versionid[3]);

	entry = create_tag(exif, EXIF_IFD_GPS, 0x0001,2);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = 2;
	memcpy(entry->data, "N", 2);
	ExifSRational a;
	ExifSRational b;
	ExifSRational c;
	//ExifSRational d;
    
	//a.numerator = sGpsInfo.latitude;//纬度
	a.numerator = sGpsInfo.latitude_Degree;//纬度
	a.denominator = 1;
    b.numerator = sGpsInfo.latitude_Cent;
	b.denominator = 1;
    c.numerator = sGpsInfo.latitude_Second;
	c.denominator = 1;//100;
	entry = create_tag(exif, EXIF_IFD_GPS, 0x0002,24);
	entry->format = EXIF_FORMAT_SRATIONAL;
	entry->components = 3;
	exif_set_srational(entry->data,FILE_BYTE_ORDER,a);
	exif_set_srational(entry->data+8,FILE_BYTE_ORDER,b);
	exif_set_srational(entry->data+16,FILE_BYTE_ORDER,c);

	entry = create_tag(exif, EXIF_IFD_GPS, 0x0003,2);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = 2;
	memcpy(entry->data, "E", 2);

    a.numerator = sGpsInfo.longitude_Degree;//sGpsInfo.longitude;//经度
	a.denominator = 1;
	b.numerator = sGpsInfo.longitude_Cent;//55;
	b.denominator = 1;
	c.numerator = sGpsInfo.longitude_Second;//5512;
	c.denominator = 1;//100;  
	entry = create_tag(exif, EXIF_IFD_GPS, 0x0004,24);
	entry->format = EXIF_FORMAT_SRATIONAL;
	entry->components = 3;
	exif_set_srational(entry->data,FILE_BYTE_ORDER,a);
	exif_set_srational(entry->data+8,FILE_BYTE_ORDER,b);
	exif_set_srational(entry->data+16,FILE_BYTE_ORDER,c);
    	
#if(0)
    /*gps time*/
	entry = create_tag(exif, EXIF_IFD_GPS, 0x0007,24);
	entry->format = EXIF_FORMAT_RATIONAL;
	entry->components = 3;
	ExifRational x;
	ExifRational y;
	ExifRational z;
	x.denominator = 1;
    x.numerator = 12;
	y.denominator = 2;
	y.numerator = 3;
	exif_set_rational(entry->data,FILE_BYTE_ORDER,x);
	exif_set_rational(entry->data+8,FILE_BYTE_ORDER,y);
	exif_set_rational(entry->data+16,FILE_BYTE_ORDER,z);
#endif

#if(1)
    /*
    #define EXIF_TAG_GPS_SPEED_REF         0x000c
    #define EXIF_TAG_GPS_SPEED             0x000d
    */
	entry = create_tag(exif, EXIF_IFD_GPS, EXIF_TAG_GPS_SPEED,9);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = 8;
    char cSpeed[20] = {0};
    sprintf(cSpeed,"%f",sGpsInfo.speed);
    memcpy(entry->data,cSpeed,8);
    //memcpy(entry->data,"1020.0045",8);
#endif


	/*date time*/
	entry = create_tag(exif, EXIF_IFD_EXIF, 0x9003,20);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = 19;
    char pDataTime[20] = {0};
    sprintf(pDataTime,"%d-%d-%d %d:%d:%d",sGpsInfo.D.year,sGpsInfo.D.month,sGpsInfo.D.day,
        sGpsInfo.D.hour,sGpsInfo.D.minute,sGpsInfo.D.second);
    memcpy(entry->data,pDataTime,19);	//memcpy(entry->data,"2012-12-11 10:00:00",19);

    /*EXIF_TAG_MAKER_NOTE*/
    entry = create_tag(exif, EXIF_IFD_EXIF, EXIF_TAG_MAKER_NOTE,strlen(pucMakerNote)+1);
	entry->format = EXIF_FORMAT_ASCII;
	entry->components = strlen(pucMakerNote);
    memcpy(entry->data,pucMakerNote,strlen(pucMakerNote));

	//printf("exif_data_len=%d\n",exif_data_len);
	exif_data_save_data(exif, &exif_data, &exif_data_len);
	//printf("exif_data_len=%d\n",exif_data_len);

    
	assert(exif_data != NULL);    
    
	//f = fopen(FILE_NAME, "wb");//write exif to this file pSrcFileName
	f = fopen(pSrcFileName, "wb");//write exif to this file pSrcFileName
	//f = fopen("19700101095721.jpg", "wb");//write exif to this file pSrcFileName
	if (!f) {
		fprintf(stderr, "Error creating file %s\n", FILE_NAME);
		exif_data_unref(exif);
		return rc;
	}
	/* Write EXIF header */
	if (fwrite(exif_header, exif_header_len, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	/* Write EXIF block length in big-endian order */
	if (fputc((exif_data_len+2) >> 8, f) < 0) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	if (fputc((exif_data_len+2) & 0xff, f) < 0) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	/* Write EXIF data block */
	if (fwrite(exif_data, exif_data_len, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}

    //printf("exif_data:%s \nimage_data_offset %d exif_header_len %d\n",exif_data,image_data_offset,exif_header_len);
       
    int image_data_len2 =  iFileSize - image_data_offset;
	/* Write JPEG image data, skipping the non-EXIF header */
	if (fwrite(buf+image_data_offset, image_data_len2, 1, f) != 1) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		goto errout;
	}
	printf("Wrote exif to jpg file\n");
	rc = 0;

errout:
	if (fclose(f)) {
		fprintf(stderr, "Error writing to file %s\n", FILE_NAME);
		rc = 1;
	}
	/* The allocator we're using for ExifData is the standard one, so use
	 * it directly to free this pointer.
	 */
	free(exif_data);
	exif_data_unref(exif);
	FILE * ddp = fopen("test.jpg","wb");
	//fwrite(image_jpg,image_jpg_len,1,ddp);//pSrcFileName
	fwrite(pSrcFileName,image_jpg_len,1,ddp);//
	fclose(ddp);
	return rc;
}





