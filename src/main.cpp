#ifdef _WIN32
#include <windows.h>

#include "AR/gsub.h"
#include "AR/video.h"
#include "AR/param.h"
#include "AR/ar.h"
#include "AR/arMulti.h"

#include "GL/gl.h"
#include "GL/glut.h"
#else if __APPLE__
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>

#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

//
// Camera configuration.
//
#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

int             xsize, ysize;
int             thresh = 100;
int             count = 0;

char           *cparam_name    = "Data/camera_para.dat";
ARParam         cparam;

char				*config_name = "Data/multi/marker.dat";
ARMultiMarkerInfoT  *config;

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw();
static int  draw_object( int obj_id, double gl_para[16]);

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	init();

    arVideoCapStart();
    argMainLoop( NULL, keyEvent, mainLoop );
	system("PAUSE");
	return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i, j, k;

    /* grab a vide frame */
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }
    if( count == 0 ) arUtilTimerReset();
    count++;

    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
		system("PAUSE");
        exit(0);
    }

   
	/* check for object visibility */
	// NEW WAY
	for( i = 0; i < config->marker_num; i++ ) {
		k = -1;
		for( j = 0; j < marker_num; j++ ) {
			if( config->marker[i].patt_id == marker_info[j].id ) {
				/* you've found a pattern */
				//printf("Found pattern: %d ",config->marker[i].patt_id);
				glColor3f( 0.0, 1.0, 0.0 );
				argDrawSquare(marker_info[j].vertex,0,0);
				if( k == -1 ) k = j;
				else if( marker_info[k].cf < marker_info[j].cf ) k = j;
			}
		}

		if( k == -1 ) {
			config->marker[i].visible = 0;
			continue;
		}

		/* calculate the transform for each marker */
		if( config->marker[i].visible == 0 ) {
            arGetTransMat(&marker_info[k],
                          config->marker[i].center, config->marker[i].width,
                          config->marker[i].trans);
        }
        else {
            arGetTransMatCont(&marker_info[k], config->marker[i].trans,
				config->marker[i].center, config->marker[i].width,
                          config->marker[i].trans);
        }
        config->marker[i].visible = 1;
		
	}

	arVideoCapNext();
    draw();

    argSwapBuffers();
}

static void init( void )
{
    ARParam  wparam;
	
    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }
    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

	// New way multiple patterns
	if( (config = arMultiReadConfigFile(config_name)) == NULL ) {
        printf("config data load error !!\n");
		system("PAUSE");
        exit(0);
    }

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

/* draw the the AR objects */
static void draw()
{
    int     i;
    double  gl_para[16];
       
	glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);

    /* calculate the viewing parameters - gl_para */
	for( i = 0; i < config->marker_num; i++ ) {
        if( config->marker[i].visible == 0 ) continue;
        argConvGlpara(config->marker[i].trans, gl_para);
		draw_object( config->marker[i].patt_id, gl_para);
    }
     
	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
	
}

/* draw the user object */
static int  draw_object( int obj_id, double gl_para[16])
{
    GLfloat   mat_ambient[]				= {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_ambient_collide[]     = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]				= {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash_collide[]       = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
 
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

 	/* set the material */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);

    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	

	if(obj_id == 0){
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash_collide);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient_collide);
		/* draw a cube */
		glTranslatef( 0.0, 0.0, 30.0 );
		glutSolidSphere(30,12,6);
	}
	else {
		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		/* draw a cube */
		glTranslatef( 0.0, 0.0, 30.0 );
		glutSolidCube(60);
	}

    argDrawMode2D();

    return 0;
}