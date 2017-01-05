
#include "cdr_XmlLog.h"
#include <assert.h>
#include "cdr_comm.h"
#include "cdr_gp.h"

extern unsigned char g_ucGpsDataBuff[300];

static unsigned int g_uiStartMileage = 0;
static unsigned int g_ulStartTimeSec = 0;
static unsigned int g_stop_milage = 0;
static unsigned int g_stop_sec = 0;
unsigned int g_cdr_start_sec = 0;

static int cdr_get_gps_data(unsigned char *pucGpsDataBuff)
{
	const unsigned char *pGpsTemp = "$GPRMC,000000,A,0000.0000,N,00000.0000,E,38.16,140.89,170816,-1.6,E,A*2C";

	if(strlen(g_ucGpsDataBuff) < 2)
	{		
		//printf("[%s %d]get gps data fails\n",__FUNCTION__,__LINE__);
		memcpy(pucGpsDataBuff,pGpsTemp,strlen(pGpsTemp));
	}
	else
	{
		memcpy(pucGpsDataBuff,g_ucGpsDataBuff,strlen(g_ucGpsDataBuff)+1);
	}
	return 0;
}


//------second-----------------------------------------
unsigned int cdr_get_current_sec()
{
    unsigned int timep = 0;
    
    timep = time((time_t*)NULL);
    
    return timep;
}

unsigned int get_cdr_start_sec()
{
    unsigned int timep = 0;
    
    timep = g_cdr_start_sec;
    
    return timep;
}

void set_cdr_start_sec()
{
    unsigned int timep = 0;
    
    timep = time((time_t*)NULL);

    g_cdr_start_sec = timep;
}
int set_cdr_stop_sec(unsigned int uiSec)
{
    g_stop_sec = uiSec;
    
    return 0;
}

unsigned int get_cdr_stop_sec()
{
    printf("get_cdr_stop_sec:%d\n",g_stop_sec);
    if(g_stop_sec == 0x00)
    {
      g_stop_sec = time((time_t*)NULL);
    }
    return g_stop_sec;
}

unsigned int get_cdr_trip_sec(unsigned int ulSec)
{
    unsigned int ulTripTime = 0;
    
    ulTripTime = ulSec - g_ulStartTimeSec;
    
    return ulTripTime;
}

//-----------mileage---------------------------------------
unsigned int cdr_get_current_mileage()
{
    unsigned  int mileage = 0;//1100;

    return mileage;
}
unsigned int get_cdr_start_milage()
{
    unsigned  int mileage = 0;//1000;

    return mileage;
}
unsigned int get_cdr_stop_milage()
{
    g_stop_milage = 0;//1100;
    return g_stop_milage;
}
void set_cdr_stop_milage(unsigned  int mileage)
{
    //test 
    unsigned  int imileage = 0;

    imileage = 0;//1100;
    
    g_stop_milage = imileage;
}
//input agrt :stopMileage
unsigned int get_cdr_trip_milage(unsigned int uiMileage)
{
    unsigned int uiTripMileage = 0;
    
    uiTripMileage = uiMileage - g_uiStartMileage;
    
    uiTripMileage = 0;//100;
    
    return uiTripMileage;
}


mxml_node_t * cdr_add_xml_txt_elem(mxml_node_t *parent,unsigned char *name,unsigned char *pValue)
{    
    mxml_node_t *item;
    
    item = mxmlNewElement(parent, name);
    mxmlAdd(parent,MXML_ADD_AFTER,NULL, item);
    mxmlNewText(item, 0, pValue);
    
    return item;
}

mxml_node_t * cdr_add_xml_int_elem(mxml_node_t *parent,unsigned char *name,unsigned long pValue)
{    
    mxml_node_t *item;
    
    item = mxmlNewElement(parent, name);
    mxmlAdd(parent,MXML_ADD_AFTER,NULL, item);
    mxmlNewInteger(item, pValue);

    return item;
}

static int cdr_get_elem_type(eRecodType eRecodType,unsigned char *eRecodTypeArr)
{   
   switch(eRecodType)
    {
    case ePHOTO:     
        memcpy(eRecodTypeArr,PHOTO,strlen(PHOTO)+1);
        break;
    case eGPHOTO:    
        memcpy(eRecodTypeArr,GPHOTO,strlen(GPHOTO)+1);
        break;
    case eAPPLOGIN:  
        memcpy(eRecodTypeArr,APP_LOGIN,strlen(APP_LOGIN)+1);
        break;
    case eSTART_CDR: 
        memcpy(eRecodTypeArr,START_CDR,strlen(START_CDR)+1);
        break;
    case eSTOP_CDR:  
        memcpy(eRecodTypeArr,STOP_CDR,strlen(STOP_CDR)+1);
        break;
    default: 
        return -1;
    }
   return 0;

}

static pthread_mutex_t cdr_addlog_mutex;

int cdr_add_log(eRecodType eRecodType, unsigned char const *pucName, unsigned char const *pucValue)
{    
	pthread_mutex_lock(&cdr_addlog_mutex);

    unsigned char ucTimeTemp[30] = {0};
    unsigned char ucTimeTempFile[10]= {0};
    unsigned char ucGprsTemp[300]    = {0};
    unsigned char ucFileName[40]    = {0};
    unsigned char eRecodTypeArr[20] = {0};

    //unsigned char ucNameArrTem[30] = {0};
    //unsigned char ucValueArrTem[40] = {0};

	//Add a flag for stop cdr.
	static int systemstopflag = 0;
	

    FILE *fp = NULL;
    mxml_node_t *tree,*log;
    mxml_node_t *item;

    unsigned  long ulMileage = 0;
    unsigned  long ulSec = 0;
    unsigned  long ulTripMileage = 0;
    unsigned  long ulTripTime = 0;

	//after STOP_CDR ,stop log.
   	if(systemstopflag == 1)
   	{
		pthread_mutex_unlock(&cdr_addlog_mutex);
   		return 0;
   	}
   
    cdr_get_elem_type(eRecodType,eRecodTypeArr);
    
    cdr_log_get_time(ucTimeTemp);
    //set_cdr_start_time();

    memcpy(ucTimeTempFile,ucTimeTemp,8);

    snprintf(ucFileName,40,"/mnt/mmc/log/cdr_log%s.xml",ucTimeTempFile);
	
    cdr_get_gps_data(ucGprsTemp);

    fp = fopen(ucFileName, "r");
    if(fp == NULL)
    {
        tree = mxmlNewXML("1.0");
        log = mxmlNewElement(tree, "log");          
    }
    else
	{
        tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
        fclose(fp);
        log = mxmlFindElement(tree, tree, "log",NULL, NULL,MXML_DESCEND);
        if(log == NULL)
        {
        	//remove the error format file.
        	remove(ucFileName);
        	mxmlRelease(tree);		
			pthread_mutex_unlock(&cdr_addlog_mutex);
        	return -1;
        }
    }
    //item
    item = mxmlNewElement(log, "item");
    mxmlAdd(log,MXML_ADD_AFTER,NULL, item);

    cdr_add_xml_txt_elem(item,"time",ucTimeTemp);
    cdr_add_xml_txt_elem(item,"type",eRecodTypeArr);
    cdr_add_xml_txt_elem(item,"gps",ucGprsTemp);

    //media 
    if((NULL!=pucName)&&(NULL!=pucValue))
    {
       cdr_add_xml_txt_elem(item,pucName,pucValue);
    }

    ulMileage = cdr_get_current_mileage();
    ulSec = cdr_get_current_sec();
    
    if(strcmp(START_CDR,eRecodTypeArr) == 0)
    {      
      g_ulStartTimeSec = get_cdr_start_sec();
      g_uiStartMileage = get_cdr_start_milage();
      
      cdr_add_xml_int_elem(item,"startMileage",g_uiStartMileage);
    }

    if(strcmp(STOP_CDR,eRecodTypeArr) == 0)
    {
      ulTripMileage = get_cdr_trip_milage(1100);
      ulTripTime = get_cdr_trip_sec(ulSec);
      set_cdr_stop_sec(ulSec);
      
      //endMileage
      cdr_add_xml_int_elem(item,"endMileage",ulMileage);  
      set_cdr_stop_milage(ulMileage);
      
      //tirpMileage
      cdr_add_xml_int_elem(item,"tirpMileage",ulTripMileage);
      
      //tirpTime
      cdr_add_xml_int_elem(item,"tirpTime",(unsigned int)ulTripTime);  
	  
	  systemstopflag = 1;
    }
    
    fp = fopen(ucFileName, "w");
    if(fp==NULL)
    {
        printf("[%s %d] open file fails\n",__FUNCTION__,__LINE__);
		pthread_mutex_unlock(&cdr_addlog_mutex);
        return -1;
    }
    mxmlSaveFile(tree,fp,MXML_NO_CALLBACK);
    fflush(fp);
    fclose(fp);	
    fp = NULL;
    mxmlDelete(tree); 
	pthread_mutex_unlock(&cdr_addlog_mutex);

    return 0;
}





