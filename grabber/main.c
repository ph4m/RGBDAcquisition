#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../acquisition/Acquisition.h"
#include "../tools/Calibration/calibration.h"

char inputname[512]={0};
char outputfoldername[512]={0};

unsigned int delay = 0;

int calibrationSet = 0;
struct calibration calib;




int main(int argc, char *argv[])
{
 fprintf(stderr,"Generic Grabber Application based on Acquisition lib .. \n");
 unsigned int possibleModules = acquisitionGetModulesCount();
 fprintf(stderr,"Linked to %u modules.. \n",possibleModules);


  ModuleIdentifier moduleID = OPENGL_ACQUISITION_MODULE;//OPENNI1_ACQUISITION_MODULE;//

  if (possibleModules==0)
    {
       fprintf(stderr,"Acquisition Library is linked to zero modules , can't possibly do anything..\n");
       return 1;
    }

  unsigned int width=640,height=480,framerate=30;
  unsigned int frameNum=0,maxFramesToGrab=10;
  int i=0;
  for (i=0; i<argc; i++)
  {
    if (strcmp(argv[i],"-delay")==0) {
                                       delay=atoi(argv[i+1]);
                                       fprintf(stderr,"Delay set to %u seconds \n",delay);
                                      } else
    if (strcmp(argv[i],"-resolution")==0) {
                                             width=atoi(argv[i+1]);
                                             height=atoi(argv[i+2]);
                                             fprintf(stderr,"Resolution set to %u x %u \n",width,height);
                                           } else
    if (strcmp(argv[i],"-calibration")==0) {
                                             calibrationSet=1;
                                             if (!ReadCalibration(argv[i+1],width,height,&calib) )
                                             {
                                               fprintf(stderr,"Could not read calibration file `%s`\n",argv[i+1]);
                                               return 1;
                                             }
                                           } else
    if (strcmp(argv[i],"-maxFrames")==0) {
                                           maxFramesToGrab=atoi(argv[i+1]);
                                           fprintf(stderr,"Setting frame grab to %u \n",maxFramesToGrab);
                                         } else
    if (strcmp(argv[i],"-module")==0)    {
                                           moduleID = getModuleIdFromModuleName(argv[i+1]);
                                           fprintf(stderr,"Overriding Module Used , set to %s ( %u ) \n",getModuleStringName(moduleID),moduleID);
                                         } else
    if (
         (strcmp(argv[i],"-to")==0) ||
         (strcmp(argv[i],"-o")==0)
        )
        {
          strcpy(outputfoldername,"frames/");
          strcat(outputfoldername,argv[i+1]);
          fprintf(stderr,"OutputPath , set to %s  \n",outputfoldername);
         }
       else
    if (
        (strcmp(argv[i],"-from")==0) ||
        (strcmp(argv[i],"-i")==0)
       )
       { strcat(inputname,argv[i+1]); fprintf(stderr,"Input , set to %s  \n",inputname); }
      else
    if (strcmp(argv[i],"-fps")==0)       {
                                             framerate=atoi(argv[i+1]);
                                             fprintf(stderr,"Framerate , set to %u  \n",framerate);
                                         }
  }


  if (!acquisitionIsModuleLinked(moduleID))
   {
       fprintf(stderr,"The module you are trying to use is not linked in this build of the Acquisition library..\n");
       return 1;
   }

  //We need to initialize our module before calling any related calls to the specific module..
  if (!acquisitionStartModule(moduleID,16 /*maxDevices*/ , 0 ))
  {
       fprintf(stderr,"Could not start module %s ..\n",getModuleStringName(moduleID));
       return 1;
   }

  //We want to initialize all possible devices in this example..
  unsigned int devID=0,maxDevID=acquisitionGetModuleDevices(moduleID);
  if (maxDevID==0)
  {
      fprintf(stderr,"No devices found for Module used \n");
      return 1;
  }

   if ( calibrationSet )
   {
    fprintf(stderr,"Set Far/Near to %f/%f\n",calib.farPlane,calib.nearPlane);
    acquisitionSetColorCalibration(moduleID,devID,&calib);
    acquisitionSetDepthCalibration(moduleID,devID,&calib);
   }



  char * devName = inputname;
  if (strlen(inputname)<1) { devName=0; }
    //Initialize Every OpenNI Device
    for (devID=0; devID<maxDevID; devID++)
     {
        /*The first argument (Dev ID) could also be ANY_OPENNI2_DEVICE for a single camera setup */
        acquisitionOpenDevice(moduleID,devID,devName,width,height,framerate);

        if ( strstr(inputname,"noRegistration")!=0 )         {  } else
        if ( strstr(inputname,"rgbToDepthRegistration")!=0 ) { acquisitionMapRGBToDepth(moduleID,devID); } else
                                                             { acquisitionMapDepthToRGB(moduleID,devID);  }

         //We initialize the target for our frames ( can be network or files )
         if (! acquisitionInitiateTargetForFrames(moduleID,devID,outputfoldername) )
         {
           return 1;
         }
     }


   countdownDelay(delay);
   fprintf(stderr,"Starting Grabbing!\n");


   if ( maxFramesToGrab==0 ) { maxFramesToGrab= 1294967295; } //set maxFramesToGrab to "infinite" :P
   for (frameNum=0; frameNum<maxFramesToGrab; frameNum++)
    {

     for (devID=0; devID<maxDevID; devID++)
      {
        acquisitionStartTimer(0);

        acquisitionSnapFrames(moduleID,devID);

        acquisitionPassFramesToTarget(moduleID,devID,frameNum);

        acquisitionStopTimer(0);
        if (frameNum%25==0) fprintf(stderr,"%0.2f fps\n",acquisitionGetTimerFPS(0));

        /*
         If someone would like to manually save things instead of using acquisitionPassFramesToTarget he could call the following calls ,left here for future reference
         ------------------------------------------------------------------------------------------------------------------------------
         char outfilename[1024]={0};

         sprintf(outfilename,"%s/colorFrame_%u_%05u",outputfoldername,devID,frameNum);
         acquisitionSaveColorFrame(moduleID,devID,outfilename);

         sprintf(outfilename,"%s/depthFrame_%u_%05u",outputfoldername,devID,frameNum);
         acquisitionSaveDepthFrame(moduleID,devID,outfilename);

         sprintf(outfilename,"%s/pointCloud_%u_%05u.pcd",outputfoldername,devID,frameNum);
         acquisitionSavePCDPointCoud(moduleID,devID,outfilename);

         sprintf(outfilename,"%s/depthFrame1C_%u_%05u.pnm",outputfoldername,devID,frameNum);
         acquisitionSaveDepthFrame1C(moduleID,devID,outfilename);

         sprintf(outfilename,"%s/coloreddepthFrame_%u_%05u.pnm",outputfoldername,devID,frameNum);
         acquisitionSaveColoredDepthFrame(moduleID,devID,outfilename);
        */
      }
    }

    fprintf(stderr,"Done grabbing %u frames! \n",maxFramesToGrab);

    for (devID=0; devID<maxDevID; devID++)
     {
        //Stop our target ( can be network or files )
        acquisitionStopTargetForFrames(moduleID,devID);
        /*The first argument (Dev ID) could also be ANY_OPENNI2_DEVICE for a single camera setup */
        acquisitionCloseDevice(moduleID,devID);
     }

    acquisitionStopModule(moduleID);

    return 0;
}
