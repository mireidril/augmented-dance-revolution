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
#include "time.h"

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

// ================================= VARIABLES ===========================================

const int				m_thresh = 50;
int						m_Xsize, m_Ysize;
ARMultiMarkerInfoT *	m_config;
GLuint*					m_texturesIds;
GLuint					m_text1;
int						m_nbImages;
enum Marker {C, B, SR, SL, FR, BR, FL, BL};
typedef struct  {
	unsigned int  patt1;
	unsigned int  patt2;
	unsigned int  patt3;
	unsigned int  patt4;
	unsigned int  patt5;
	unsigned int  patt6;
	unsigned int  patt7;
	unsigned int  patt8;
	
	}position;

position pos[123*4];
unsigned int bar, beat;
clock_t			start, end;
double			elapsed;
const double BPM_96 =  625;
const double BPM_156 = 384;
double deltaTime;

// ================================= FONCTIONS ===========================================
//Initialise la fenêtre Glut et les paramètres ARToolkit
void init(int argc, char **argv);
//Charge toutes les images de l'application
void initImages();
//Charge une image et lui attribue un identifiant de texture stocké dans m_texturesIds[id]
void loadImage(const char * filename, int id);

//Définit la boucle principale de l'application
void run();
//Gère les opérations de l'application
void update();
//Gère le rendu de l'application
void render();

//Détruit et désalloue toutes les ressources de l'application
void cleanUp();

//Dessine un objet 3D selon la matrice de vue passée en paramètre
void drawObject(int obj_id, double gl_para[16]);
//Dessine le texte s à la position (x, y, z) de la fenêtre
void drawText(float x, float y, float z, void* font, const char* s);
//Dessine l'image id à la position (x, y, z) de la fenêtre
void drawImage(int id, float x, float y, float z, int size);

//Gère les évènements clavier de l'application
void keyEvent(unsigned char key, int x, int y);

//=======================================================================================

void init(int argc, char **argv)
{
	//Glut init
	glutInit(&argc, argv);
	
	#ifdef _WIN32
		char *vconf = "Data\\WDM_camera_flipV.xml";
	#else
		char *vconf = "";
	#endif

	ARParam  wparam;
    // open the video path
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    // find the size of the window
    if( arVideoInqSize(&m_Xsize, &m_Ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", m_Xsize, m_Ysize);

    // set the initial camera parameters
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

    // open the graphics window
    argInit( &cparam, 2.0, 0.0, 0, 0, 0 );

	//sound init
	bar = 0;
	beat = 0;
	deltaTime = BPM_96;
	
	//Init SDL_mixer
	if( Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
		std::cout<<"problem init son"<<std::endl; //Initialisation de l'API Mixer
	Mix_Music * musique = Mix_LoadMUS("../musics/queen.ogg");
	Mix_VolumeMusic(MIX_MAX_VOLUME/2);
	if(musique == NULL){
		std::cout<<"musique non jouée"<<std::endl;
	}
	else
	{
		//Mix_PlayMusic(musique, -1);
	}

	start = clock();
}

void initImages()
{
	m_nbImages = 1;
	m_texturesIds = new GLuint[m_nbImages];
	loadImage("../images/test.jpg", 0);
}

void loadImage(const char * filename, int id)
{
    SDL_Surface * picture_surface = NULL;

    picture_surface = IMG_Load(filename);
    if (picture_surface == NULL)
        std::cout<<"Image "<<filename<<" non chargée !!"<<std::endl;

	//Retournement surface car standard différents entre OpenGL et SDL
	int current_line,pitch;
    SDL_Surface * fliped_surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                   picture_surface->w, picture_surface->h,
                                   picture_surface->format->BitsPerPixel,
                                   picture_surface->format->Rmask,
                                   picture_surface->format->Gmask,
                                   picture_surface->format->Bmask,
                                   picture_surface->format->Amask);


    SDL_LockSurface(picture_surface);
    SDL_LockSurface(fliped_surface);

    pitch = picture_surface->pitch;
    for (current_line = 0; current_line < picture_surface->h; current_line ++)
    {
        memcpy(&((unsigned char* )fliped_surface->pixels)[current_line*pitch],
               &((unsigned char* )picture_surface->pixels)[(picture_surface->h - 1  -
                                                    current_line)*pitch],
               pitch);
    }

    SDL_UnlockSurface(fliped_surface);
    SDL_UnlockSurface(picture_surface);
	SDL_FreeSurface(picture_surface);
    picture_surface = fliped_surface;

	if(id < m_nbImages)
	{
		glGenTextures(1, &m_texturesIds[id]);
		glBindTexture(GL_TEXTURE_2D, m_texturesIds[id]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, picture_surface->w, picture_surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, picture_surface->pixels);
	}

	SDL_FreeSurface(picture_surface);
}

void run()
{
	arVideoCapStart();

	//Charge les images
	initImages();
	
    argMainLoop( NULL, keyEvent, update );
}

void update()
{
	end = clock();
	elapsed = ((double)end - start);

	if(elapsed >= deltaTime){
		 
	beat++;
	if(beat == 5){
		bar++;
		beat=1;
	}

	if(bar == 13) deltaTime = BPM_156;

		std::cout << "mesure " << bar;
		std::cout << "beat " << beat << std::endl;
		start = end;
	}

	ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i, j, k;

    // grab a vide frame
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }

	//mirror the image camera
	int sizeX, sizeY;
	arVideoInqSize(&sizeX, &sizeY);
	ARUint8 * dataPtrFlipped = new ARUint8[sizeX*sizeY*AR_PIX_SIZE_DEFAULT];
	
	for(int i = 0; i < sizeY; ++i)
	{
		for(int j = 0, k = (sizeX * AR_PIX_SIZE_DEFAULT) - AR_PIX_SIZE_DEFAULT; j < sizeX * AR_PIX_SIZE_DEFAULT; j += AR_PIX_SIZE_DEFAULT, k-=AR_PIX_SIZE_DEFAULT)
		{
			dataPtrFlipped[i*sizeX * AR_PIX_SIZE_DEFAULT + j + 0] = dataPtr[i*sizeX * AR_PIX_SIZE_DEFAULT + k + 0];
			dataPtrFlipped[i*sizeX * AR_PIX_SIZE_DEFAULT + j + 1] = dataPtr[i*sizeX * AR_PIX_SIZE_DEFAULT + k + 1];
			dataPtrFlipped[i*sizeX * AR_PIX_SIZE_DEFAULT + j + 2] = dataPtr[i*sizeX * AR_PIX_SIZE_DEFAULT + k + 2];
			dataPtrFlipped[i*sizeX * AR_PIX_SIZE_DEFAULT + j + 3] = dataPtr[i*sizeX * AR_PIX_SIZE_DEFAULT + k + 3];
		}
	}

    argDrawMode2D();
    argDispImage( dataPtrFlipped, 0, 0 );

    // detect the markers in the video frame 
    if( arDetectMarker(dataPtrFlipped, m_thresh, &marker_info, &marker_num) < 0 ) {
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

    argSwapBuffers();
	delete []dataPtrFlipped;
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

	drawText(m_Xsize, m_Ysize, 100, GLUT_BITMAP_TIMES_ROMAN_24, "HEllo!");
	//drawImage(0, 600, 500, 0, 200);
}


void cleanUp()
{
	arVideoCapStop();
    arVideoClose();
    argCleanup();
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
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, m_text1);
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

	glColor4f(1.f,0.f,1.f, 1.f);
	glScalef( 10, 10, 10 );
    glRasterPos2f(x, y);

    while(*s)
    {
        glutBitmapCharacter(font, *s);
        s++;
    }
}

void drawImage(int id, float x, float y, float z, int size)
{
	if(id < m_nbImages)
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
		glBindTexture(GL_TEXTURE_2D, m_texturesIds[id]);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTranslatef(x + size/2, y + size/2, z);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 0);
			glVertex3i(size/2, -size/2,-1);
			glTexCoord2i(0,1);
			glVertex3i(size/2,size/2,-1);
			glTexCoord2i(1,1);
			glVertex3i(-size/2,size/2,-1);
			glTexCoord2i(1,0);
			glVertex3i(-size/2,-size/2,-1);
		glEnd();
	}
}

void keyEvent(unsigned char key, int x, int y)
{
	//ECHAP
	if (key == 0x1b) 
	{
		exit(0);
	}
}



#endif //__APPLICATION_HPP__