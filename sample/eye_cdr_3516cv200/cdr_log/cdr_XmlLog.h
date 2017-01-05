
#ifndef XMLLOG_H
#define XMLLOG_H

#include "minxml/mxml.h"

typedef enum{
    ePHOTO = 1,
    eGPHOTO = 2,
    eAPPLOGIN = 3,
    eSTART_CDR = 4,
    eSTOP_CDR= 5,
}eRecodType;

#define PHOTO      "Photo"
#define GPHOTO     "GPhoto"
#define APP_LOGIN  "App login"
#define START_CDR  "Start CDR"
#define STOP_CDR   "Stop CDR"

int cdr_add_log(eRecodType eRecodType,  unsigned char const *pucName, unsigned char const *pucValue);

mxml_node_t * cdr_add_xml_txt_elem(mxml_node_t *parent,unsigned char *name,unsigned char *pValue);
mxml_node_t * cdr_add_xml_int_elem(mxml_node_t *parent,unsigned char *name,unsigned long pValue);

unsigned int get_cdr_start_milage();
unsigned int get_cdr_stop_milage();
unsigned int get_cdr_trip_milage(unsigned int ulMileage);

void set_cdr_start_sec(void);
unsigned int get_cdr_start_sec(void);
unsigned int get_cdr_stop_sec();
unsigned int get_cdr_trip_sec(unsigned int ulSec);
#endif



