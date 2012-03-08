#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#ifdef _WIN32
#include <windows.h>

#include "AR/gsub.h"
#include "AR/video.h"
#include "AR/param.h"
#include "AR/ar.h"
#include "AR/arMulti.h"

#include "GL/gl.h"
#include "GL/glut.h"

#include "SDL/SDL_mixer.h"
#include "SDL/SDL_image.h"
#endif
#if __APPLE__
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

#include <SDL/SDL_mixer.h>

#endif

#include <iostream>

//Variables
const int				m_thresh = 50;
int						m_Xsize, m_Ysize;
ARMultiMarkerInfoT *	m_config;
GLuint*					m_texturesIds;
int						m_nbImages;
enum Marker {C, B, SR, SL, FR, BR, FL, BL};

//Functions
void cleanUp();
void init(int argc, char **argv);
void run();
void update();
void render();

void drawObject(int obj_id, double gl_para[16]);
void drawText(float x, float y, float z, void* font, const char* s);
void drawImage(int id, float x, float y, float z);

void keyEvent(unsigned char key, int x, int y);
void loadImage(const char * filename, int id);

//Functions definitions
void cleanUp()
{
	arVideoCapStop();
    arVideoClose();
    argCleanup();
}

void init(int argc, char **argv)
{
	//Glut init
	glutInit(&argc, argv);
	
	#ifdef _WIN32
		char *vconf = "Data\\WDM_camera_flipV.xml";
	#else
		char *vconf = "";
	#endif

	//Init textures
	m_nbImages = 1;
	m_texturesIds = new GLuint[m_nbImages];
	loadImage("../images/test.jpg", 0);

	ARParam  wparam;
    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&m_Xsize, &m_Ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", m_Xsize, m_Ysize);

    /* set the initial camera parameters */
	char *cparam_name = "Data/camera_para.dat";
	if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }
	ARParam cparam;
    arParamChangeSize( &wparam, m_Xsize, m_Ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

	// New way multiple patterns
	char *config_name = "Data/multi/marker.dat";
	if( (m_config = arMultiReadConfigFile(config_name)) == NULL ) {
        printf("config data load error !!\n");
        exit(0);
    }

    /* open the graphics window */
    argInit( &cparam, 1.0, 0.0, 0, 0, 0 );
}

void run()
{
	arVideoCapStart();

    argMainLoop( NULL, keyEvent, update );
}

void update()
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

    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, m_thresh, &marker_info, &marker_num) < 0 ) {
		exit(0);
    }

   
	/* check for object visibility */
	// NEW WAY
 	for( i = 0; i < m_config->marker_num; i++ ) {
		k = -1;
		for( j = 0; j < marker_num; j++ ) {
			if( m_config->marker[i].patt_id == marker_info[j].id ) {
				/* you've found a pattern */
				printf("Found pattern: %d ", m_config->marker[i].patt_id);
				switch(m_config->marker[i].patt_id){
					case C: printf("Chest"); break;
					case B: printf("Back"); break;
					case SR: printf("Shoulder Right"); break;
					case SL: printf("Shoulder Left"); break;
					case FR: printf("Hand Right Front"); break;
					case BR: printf("Hand Right Back"); break;
					case FL: printf("Hand Left Front"); break;
					case BL: printf("Hand Left Back"); break;
				};
				printf("\n");
				glColor3f( 0.0, 1.0, 0.0 );
				argDrawSquare(marker_info[j].vertex,0,0);
				if( k == -1 ) k = j;
				else if( marker_info[k].cf < marker_info[j].cf ) k = j;
			}
		}

		if( k == -1 ) {
			m_config->marker[i].visible = 0;
			continue;
		}

		/* calculate the transform for each marker */
		if( m_config->marker[i].visible == 0 ) {
            arGetTransMat(&marker_info[k],
                          m_config->marker[i].center, m_config->marker[i].width,
                          m_config->marker[i].trans);
        }
        else {
            arGetTransMatCont(&marker_info[k], m_config->marker[i].trans,
				m_config->marker[i].center, m_config->marker[i].width,
                          m_config->marker[i].trans);
        }
        m_config->marker[i].visible = 1;
		
	}

	arVideoCapNext();
    render();
	drawText(m_Xsize, m_Ysize, 100, GLUT_BITMAP_TIMES_ROMAN_24, "HEllo!");
	//drawImage(0, m_Xsize/2, m_Ysize/2, 30);

    argSwapBuffers();
}

void render()
{
	int     i;
    double  gl_para[16];
       
	glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_LIGHTING);
	glPushMatrix();

    /* calculate the viewing parameters - gl_para */
	for( i = 0; i < m_config->marker_num; i++ ) {
        if( m_config->marker[i].visible == 0 ) continue;
        argConvGlpara(m_config->marker[i].trans, gl_para);
		drawObject( m_config->marker[i].patt_id, gl_para);
    }
    glPopMatrix();
	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}

void drawObject(int obj_id, double gl_para[16])
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
		glTranslatef( 0.0, 0.0, 1 );
		glutSolidCube(120);
	}

    argDrawMode2D();
}

void drawText(float x, float y, float z, void* font, const char* s)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

	glColor4f(1.f,1.f,1.f, 1.f);
    glRasterPos2f(x, y);

    while(*s)
    {
        glutBitmapCharacter(font, *s);
        s++;
    }
}

void drawImage(int id, float x, float y, float z)
{
	if(id < m_nbImages)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, m_texturesIds[id]);
		glTranslatef( x, y, z);
		//glColor4f(1.f,1.f,1.f, 1.f);
		glutSolidCube(60);
	}
}

void keyEvent(unsigned char key, int x, int y)
{
	if (key == 0x1b) 
	{
		exit(0);
	}
}

void loadImage(const char * filename, int id)
{
    SDL_Surface * picture_surface = NULL;

    picture_surface = IMG_Load(filename);
    if (picture_surface == NULL)
        std::cout<<"ERRREUREURUERUE"<<std::endl;

	if(id < m_nbImages)
	{
		glGenTextures(1, &m_texturesIds[id]);
		glBindTexture(GL_TEXTURE_2D, m_texturesIds[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, picture_surface->w, picture_surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, picture_surface->pixels);
	}

	SDL_FreeSurface(picture_surface);
}

#endif //__APPLICATION_HPP__