#include <GL/gl.h>
#include <GL/glx.h>    /* this includes the necessary X headers */

#include <stdio.h>
#include <stdlib.h>

#include "tiledRenderer.h"

#include "TrajectoryParser/TrajectoryParser.h"
#include "model_loader.h"
#include "scene.h"

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

struct VirtualStream * scene = 0;
struct Model ** models=0;

float farPlane = 255; //<--be aware that this has an effect on the depth maps generated
float nearPlane= 1; //<--this also

float depthUnit = 1;

int WIDTH=640;
int HEIGHT=480;

int framesRendered =0 ;



int useIntrinsicMatrix=0;
double cameraMatrix[9]={
                        0.0 , 0.0 , 0.0 ,
                        0.0 , 0.0 , 0.0 ,
                        0.0 , 0.0 , 0.0
                       };


double customProjectionMatrix[16]={0};




int useCustomModelViewMatrix=0;
double customModelViewMatrix[16]={
                                   1.0 , 0.0 , 0.0 , 0.0 ,
                                   0.0 , 1.0 , 0.0 , 0.0 ,
                                   0.0 , 0.0 , 1.0 , 0.0 ,
                                   0.0 , 0.0 , 0.0 , 1.0
                                 };
double customTranslation[3]={0};
double customRodriguezRotation[3]={0};



const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 0.1f, 0.1f, 0.1f, 1.0f };
      GLfloat mat_shininess = 5.0f;


float camera_pos_x = 0.0f; float camera_pos_y = 0.0f; float camera_pos_z = 8.0f;
float camera_angle_x = 0.0f; float camera_angle_y = 0.0f; float camera_angle_z = 0.0f;

unsigned int ticks = 0;




int updateProjectionMatrix()
{
  fprintf(stderr,"updateProjectionMatrix activated \n");

  if ( scene->projectionMatrixDeclared )
  { //Scene configuration overwrites local configuration
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd( scene->projectionMatrix ); // we load a matrix of Doubles
    glViewport(0,0,WIDTH,HEIGHT);

   print4x4DMatrix("OpenGL Projection Matrix Given by Trajectory Parser", scene->projectionMatrix );

  } else
  if (useIntrinsicMatrix)
  {
   int viewport[4]={0};

   fprintf(stderr,"Width %u x Height %u \n",WIDTH,HEIGHT);
   buildOpenGLProjectionForIntrinsics   (
                                             customProjectionMatrix  ,
                                             viewport ,
                                             cameraMatrix[0],
                                             cameraMatrix[4],
                                             0.0,
                                             cameraMatrix[2],
                                             cameraMatrix[5],
                                             WIDTH,
                                             HEIGHT,
                                             nearPlane ,
                                             farPlane
                                           );

   print4x4DMatrix("OpenGL Projection Matrix", customProjectionMatrix );

   glMatrixMode(GL_PROJECTION);
   glLoadMatrixd(customProjectionMatrix); // we load a matrix of Doubles
   glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
  }
    else
  {
  glLoadIdentity();
  glFrustum(-1.0, 1.0, -1.0, 1.0, nearPlane , farPlane);
  glViewport(0, 0, WIDTH, HEIGHT);
 }
}









int initScene(char * confFile)
{
  fprintf(stderr,"Initializing Scene\n");
  scene = createVirtualStream(confFile);
  fprintf(stderr,"createVirtualStream returned \n");
  if (scene==0) { fprintf(stderr,RED "Could not read scene data \n" NORMAL); return 0; }

  glEnable(GL_DEPTH_TEST); /* enable depth buffering */
  glDepthFunc(GL_LESS);    /* pedantic, GL_LESS is the default */
  glDepthMask(GL_TRUE);
  glClearDepth(1.0);       /* pedantic, 1.0 is the default */

  //HQ settings
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  /* frame buffer clears should be to black */
  glClearColor(0.0, 0.0, 0.0, 0.0);

  /* set up projection transform */
  glMatrixMode(GL_PROJECTION);

  updateProjectionMatrix();

  /* establish initial viewport */
  /* pedantic, full window size is default viewport */

  glEnable(GL_COLOR);
  glEnable(GL_COLOR_MATERIAL);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,    mat_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,    mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,   mat_specular);
  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS,   mat_shininess); // <- this was glMateriali



  models = (struct Model **) malloc(scene->numberOfObjectTypes * sizeof(struct Model **));

  unsigned int i=0;  //Object 0 is camera so we don't need to load a model or something for it
  for (i=1; i<scene->numberOfObjectTypes; i++)
    {
         fprintf(stderr,"Loading Model %s ( %u )\n",scene->object[i].name,i);
         models[i] = loadModel("Models/",getObjectTypeModel(scene,i));
         if (models[i]!=0) {  fprintf(stderr,GREEN "Model %s , is now loaded as model[%u] \n" NORMAL,getObjectTypeModel(scene,i) ,i ); } else
                           {  fprintf(stderr,RED "Failed loading model %s ( %u ) \n" NORMAL,getObjectTypeModel(scene,i),i);          }

    }

  return 1;
}


int closeScene()
{
  unsigned int i=0;
  //Object 0 is camera
  for (i=1; i<scene->numberOfObjectTypes; i++)
    {
       unloadModel(models[i]);
    }
  free(models);

  destroyVirtualStream(scene);

  return 1;
}



int tickScene()
{
   //ALL positions should be calculated here!
   //i dont like the way this is working now
   float x,y,z,heading,pitch,roll;
   float posStack[7];
   float * pos = (float*) &posStack;

   unsigned int i=0;
  //Object 0 is camera  lets calculate its position
   calculateVirtualStreamPos(scene,0,ticks*100,pos);
   camera_pos_x = pos[0];  camera_pos_y = pos[1]; camera_pos_z = pos[2];
   camera_angle_x = pos[3]; camera_angle_y = pos[4]; camera_angle_z = pos[5];

   usleep(100);
   ++ticks;
   return 1;
}





int print3DPoint2DWindowPosition(int objID , float x3D , float y3D , float z3D)
{
      GLint viewport[4];
      GLdouble modelview[16];
      GLdouble projection[16];

      GLdouble posX = x3D , posY = y3D , posZ = z3D;
      GLdouble winX, winY, winZ=0.0;

      glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
      glGetDoublev( GL_PROJECTION_MATRIX, projection );
      glGetIntegerv( GL_VIEWPORT, viewport );

      gluProject( posX, posY, posZ , modelview, projection, viewport, &winX, &winY, &winZ);

      if  (
            (winX < 0) || (winX >= WIDTH) ||
            (winY < 0) || (winY >= HEIGHT)
          )
      {
         fprintf(stderr,"Warn : Object %u offscreen ( %0.2f , %0.2f , %0.2f ) will end up at %0.2f,%0.2f(%0.2f)\n" , objID , x3D , y3D , z3D , winX,winY , winZ);
      }

}


int drawAllObjectsAtPositionsFromTrajectoryParser()
{
 if (scene==0) { return 0; }

  unsigned char noColor=0;
  float posStack[7]={0};
  float R=1.0f , G=1.0f ,  B=0.0f , trans=0.0f;
  unsigned int i;
  //Object 0 is camera , so we draw object 1 To numberOfObjects-1
  for (i=1; i<scene->numberOfObjects; i++)
    {
       struct Model * mod = models[scene->object[i].type];
       float * pos = (float*) &posStack;
       if ( calculateVirtualStreamPos(scene,i,ticks*100,pos) )
       {
         //This is a stupid way of passing stuff to be drawn
         R=1.0f; G=1.0f;  B=1.0f; trans=0.0f; noColor=0;
         getObjectColorsTrans(scene,i,&R,&G,&B,&trans,&noColor);
         //fprintf(stderr,"Object %s should be RGB(%0.2f,%0.2f,%0.2f) , Transparency %0.2f , ColorDisabled %u\n",scene->object[i].name,R,G,B,trans,noColor);
         setModelColor(mod,&R,&G,&B,&trans,&noColor);
         mod->scale = scene->object[i].scale;
         //fprintf(stderr,"Model %s is now RGB(%0.2f,%0.2f,%0.2f) , Transparency %0.2f , ColorDisabled %u\n",scene->object[i].name, mod->colorR, mod->colorG, mod->colorB, mod->transparency,mod->nocolor );

         if (scene->debug)
                { print3DPoint2DWindowPosition(i , pos[0],pos[1],pos[2] ); }

         if (! drawModelAt(mod,pos[0],pos[1],pos[2],pos[3],pos[4],pos[5]) )
             { fprintf(stderr,RED "Could not draw object %u , type %u \n" NORMAL ,i , scene->object[i].type ); }
       } else
       { fprintf(stderr,YELLOW "Could not determine position of object %s (%u) , so not drawing it\n" NORMAL,scene->object[i].name,i); }
    }
  return 1;
}







int renderScene()
{

  if (scene!=0) { glClearColor(scene->backgroundR,scene->backgroundG,scene->backgroundB,0.0); } else
                { glClearColor(0.0,0.0,0.0,0.0); }

  glEnable (GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW );


  if ( (scene!=0) && ( scene->modelViewMatrixDeclared ) )
  { //Scene configuration overwrites local configuration
   glLoadMatrixd( scene->modelViewMatrix ); // we load a matrix of Doubles
   //print4x4DMatrix("OpenGL ModelView Matrix Given by Trajectory Parser", scene->modelViewMatrix );
  } else
  //If setOpenGLExtrinsicCalibration has set a custom MODELVIEW matrix we will use it
  if (useCustomModelViewMatrix)
  {
    //We load the matrix produced by convertRodriguezAndTranslationToOpenGL4x4DMatrix
    glLoadMatrixd((const GLdouble*) customModelViewMatrix);
    // We flip our coordinate system so it comes straight
    glRotatef(90,-1.0,0,0);
  } else
  // we create a modelview matrix on the fly by using the camera declared in trajectory parser
  {
    glLoadIdentity();
    glRotatef(camera_angle_x,-1.0,0,0); // Peristrofi gyrw apo ton x
    glRotatef(camera_angle_y,0,-1.0,0); // Peristrofi gyrw apo ton y
    glRotatef(camera_angle_z,0,0,-1.0);
    glTranslatef(-camera_pos_x, -camera_pos_y, -camera_pos_z);
  }

  drawAllObjectsAtPositionsFromTrajectoryParser();


  ++framesRendered;

 return 1;
}







int setupPhotoshoot(
                        void * context,
                        int objID,
                        unsigned int columns , unsigned int rows ,
                        float distance,
                        float angleX,float angleY,float angleZ ,
                        float angXVariance ,float angYVariance , float angZVariance
                       )
{

  struct tiledRendererConfiguration * configuration = (struct tiledRendererConfiguration *) context;

  configuration->columns=columns;
  configuration->rows=rows;
  configuration->objID=objID;
  configuration->distance=distance;
  configuration->angleX=angleX;
  configuration->angleY=angleY;
  configuration->angleZ=angleZ;
  configuration->angXVariance=angXVariance;
  configuration->angYVariance=angYVariance;
  configuration->angZVariance=angZVariance;

  configuration->scenePTR = (void *) scene;
  configuration->modelPTR = (void *) models;
  return 1;
}

void * createPhotoshoot(
                        int objID,
                        unsigned int columns , unsigned int rows ,
                        float distance,
                        float angleX,float angleY,float angleZ ,
                        float angXVariance ,float angYVariance , float angZVariance
                       )
{

  struct tiledRendererConfiguration * configuration = 0;

  configuration = (struct tiledRendererConfiguration * ) malloc(sizeof( struct tiledRendererConfiguration));
  if (configuration == 0) { fprintf(stderr,"Could not allocate a configuration structure\n"); return 0; }


  configuration->columns=columns;
  configuration->rows=rows;
  configuration->objID=objID;
  configuration->distance=distance;
  configuration->angleX=angleX;
  configuration->angleY=angleY;
  configuration->angleZ=angleZ;
  configuration->angXVariance=angXVariance;
  configuration->angYVariance=angYVariance;
  configuration->angZVariance=angZVariance;

  configuration->scenePTR = (void *) scene;
  configuration->modelPTR = (void *) models;



  return (void*) configuration;

}



int renderPhotoshoot( void * context  )
{
  struct tiledRendererConfiguration * configuration=context;

  fprintf(stderr," renderPhotoshoot Rows/Cols %u/%u  Distance %0.2f , Angles %0.2f %0.2f %0.2f\n",configuration->rows,configuration->columns,configuration->distance,configuration->angleX,configuration->angleY,configuration->angleZ);
  fprintf(stderr,"Angle Variance %0.2f %0.2f %0.2f\n",configuration->angXVariance,configuration->angYVariance,configuration->angZVariance);


  int i= tiledRenderer_Render(configuration);

  if (i) { framesRendered++; return 1; }
  return 0;
}






