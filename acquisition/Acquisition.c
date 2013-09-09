#include "Acquisition.h"
#include "acquisition_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dlfcn.h>

#define EPOCH_YEAR_IN_TM_YEAR 1900


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

//This should probably pass on to each of the sub modules
float minDistance = -10;
float scaleFactor = 0.0021;

const char *days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

unsigned long tickBase = 0;

unsigned int simulateTick=0;
unsigned long simulatedTickValue=0;

struct acquisitionPluginInterface plugins[NUMBER_OF_POSSIBLE_MODULES]={0};


int acquisitionSimulateTime(unsigned long timeInMillisecs)
{
  simulateTick=1;
  simulatedTickValue=timeInMillisecs;
  return 1;
}

int fileExists(char * filename)
{
  FILE * fp  = fopen(filename,"r");
    if (fp!=0)
    {
      fclose(fp);
      return 1;
    }

  return 0;
}


unsigned long GetTickCount()
{
   if (simulateTick) { return simulatedTickValue; }

   //This returns a monotnic "uptime" value in milliseconds , it behaves like windows GetTickCount() but its not the same..
   struct timespec ts;
   if ( clock_gettime(CLOCK_MONOTONIC,&ts) != 0) { fprintf(stderr,"Error Getting Tick Count\n"); return 0; }

   if (tickBase==0)
   {
     tickBase = ts.tv_sec*1000 + ts.tv_nsec/1000000;
     return 0;
   }

   return ( ts.tv_sec*1000 + ts.tv_nsec/1000000 ) - tickBase;
}

/*
int GetDateString(char * output,char * label,unsigned int now,unsigned int dayofweek,unsigned int day,unsigned int month,unsigned int year,unsigned int hour,unsigned int minute,unsigned int second)
{
   //Date: Sat, 29 May 2010 12:31:35 GMT
   //Last-Modified: Sat, 29 May 2010 12:31:35 GMT
   if ( now )
      {
        time_t clock = time(NULL);
        struct tm * ptm = gmtime ( &clock );

        sprintf(output,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[ptm->tm_wday],ptm->tm_mday,months[ptm->tm_mon],EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

      } else
      {
        sprintf(output,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[dayofweek],day,months[month],year,hour,minute,second);
      }
    return 1;
}*/




unsigned int simplePow(unsigned int base,unsigned int exp)
{
    if (exp==0) return 1;
    unsigned int retres=base;
    unsigned int i=0;
    for (i=0; i<exp-1; i++)
    {
        retres*=base;
    }
    return retres;
}



int savePCD_PointCloud(char * filename , short * depthFrame , char * colorFrame , unsigned int width , unsigned int height , float cx , float cy , float fx , float fy )
{
    if(depthFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) depth frame , will not write any file output\n",filename); return 0; }
    if(colorFrame==0) { fprintf(stderr,"saveToPCD_PointCloud(%s) called for an unallocated (empty) color frame , will not write any file output\n",filename); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");
    if (fd!=0)
    {
        fprintf(fd, "# .PCD v.5 - Point Cloud Data file format\n");
        fprintf(fd, "FIELDS x y z rgb\n");
        fprintf(fd, "SIZE 4 4 4 4\n");
        fprintf(fd, "TYPE F F F U\n");
        fprintf(fd, "WIDTH %u\n",width);
        fprintf(fd, "HEIGHT %u\n",height);
        fprintf(fd, "POINTS %u\n",height*width);
        fprintf(fd, "DATA ascii\n");

        short * depthPTR = depthFrame;
        char  * colorPTR = colorFrame;

        unsigned int px=0,py=0;
        float x=0.0,y=0.0,z=0.0;
        unsigned char * r , * b , * g;
        unsigned int rgb=0;

        for (py=0; py<height; py++)
        {
         for (px=0; px<width; px++)
         {
           z = * depthPTR; ++depthPTR;
           x = (px - cx) * (z + minDistance) * scaleFactor * (width/height) ;
           y = (py - cy) * (z + minDistance) * scaleFactor;

           r=colorPTR; ++colorPTR;
           g=colorPTR; ++colorPTR;
           b=colorPTR; ++colorPTR;


        /* To pack it :
            int rgb = ((int)r) << 16 | ((int)g) << 8 | ((int)b);

           To unpack it :
            int rgb = ...;
            uint8_t r = (rgb >> 16) & 0x0000ff;
            uint8_t g = (rgb >> 8) & 0x0000ff;
            uint8_t b = (rgb) & 0x0000ff; */
           rgb = ((int)*r) << 16 | ((int)*g) << 8 | ((int)*b);

           fprintf(fd, "%0.4f %0.4f %0.4f %u\n",x,y,z,rgb);
         }
        }
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }

   return 0;
}




int saveRawImageToFile(char * filename,char * pixels , unsigned int width , unsigned int height , unsigned int channels , unsigned int bitsperpixel)
{
    //fprintf(stderr,"saveRawImageToFile(%s) called\n",filename);

    if ( (width==0) || (height==0) || (channels==0) || (bitsperpixel==0) ) { fprintf(stderr,"saveRawImageToFile(%s) called with zero dimensions\n",filename); return 0;}
    if(pixels==0) { fprintf(stderr,"saveRawImageToFile(%s) called for an unallocated (empty) frame , will not write any file output\n",filename); return 0; }
    if (bitsperpixel>16) { fprintf(stderr,"PNM does not support more than 2 bytes per pixel..!\n"); return 0; }

    FILE *fd=0;
    fd = fopen(filename,"wb");

    if (fd!=0)
    {
        unsigned int n;
        if (channels==3) fprintf(fd, "P6\n");
        else if (channels==1) fprintf(fd, "P5\n");
        else
        {
            fprintf(stderr,"Invalid channels arg (%u) for SaveRawImageToFile\n",channels);
            fclose(fd);
            return 1;
        }

        char output[256]={0};
      /*GetDateString(output,"TIMESTAMP",1,0,0,0,0,0,0,0);
        fprintf(fd, "#%s\n", output );*/

        fprintf(fd, "#TIMESTAMP %u\n",GetTickCount());


        fprintf(fd, "%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);

        float tmp_n = (float) bitsperpixel/ 8;
        tmp_n = tmp_n *  width * height * channels ;
        n = (unsigned int) tmp_n;

        fwrite(pixels, 1 , n , fd);
        fflush(fd);
        fclose(fd);
        return 1;
    }
    else
    {
        fprintf(stderr,"SaveRawImageToFile could not open output file %s\n",filename);
        return 0;
    }
    return 0;
}

//Ok this is basically casting the 2 bytes of depth into 3 RGB bytes leaving one color channel off (the blue one)
//depth is casted to char to simplify things , but that adds sizeof(short) to the pointer arethemetic!
char * convertShortDepthToRGBDepth(short * depth,unsigned int width , unsigned int height)
{
  if (depth==0)  { fprintf(stderr,"Depth is not allocated , cannot perform DepthToRGB transformation \n"); return 0; }
  char * depthPTR= (char*) depth; // This will be the traversing pointer for input
  char * depthLimit = (char*) depth + width*height * sizeof(short); //<- we use sizeof(short) because we have casted to char !


  char * outFrame = (char*) malloc(width*height*3*sizeof(char));
  if (outFrame==0) { fprintf(stderr,"Could not perform DepthToRGB transformation\nNo memory for new frame\n"); return 0; }

  char * outFramePTR = outFrame; // This will be the traversing pointer for output
  while ( depthPTR<depthLimit )
  {
     * outFramePTR = *depthPTR; ++outFramePTR; ++depthPTR;
     * outFramePTR = *depthPTR; ++outFramePTR; ++depthPTR;
     * outFramePTR = 0;         ++outFramePTR;
  }
 return outFrame;
}


//Ok this is basically casting the 2 bytes of depth into 3 RGB bytes leaving one color channel off (the blue one)
//depth is casted to char to simplify things , but that adds sizeof(short) to the pointer arethemetic!
char * convertShortDepthToCharDepth(short * depth,unsigned int width , unsigned int height , unsigned int min_depth , unsigned int max_depth)
{
  if (depth==0)  { fprintf(stderr,"Depth is not allocated , cannot perform DepthToRGB transformation \n"); return 0; }
  short * depthPTR= depth; // This will be the traversing pointer for input
  short * depthLimit =  depth + width*height; //<- we use sizeof(short) because we have casted to char !


  char * outFrame = (char*) malloc(width*height*1*sizeof(char));
  if (outFrame==0) { fprintf(stderr,"Could not perform DepthToRGB transformation\nNo memory for new frame\n"); return 0; }

  float depth_range = max_depth-min_depth;
  if (depth_range ==0 ) { depth_range = 1; }
  float multiplier = 255 / depth_range;


  char * outFramePTR = outFrame; // This will be the traversing pointer for output
  while ( depthPTR<depthLimit )
  {
     unsigned int scaled = (unsigned int) (*depthPTR) * multiplier;
     unsigned char scaledChar = (unsigned char) scaled;
     * outFramePTR = scaledChar;

     ++outFramePTR;
     ++depthPTR;
  }
 return outFrame;
}



int acquisitionGetModulesCount()
{
  unsigned int modules_linked = 0;


  #if USE_V4L2
   modules_linked+=2;
   fprintf(stderr,"V4L2 code linked\n");
   fprintf(stderr,"V4L2Stereo code linked\n");
  #endif

  #if USE_OPENGL
   ++modules_linked;
   fprintf(stderr,"OpenGL code linked\n");
  #endif

  #if USE_OPENNI1
   ++modules_linked;
   fprintf(stderr,"OpenNI1 code linked\n");
  #endif

  #if USE_OPENNI2
   ++modules_linked;
   fprintf(stderr,"OpenNI2 code linked\n");
  #endif

  #if USE_FREENECT
   ++modules_linked;
   fprintf(stderr,"Freenect code linked\n");
  #endif

  #if USE_TEMPLATE
   ++modules_linked;
   fprintf(stderr,"Template code linked\n");
  #endif

  return modules_linked;

}


ModuleIdentifier getModuleIdFromModuleName(char * moduleName)
{
   ModuleIdentifier moduleID = 0;
          if (strcasecmp("FREENECT",moduleName)==0 )  { moduleID = FREENECT_ACQUISITION_MODULE; } else
          if (strcasecmp("OPENNI",moduleName)==0 )  { moduleID = OPENNI1_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENNI1",moduleName)==0 )  { moduleID = OPENNI1_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENNI2",moduleName)==0 )  { moduleID = OPENNI2_ACQUISITION_MODULE;  } else
          if (strcasecmp("OPENGL",moduleName)==0 )   { moduleID = OPENGL_ACQUISITION_MODULE;   } else
          if (strcasecmp("V4L2",moduleName)==0 )   { moduleID = V4L2_ACQUISITION_MODULE;   } else
          if (strcasecmp("V4L2STEREO",moduleName)==0 )   { moduleID = V4L2STEREO_ACQUISITION_MODULE;   } else
          if (strcasecmp("TEMPLATE",moduleName)==0 )  { moduleID = TEMPLATE_ACQUISITION_MODULE; }
   return moduleID;
}


char * getModuleStringName(ModuleIdentifier moduleID)
{
  switch (moduleID)
    {
      case V4L2_ACQUISITION_MODULE    :  return (char*) "V4L2 MODULE"; break;
      case V4L2STEREO_ACQUISITION_MODULE    :  return (char*) "V4L2STEREO MODULE"; break;
      case FREENECT_ACQUISITION_MODULE:  return (char*) "FREENECT MODULE"; break;
      case OPENNI1_ACQUISITION_MODULE :  return (char*) "OPENNI1 MODULE"; break;
      case OPENNI2_ACQUISITION_MODULE :  return (char*) "OPENNI2 MODULE"; break;
      case OPENGL_ACQUISITION_MODULE :  return (char*) "OPENGL MODULE"; break;
      case TEMPLATE_ACQUISITION_MODULE    :  return (char*) "TEMPLATE MODULE"; break;
    };
    return (char*) "UNKNOWN MODULE";
}



int acquisitionIsModuleLinked(ModuleIdentifier moduleID)
{
    switch (moduleID)
    {
      #if USE_V4L2
      case V4L2_ACQUISITION_MODULE    :
          return 1;
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
          return 1;
      break;
      #endif
      case OPENGL_ACQUISITION_MODULE    :
        #if USE_OPENGL
          return 1;
        #endif
      break;
      case TEMPLATE_ACQUISITION_MODULE:
        #if USE_TEMPLATE
          return 1;
        #endif
      break;
      case FREENECT_ACQUISITION_MODULE:
        #if USE_FREENECT
          return 1;
        #endif
      break;
      case OPENNI1_ACQUISITION_MODULE :
        #if USE_OPENNI1
          return 1;
        #endif
      break;
      case OPENNI2_ACQUISITION_MODULE :
        #if USE_OPENNI2
          return 1;
        #endif
      break;
    };

    return 0;
}


void printCall(ModuleIdentifier moduleID,DeviceIdentifier devID,char * fromFunction)
{
   #if PRINT_DEBUG_EACH_CALL
    fprintf(stderr,"called %s module %u , device %u ..\n",fromFunction,moduleID,devID);
   #endif
}



void MeaningfullWarningMessage(ModuleIdentifier moduleFailed,DeviceIdentifier devFailed,char * fromFunction)
{
  if (!acquisitionIsModuleLinked(moduleFailed))
   {
       fprintf(stderr,"%s is not linked in this build of the acquisition library system..\n",getModuleStringName(moduleFailed));
       return ;
   }

   fprintf(stderr,"%s hasn't got an implementation for function %s ..\n",getModuleStringName(moduleFailed),fromFunction);
}



int linkToPlugin(char * moduleName,char * modulePath ,  ModuleIdentifier moduleID)
{
   char functionNameStr[1024]={0};
   char *error;
   plugins[moduleID].handle = dlopen (modulePath, RTLD_LAZY);
   if (!plugins[moduleID].handle)
       {
        fprintf (stderr, "Failed while loading %s  - %s\n", moduleName , dlerror());
        return 0;
       }

    dlerror();    /* Clear any existing error */


  //Start Stop ================================================================================================================
  sprintf(functionNameStr,"start%sModule",moduleName);
  plugins[moduleID].startModule = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }

  sprintf(functionNameStr,"stop%sModule",moduleName);
  plugins[moduleID].stopModule = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }

  //================================================================================================================
  sprintf(functionNameStr,"map%sDepthToRGB",moduleName);
  plugins[moduleID].mapDepthToRGB = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"map%sRGBToDepth",moduleName);
  plugins[moduleID].mapRGBToDepth = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }


  sprintf(functionNameStr,"create%sDevice",moduleName);
  plugins[moduleID].createDevice = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"destroy%sDevice",moduleName);
  plugins[moduleID].destroyDevice = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }


  sprintf(functionNameStr,"get%sNumberOfDevices",moduleName);
  plugins[moduleID].getNumberOfDevices = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }


  sprintf(functionNameStr,"seek%sFrame",moduleName);
  plugins[moduleID].seekFrame = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"snap%sFrames",moduleName);
  plugins[moduleID].snapFrames = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }


  sprintf(functionNameStr,"getLast%sColorTimestamp",moduleName);
  plugins[moduleID].getLastColorTimestamp = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"getLast%sDepthTimestamp",moduleName);
  plugins[moduleID].getLastDepthTimestamp = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }


  sprintf(functionNameStr,"get%sColorWidth",moduleName);
  plugins[moduleID].getColorWidth = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorHeight",moduleName);
  plugins[moduleID].getColorHeight = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorDataSize",moduleName);
  plugins[moduleID].getColorDataSize = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorChannels",moduleName);
  plugins[moduleID].getColorChannels = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorBitsPerPixel",moduleName);
  plugins[moduleID].getColorBitsPerPixel = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorPixels",moduleName);
  plugins[moduleID].getColorPixels = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorFocalLength",moduleName);
  plugins[moduleID].getColorFocalLength = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorPixelSize",moduleName);
  plugins[moduleID].getColorPixelSize = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sColorCalibration",moduleName);
  plugins[moduleID].getColorCalibration = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"set%sColorCalibration",moduleName);
  plugins[moduleID].setColorCalibration = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }



  sprintf(functionNameStr,"get%sDepthWidth",moduleName);
  plugins[moduleID].getDepthWidth = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthHeight",moduleName);
  plugins[moduleID].getDepthHeight = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthDataSize",moduleName);
  plugins[moduleID].getDepthDataSize = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthChannels",moduleName);
  plugins[moduleID].getDepthChannels = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthBitsPerPixel",moduleName);
  plugins[moduleID].getDepthBitsPerPixel = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthPixels",moduleName);
  plugins[moduleID].getDepthPixels = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthFocalLength",moduleName);
  plugins[moduleID].getDepthFocalLength = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthPixelSize",moduleName);
  plugins[moduleID].getDepthPixelSize = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"get%sDepthCalibration",moduleName);
  plugins[moduleID].getDepthCalibration = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }
  sprintf(functionNameStr,"set%sDepthCalibration",moduleName);
  plugins[moduleID].setDepthCalibration = dlsym(plugins[moduleID].handle, functionNameStr );
  if ((error = dlerror()) != NULL)  { fprintf (stderr, YELLOW  "Could not find a definition of %s : %s\n" NORMAL ,functionNameStr ,  error); }



  return 1;
}

int unlinkPlugin(ModuleIdentifier moduleID)
{
    if (plugins[moduleID].handle==0) { return 1; }
    dlclose(plugins[moduleID].handle);
    plugins[moduleID].handle=0;
    return 1;
}



/*! ------------------------------------------
    BASIC START STOP MECHANISMS FOR MODULES..
   ------------------------------------------*/
int acquisitionStartModule(ModuleIdentifier moduleID,unsigned int maxDevices,char * settings)
{
    switch (moduleID)
    {
      case V4L2_ACQUISITION_MODULE    :

          if (fileExists("../v4l2_acquisition_shared_library/libV4L2Acquisition.so"))
                linkToPlugin("V4L2","../v4l2_acquisition_shared_library/libV4L2Acquisition.so",moduleID); else
          if (fileExists("libV4L2Acquisition.so"))
                linkToPlugin("V4L2","libV4L2Acquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL ,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
          if (fileExists("../v4l2stereo_acquisition_shared_library/libV4L2StereoAcquisition.so"))
                linkToPlugin("V4L2Stereo","../v4l2stereo_acquisition_shared_library/libV4L2StereoAcquisition.so",moduleID); else
          if (fileExists("libV4L2StereoAcquisition.so"))
                linkToPlugin("V4L2Stereo","libV4L2StereoAcquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case OPENGL_ACQUISITION_MODULE    :
          if (fileExists("../opengl_acquisition_shared_library/libOpenGLAcquisition.so"))
                 linkToPlugin("OpenGL","../opengl_acquisition_shared_library/libOpenGLAcquisition.so",moduleID); else
          if (fileExists("libOpenGLAcquisition.so"))
                 linkToPlugin("OpenGL","libOpenGLAcquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case TEMPLATE_ACQUISITION_MODULE:
          if (fileExists("../template_acquisition_shared_library/libTemplateAcquisition.so"))
                 linkToPlugin("Template","../template_acquisition_shared_library/libTemplateAcquisition.so",moduleID); else
          if (fileExists("libTemplateAcquisition.so"))
                 linkToPlugin("Template","libTemplateAcquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case FREENECT_ACQUISITION_MODULE:
          if (fileExists("../libfreenect_acquisition_shared_library/libFreenectAcquisition.so"))
                 linkToPlugin("Freenect","../libfreenect_acquisition_shared_library/libFreenectAcquisition.so",moduleID); else
          if (fileExists("libFreenectAcquisition.so"))
                 linkToPlugin("Freenect","libFreenectAcquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case OPENNI1_ACQUISITION_MODULE :
          if (fileExists("../openni1_acquisition_shared_library/libOpenNI1Acquisition.so"))
                 linkToPlugin("OpenNI1","../openni1_acquisition_shared_library/libOpenNI1Acquisition.so",moduleID); else
          if (fileExists("libOpenNI1Acquisition.so"))
                 linkToPlugin("OpenNI1","libOpenNI1Acquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
      case OPENNI2_ACQUISITION_MODULE :
          if (fileExists("../openni2_acquisition_shared_library/libOpenNI2Acquisition.so"))
                 linkToPlugin("OpenNI2","../openni2_acquisition_shared_library/libOpenNI2Acquisition.so",moduleID); else
          if (fileExists("libOpenNI2Acquisition.so"))
                 linkToPlugin("OpenNI2","libOpenNI2Acquisition.so",moduleID); else
          { fprintf(stderr,RED "Could not find %s plugin shared object \n" NORMAL,getModuleStringName(moduleID)); return 0; }

          if (*plugins[moduleID].startModule!=0) { return (*plugins[moduleID].startModule) (maxDevices,settings); }
      break;
    };

    MeaningfullWarningMessage(moduleID,0,"acquisitionStartModule");
    return 0;
}


int acquisitionStopModule(ModuleIdentifier moduleID)
{
    if (*plugins[moduleID].stopModule!=0) { return (*plugins[moduleID].stopModule) (); }
    unlinkPlugin(moduleID);

    MeaningfullWarningMessage(moduleID,0,"acquisitionStopModule");
    return 0;
}


int acquisitionGetModuleDevices(ModuleIdentifier moduleID)
{
    printCall(moduleID,0,"acquisitionGetModuleDevices");
    if (plugins[moduleID].getNumberOfDevices!=0) { return (*plugins[moduleID].getNumberOfDevices) (); }
    MeaningfullWarningMessage(moduleID,0,"acquisitionGetModuleDevices");
    return 0;
}



/*! ------------------------------------------
    FRAME SNAPPING MECHANISMS FOR MODULES..
   ------------------------------------------*/
int acquisitionOpenDevice(ModuleIdentifier moduleID,DeviceIdentifier devID,char * devName,unsigned int width,unsigned int height,unsigned int framerate)
{
    printCall(moduleID,devID,"acquisitionOpenDevice");
    if (plugins[moduleID].createDevice!=0) { return (*plugins[moduleID].createDevice) (devID,devName,width,height,framerate); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionOpenDevice");
    return 0;
}

 int acquisitionCloseDevice(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionCloseDevice");
    if (plugins[moduleID].destroyDevice!=0) { return (*plugins[moduleID].destroyDevice) (devID); }

    switch (moduleID)
    {
      #if USE_V4L2
      case V4L2_ACQUISITION_MODULE    :
          return destroyV4L2Device(devID);
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
          return destroyV4L2StereoDevice(devID);
      break;
      #endif
    };
    MeaningfullWarningMessage(moduleID,devID,"acquisitionCloseDevice");
    return 0;
}


 int acquisitionSeekFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int seekFrame)
{
    printCall(moduleID,devID,"acquisitionSeekFrame");
    if (*plugins[moduleID].seekFrame!=0) { return (*plugins[moduleID].seekFrame) (devID,seekFrame); }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSeekFrame");
    return 0;
}


 int acquisitionSnapFrames(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionSnapFrames");
    //fprintf(stderr,"acquisitionSnapFrames called moduleID=%u devID=%u\n",moduleID,devID);
    if (*plugins[moduleID].snapFrames!=0) { return (*plugins[moduleID].snapFrames) (devID); }

    switch (moduleID)
    {
      #if USE_V4L2
      case V4L2_ACQUISITION_MODULE    :
          return snapV4L2Frames(devID);
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
          return snapV4L2StereoFrames(devID);
      break;
      #endif
    };
    MeaningfullWarningMessage(moduleID,devID,"acquisitionSnapFrames");
    return 0;
}

 int acquisitionSaveColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColorFrame");
    char filenameFull[2048]={0};
    sprintf(filenameFull,"%s.pnm",filename);



         if (
              (*plugins[moduleID].getColorPixels!=0) && (*plugins[moduleID].getColorWidth!=0) && (*plugins[moduleID].getColorHeight!=0) &&
              (*plugins[moduleID].getColorChannels!=0) && (*plugins[moduleID].getColorBitsPerPixel!=0)
            )
         {
            return saveRawImageToFile(
                                      filenameFull,
                                      (*plugins[moduleID].getColorPixels)      (devID),
                                      (*plugins[moduleID].getColorWidth)       (devID),
                                      (*plugins[moduleID].getColorHeight)      (devID),
                                      (*plugins[moduleID].getColorChannels)    (devID),
                                      (*plugins[moduleID].getColorBitsPerPixel)(devID)
                                     );
         }


    switch (moduleID)
    {
      #if USE_V4L2
      case V4L2_ACQUISITION_MODULE    :
          return saveRawImageToFile(filename,getV4L2ColorPixels(devID),getV4L2ColorWidth(devID),getV4L2ColorHeight(devID),getV4L2ColorChannels(devID),getV4L2ColorBitsPerPixel(devID));
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
         sprintf(filenameFull,"%s_0.pnm",filename);
         saveRawImageToFile(filenameFull,getV4L2StereoColorPixelsLeft(devID),getV4L2StereoColorWidth(devID),getV4L2StereoColorHeight(devID),getV4L2StereoColorChannels(devID),getV4L2StereoColorBitsPerPixel(devID));
         sprintf(filenameFull,"%s_1.pnm",filename);
         saveRawImageToFile(filenameFull,getV4L2StereoColorPixelsRight(devID),getV4L2StereoColorWidth(devID),getV4L2StereoColorHeight(devID),getV4L2StereoColorChannels(devID),getV4L2StereoColorBitsPerPixel(devID));
        return 1;
      break;
      #endif
    };
    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColorFrame");
    return 0;
}



 int acquisitionSavePCDPointCoud(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
  unsigned int width;
  unsigned int height;
  unsigned int channels;
  unsigned int bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  return savePCD_PointCloud(filename,acquisitionGetDepthFrame(moduleID,devID),acquisitionGetColorFrame(moduleID,devID),
                            width,height,width/2,height/2, 1.0 /*DUMMY fx*/, 1.0 /*DUMMY fy*/ );
}



 int acquisitionSaveDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveDepthFrame");
    char filenameFull[2048]={0};
    sprintf(filenameFull,"%s.pnm",filename);


          if (
              (*plugins[moduleID].getDepthPixels!=0) && (*plugins[moduleID].getDepthWidth!=0) && (*plugins[moduleID].getDepthHeight!=0) &&
              (*plugins[moduleID].getDepthChannels!=0) && (*plugins[moduleID].getDepthBitsPerPixel!=0)
             )
         {
            return saveRawImageToFile(
                                      filenameFull,
                                      (*plugins[moduleID].getDepthPixels)      (devID),
                                      (*plugins[moduleID].getDepthWidth)       (devID),
                                      (*plugins[moduleID].getDepthHeight)      (devID),
                                      (*plugins[moduleID].getDepthChannels)    (devID),
                                      (*plugins[moduleID].getDepthBitsPerPixel)(devID)
                                     );
         }

    switch (moduleID)
    {
      #if USE_V4L2
      case V4L2_ACQUISITION_MODULE    :
           fprintf(stderr,"V4L2 Does not have a depth frame\n");
      break;
      case V4L2STEREO_ACQUISITION_MODULE    :
          return saveRawImageToFile(filenameFull,getV4L2StereoDepthPixels(devID),getV4L2StereoDepthWidth(devID),getV4L2StereoDepthHeight(devID),getV4L2StereoDepthChannels(devID),getV4L2StereoDepthBitsPerPixel(devID));
      break;
      #endif
    };
    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveDepthFrame");
    return 0;
}

 int acquisitionSaveColoredDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColoredDepthFrame");

    char filenameFull[1024]={0};
    sprintf(filenameFull,"%s.pnm",filename);

    unsigned int width = 0 ;
    unsigned int height = 0 ;
    unsigned int channels = 0 ;
    unsigned int bitsperpixel = 0 ;
    short * inFrame = 0;
    char * outFrame = 0 ;

    inFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (inFrame!=0)
      {
       acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
       outFrame = convertShortDepthToRGBDepth(inFrame,width,height);
       if (outFrame!=0)
        {
         saveRawImageToFile(filenameFull,outFrame,width,height,3,8);
         free(outFrame);
         return 1;
        }
      }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColoredDepthFrame");
    return 0;
}


int acquisitionSaveDepthFrame1C(ModuleIdentifier moduleID,DeviceIdentifier devID,char * filename)
{
    printCall(moduleID,devID,"acquisitionSaveColoredDepthFrame");

    char filenameFull[1024]={0};
    sprintf(filenameFull,"%s.pnm",filename);

    unsigned int width = 0 ;
    unsigned int height = 0 ;
    unsigned int channels = 0 ;
    unsigned int bitsperpixel = 0 ;
    short * inFrame = 0;
    char * outFrame = 0 ;

    inFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (inFrame!=0)
      {
       acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
       outFrame = convertShortDepthToCharDepth(inFrame,width,height,0,7000);
       if (outFrame!=0)
        {
         saveRawImageToFile(filenameFull,outFrame,width,height,1,8);
         free(outFrame);
         return 1;
        }
      }

    MeaningfullWarningMessage(moduleID,devID,"acquisitionSaveColoredDepthFrame");
    return 0;
}



int acquisitionGetColorCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetColorCalibration");
   if (*plugins[moduleID].getColorCalibration!=0) { return (*plugins[moduleID].getColorCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorCalibration");
   return 0;
}

int acquisitionGetDepthCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetDepthCalibration");
   if (*plugins[moduleID].getDepthCalibration!=0) { return (*plugins[moduleID].getDepthCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthCalibration");
   return 0;
}





int acquisitionSetColorCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetColorCalibration");
   if (*plugins[moduleID].setColorCalibration!=0) { return (*plugins[moduleID].setColorCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorCalibration");
   return 0;
}

int acquisitionSetDepthCalibration(ModuleIdentifier moduleID,DeviceIdentifier devID,struct calibration * calib)
{
   printCall(moduleID,devID,"acquisitionGetDepthCalibration");
   if (*plugins[moduleID].setDepthCalibration!=0) { return (*plugins[moduleID].setDepthCalibration) (devID,calib); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthCalibration");
   return 0;
}


unsigned long acquisitionGetColorTimestamp(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
   printCall(moduleID,devID,"acquisitionGetColorTimestamp");
   if (*plugins[moduleID].getLastColorTimestamp!=0) { return (*plugins[moduleID].getLastColorTimestamp) (devID); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorTimestamp");
   return 0;
}

unsigned long acquisitionGetDepthTimestamp(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
   printCall(moduleID,devID,"acquisitionGetDepthTimestamp");
   if (*plugins[moduleID].getLastDepthTimestamp!=0) { return (*plugins[moduleID].getLastDepthTimestamp) (devID); }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthTimestamp");
   return 0;
}


char * acquisitionGetColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetColorFrame");
  if (*plugins[moduleID].getColorPixels!=0) { return (*plugins[moduleID].getColorPixels) (devID); }
  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorFrame");
  return 0;
}

unsigned int acquisitionCopyColorFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,char * mem,unsigned int memlength)
{
  char * color = acquisitionGetColorFrame(moduleID,devID);
  if (color==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
  unsigned int copySize = width*height*channels*(bitsperpixel/8);
  memcpy(mem,color,copySize);
  return copySize;
}


unsigned int acquisitionCopyColorFramePPM(ModuleIdentifier moduleID,DeviceIdentifier devID,char * mem,unsigned int memlength)
{
  char * color = acquisitionGetColorFrame(moduleID,devID);
  if (color==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetColorFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  sprintf(mem, "P6%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);
  unsigned int payloadStart = strlen(mem);

  char * memPayload = mem + payloadStart ;
  memcpy(memPayload,color,width*height*channels*(bitsperpixel/8));

  payloadStart += width*height*channels*(bitsperpixel/8);
  return payloadStart;
}

short * acquisitionGetDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
  printCall(moduleID,devID,"acquisitionGetDepthFrame");
  if (*plugins[moduleID].getDepthPixels!=0) { return (short*) (*plugins[moduleID].getDepthPixels) (devID); }
  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthFrame");
  return 0;
}


unsigned int acquisitionCopyDepthFrame(ModuleIdentifier moduleID,DeviceIdentifier devID,short * mem,unsigned int memlength)
{
  short * depth = acquisitionGetDepthFrame(moduleID,devID);
  if (depth==0) { return 0; }
  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);
  unsigned int copySize = width*height*channels*(bitsperpixel/8);
  memcpy(mem,depth,copySize);
  return copySize;
}


unsigned int acquisitionCopyDepthFramePPM(ModuleIdentifier moduleID,DeviceIdentifier devID,short * mem,unsigned int memlength)
{
  short * depth = acquisitionGetDepthFrame(moduleID,devID);
  if (depth==0) { return 0; }

  unsigned int width , height , channels , bitsperpixel;
  acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel);

  sprintf((char*) mem, "P5%d %d\n%u\n", width, height , simplePow(2 ,bitsperpixel)-1);
  unsigned int payloadStart = strlen((char*) mem);

  short * memPayload = mem + payloadStart ;
  memcpy(memPayload,depth,width*height*channels*(bitsperpixel/8));

  payloadStart += width*height*channels*(bitsperpixel/8);
  return payloadStart;
}


int acquisitionGetDepth3DPointAtXY(ModuleIdentifier moduleID,DeviceIdentifier devID,unsigned int x2d, unsigned int y2d , float *x, float *y , float *z  )
{
    short * depthFrame = acquisitionGetDepthFrame(moduleID,devID);
    if (depthFrame == 0 ) { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepth3DPointAtXY , getting depth frame"); return 0; }

    unsigned int width; unsigned int height; unsigned int channels; unsigned int bitsperpixel;
    if (! acquisitionGetDepthFrameDimensions(moduleID,devID,&width,&height,&channels,&bitsperpixel) ) {  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepth3DPointAtXY getting depth frame dims"); return 0; }

    if ( (x2d>=width) || (y2d>=height) ) { MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepth3DPointAtXY incorrect 2d x,y coords"); return 0; }

    float cx = width / 2;
    float cy = height/ 2;
    short * depthValue = depthFrame + (y2d * width + x2d );
    *z = * depthValue;
    *x = (x2d - cx) * (*z + minDistance) * scaleFactor * (width/height) ;
    *y = (y2d - cy) * (*z + minDistance) * scaleFactor;

    return 1;
}



int acquisitionGetColorFrameDimensions(ModuleIdentifier moduleID,DeviceIdentifier devID ,
                                       unsigned int * width , unsigned int * height , unsigned int * channels , unsigned int * bitsperpixel )
{
  printCall(moduleID,devID,"acquisitionGetColorFrameDimensions");

  if ( (width==0)||(height==0)||(channels==0)||(bitsperpixel==0) )
    {
        fprintf(stderr,"acquisitionGetColorFrameDimensions called with invalid arguments .. \n");
        return 0;
    }


         if (
              (*plugins[moduleID].getColorWidth!=0) && (*plugins[moduleID].getColorHeight!=0) &&
              (*plugins[moduleID].getColorChannels!=0) && (*plugins[moduleID].getColorBitsPerPixel!=0)
            )
            {
              *width        = (*plugins[moduleID].getColorWidth)        (devID);
              *height       = (*plugins[moduleID].getColorHeight)       (devID);
              *channels     = (*plugins[moduleID].getColorChannels)     (devID);
              *bitsperpixel = (*plugins[moduleID].getColorBitsPerPixel) (devID);
              return 1;
            }

  MeaningfullWarningMessage(moduleID,devID,"acquisitionGetColorFrameDimensions");
  return 0;
}



int acquisitionGetDepthFrameDimensions(ModuleIdentifier moduleID,DeviceIdentifier devID ,
                                       unsigned int * width , unsigned int * height , unsigned int * channels , unsigned int * bitsperpixel )
{
  printCall(moduleID,devID,"acquisitionGetDepthFrameDimensions");

  if ( (width==0)||(height==0)||(channels==0)||(bitsperpixel==0) )
    {
        fprintf(stderr,"acquisitionGetDepthFrameDimensions called with invalid arguments .. \n");
        return 0;
    }


         if (
              (*plugins[moduleID].getDepthWidth!=0) && (*plugins[moduleID].getDepthHeight!=0) &&
              (*plugins[moduleID].getDepthChannels!=0) && (*plugins[moduleID].getDepthBitsPerPixel!=0)
            )
            {
              *width        = (*plugins[moduleID].getDepthWidth)        (devID);
              *height       = (*plugins[moduleID].getDepthHeight)       (devID);
              *channels     = (*plugins[moduleID].getDepthChannels)     (devID);
              *bitsperpixel = (*plugins[moduleID].getDepthBitsPerPixel) (devID);
              return 1;
            }
   MeaningfullWarningMessage(moduleID,devID,"acquisitionGetDepthFrameDimensions");
   return 0;
}


 int acquisitionMapDepthToRGB(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionMapDepthToRGB");
    if  (*plugins[moduleID].mapDepthToRGB!=0) { return  (*plugins[moduleID].mapDepthToRGB) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionMapDepthToRGB");
    return 0;
}


 int acquisitionMapRGBToDepth(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acquisitionMapRGBToDepth");
    if  (*plugins[moduleID].mapRGBToDepth!=0) { return  (*plugins[moduleID].mapRGBToDepth) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acquisitionMapRGBToDepth");
    return 0;
}



double acqusitionGetColorFocalLength(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
   printCall(moduleID,devID,"acqusitionGetColorFocalLength");
   if  (*plugins[moduleID].getColorFocalLength!=0) { return  (*plugins[moduleID].getColorFocalLength) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acqusitionGetColorFocalLength");
    return 0.0;
}

double acqusitionGetColorPixelSize(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acqusitionGetColorPixelSize");
    if  (*plugins[moduleID].getColorPixelSize!=0) { return  (*plugins[moduleID].getColorPixelSize) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acqusitionGetColorPixelSize");
    return 0.0;
}



double acqusitionGetDepthFocalLength(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acqusitionGetFocalLength");
    if  (*plugins[moduleID].getDepthFocalLength!=0) { return  (*plugins[moduleID].getDepthFocalLength) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acqusitionGetFocalLength");
    return 0.0;
}

double acqusitionGetDepthPixelSize(ModuleIdentifier moduleID,DeviceIdentifier devID)
{
    printCall(moduleID,devID,"acqusitionGetPixelSize");
    if  (*plugins[moduleID].getDepthPixelSize!=0) { return  (*plugins[moduleID].getDepthPixelSize) (devID); }
    MeaningfullWarningMessage(moduleID,devID,"acqusitionGetPixelSize");
    return 0.0;
}

