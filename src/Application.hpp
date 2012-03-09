#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#ifdef _WIN32
#include <windows.h>

#include "AR/gsub.h"
#include "AR/video.h"
#include "AR/param.h"
#include "AR/ar.h"
#include "AR/arMulti.h"

#include "time.h"

#include "SDL/SDL_mixer.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_rotozoom.h"
#include "SDL/SDL_ttf.h"
#else
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>

#include <time.h>

#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <SDL/SDL_ttf.h>

#endif

#include <iostream>
#include <vector>

enum Marker {C, B, SR, SL, FR, BR, FL, BL};

typedef struct image
{
	SDL_Surface * m_image;
	SDL_Rect * m_pos;
	SDL_Rect * m_size;
	SDL_Rect * m_drawingSize;
} Image;

class Application
{
	public :
		Application();
		~Application();

		//Initialise la fenêtre SDL et les paramètres ARToolkit
		void init();
		//Charge toutes les images de l'application
		void initImages();
		//Charge une image et la stocke dans m_images
		void loadImage(const char * filename, int posX, int posY, int sizeX, int sizeY, int drawingSizeX, int drawingSizeY);

		//Définit la boucle principale de l'application
		void run();
		//Gère les opérations de l'application
		void update();
		//Gère le rendu de l'application
		void render();

		//Dessine une image à la position du marqueur idMarker
		void drawObject(int idMarker);
		//Dessine le texte s à la position (x, y) de la fenêtre
		void drawText(int x, int y, SDL_Color color, char* s);
		//Dessine l'image id à la position (x, y) de la fenêtre
		void drawImage(int id, int x, int y);

		//Gère les évènements SDL de l'application
		void checkEvents();

		void initChoregraphy();
		void checkPosition();

	private :
		SDL_Surface *					m_screen;
		SDL_Surface *					m_camImage;
		int								m_windowsWidth;
		int								m_windowsHeight;
		int								m_camImageWidth;
		int								m_camImageHeight;
		bool							m_isRunning;

		const int						m_thres;
		ARMultiMarkerInfoT *			m_config;
		std::vector<Image*>				m_images;
				
		//Stocke la liste des marqueur à identifier en fonction de la mesure courante du morceau
		std::vector<Marker>				move[123*4];

		//viewCount stocke le nombre de fois qu'un marqueur a été vu dans une mesure 
		unsigned int					viewCountB, viewCountC, viewCountBL, viewCountSL, viewCountFL, viewCountSR, viewCountFR, viewCountBR;

		unsigned int					bar, beat;
		clock_t							start, end;
		double							elapsed;
		const double					BPM_96;
		const double					BPM_156;

		//Score
		TTF_Font *						m_font;

		//Stocke le temps a attendre entre 2 beat
		double							deltaTime;
};

#endif //__APPLICATION_HPP__