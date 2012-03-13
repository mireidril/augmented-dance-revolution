#include "Application.hpp"

Application::Application()
	: m_gameStarted(false)
	, m_screen(NULL)
	, m_camImage(NULL)
	, m_windowsWidth(800)
	, m_windowsHeight(600)
	, m_camImageWidth(-1)
	, m_camImageHeight(-1)
	, m_isRunning(true)
	, m_thres(50)
	, m_config(NULL)
	, m_marker_info(NULL)
	, viewCountB(0)
	, viewCountC(0)
	, viewCountBL(0)
	, viewCountSL(0)
	, viewCountFL(0)
	, viewCountSR(0)
	, viewCountFR(0)
	, viewCountBR(0)
	, bar(0)
	, beat(-1)
	, start(0)
	, end(0)
	, elapsed(-1.f)
	, BPM_96(625.0)
	, BPM_156(384.0)
	, m_font(NULL)
	, deltaTime(-1)
	, validate(NULL)
	, musique(NULL)
	, moveDone(false)
	, countDownPassed(false)
	, score(0)
{

}

Application::~Application()
{
	arVideoCapStop();
	arVideoClose();

	Mix_CloseAudio();
	TTF_Quit();

	for(int i = 0; i < m_images.size(); ++i)
	{
		SDL_FreeSurface(m_images[i]->m_image);
		delete m_images[i]->m_pos;
		delete m_images[i]->m_size;
		delete m_images[i];
	}

	SDL_FreeSurface(m_camImage);
	SDL_FreeSurface(m_screen);
	SDL_Quit();

	delete m_config;
}

//Initialise la fenêtre SDL et les paramètres ARToolkit
void Application::init()
{
	//SDL
	if(SDL_Init(SDL_INIT_VIDEO) != 0) 
	{
		std::cerr << "Can't initialize SDL : " << SDL_GetError() << std::endl;
		m_isRunning = false;
	}

	SDL_WM_SetCaption("AugmentedDanceRevolution", NULL);

	m_screen = SDL_SetVideoMode(m_windowsWidth, m_windowsHeight, 32, SDL_DOUBLEBUF|SDL_RESIZABLE);

	//SDL_ttf
	TTF_Init();
	m_font = TTF_OpenFont("../fonts/mvboli.ttf", 50);

	//Initialisation caméra
#ifdef _WIN32
	char *vconf = "Data\\WDM_camera_flipV.xml";
#else
	char *vconf = "";
#endif

	ARParam  wparam;
	// open the video path
	if( arVideoOpen( vconf ) < 0 ) exit(0);
	// find the size of the window
	if( arVideoInqSize(&m_camImageWidth, &m_camImageHeight) < 0 ) exit(0);
	printf("Image size (x,y) = (%d,%d)\n", m_camImageWidth, m_camImageHeight);

	// set the initial camera parameters
	char *cparam_name = "Data/camera_para.dat";
	if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
		printf("Camera parameter load error !!\n");
		exit(0);
	}
	ARParam cparam;
	arParamChangeSize( &wparam, m_camImageWidth, m_camImageHeight, &cparam );
	arInitCparam( &cparam );
	printf("*** Camera Parameter ***\n");
	arParamDisp( &cparam );

	// New way multiple patterns
	char *config_name = "Data/multi/marker.dat";
	if( (m_config = arMultiReadConfigFile(config_name)) == NULL ) {
		printf("config data load error !!\n");
		exit(0);
	}

	initChoregraphy();

	//sound init
	bar = 0;
	beat = 0;
	deltaTime = BPM_96;


	//SDL_mixer
	if( Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1)
		std::cout<<"problem init son"<<std::endl; //Initialisation de l'API Mixer

	Mix_AllocateChannels(2);
	musique = Mix_LoadWAV("../musics/queen.ogg");
	validate = Mix_LoadWAV("../musics/validate.ogg");

	Mix_VolumeMusic(MIX_MAX_VOLUME/2);

}

//Charge toutes les images de l'application
void Application::initImages()
{
	//Images Great !
	loadImage("../images/great!.png", 0, 0, 237, 111);
	//Images des silhouettes
	loadImage("../images/fond.png", 0, m_windowsHeight, 731, 141);
	loadImage("../images/silhouette1.png", 0, m_windowsHeight, 125, 141);
	loadImage("../images/silhouette2.png", 0, m_windowsHeight, 125, 141);

	//loadImage("../images/test.jpg", 0, 0, 512, 512, 30, 30);
}

//Charge une image et la stocke dans m_images
void Application::loadImage(const char * filename, int posX, int posY, int sizeX, int sizeY)
{
	Image * img = new Image;
	SDL_Surface * surface = IMG_Load(filename);
	img->m_image = surface;
	SDL_Rect * pos = new SDL_Rect;
	pos->x = posX;
	pos->y = posY;
	img->m_pos = pos;
	SDL_Rect * size = new SDL_Rect;
	size->x = sizeX;
	size->y = sizeY;	
	img->m_size = size;
	m_images.push_back(img);
}

//Affiche et met à jour les silhouettes et le score
void Application::updateInterface()
{
	int nbImagesShowed = 5;
	// ===== Affichage de l'interface
	drawImage(1, NULL, &m_windowsHeight);
	// ===== Affichage des silhouettes
	if(m_gameStarted)
	{
		int xBase = 10, x;
		for(int i = 0; i < nbImagesShowed; ++i)
		{
			x = xBase + 150*i + 20;
			if(bar+i < imagesMove.size())
			{
				drawImage(imagesMove[bar+i], &x, &m_windowsHeight);
			}
		}
	}
}

//Définit la boucle principale de l'application
void Application::run()
{
	init();
	initImages();
	arVideoCapStart();

	while(m_isRunning)
	{
		update();
	}
}

//Gère les opérations de l'application
void Application::update()
{

	// ====== Gère les évènements SDL
	checkEvents();

	// ====== Récupère l'image filmée par la caméra
	ARUint8         *dataPtr;
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

	//test save image
	Uint32 rmask, gmask, bmask, amask;
	rmask = 0x00000000;
	gmask = 0x00000000;
	bmask = 0x00000000;
	amask = 0x000000ff;
	m_camImage = SDL_CreateRGBSurfaceFrom(dataPtrFlipped, sizeX, sizeY, 8 * AR_PIX_SIZE_DEFAULT, sizeX * AR_PIX_SIZE_DEFAULT, rmask, gmask, bmask, amask);

	// detect the markers in the video frame 
	if( arDetectMarker(dataPtr, m_thres, &m_marker_info, &marker_num) < 0 ) {
		exit(0);
	}

	// ======Compte à rebours du début
	if(m_gameStarted && countDownPassed == false) countdownCurrent = clock();
	if (countdownCurrent - countdownStart > 3000 && countDownPassed == false){

	
		start = clock();
		if(musique)
		{
			Mix_PlayChannel(0, musique, 1);
		}
		start = clock();
		countDownPassed  = true;
	}

	// ====== Gestion de la musique et du score
	if(m_gameStarted == true && countDownPassed)
	{
		end = clock();
		elapsed = ((double)end - start);
		
		if(elapsed >= deltaTime){

			start = end;

			beat++;
			 if(beat == 2)
			 {
				 m_markersToDraw.clear();
			 }
			//Changement de mesure
			if(beat == 5){
				if (bar < 77) bar++;
				viewCountB = 0;
				viewCountBL = 0; 
				viewCountBR = 0;
				viewCountC = 0;
				viewCountFL = 0;
				viewCountFR = 0;
				viewCountSL = 0;
				viewCountSR = 0;
				beat=1;
				//std::cout << "mesure " << bar << std::endl;
				moveDone = false;
			}

			if(bar == 13) deltaTime = BPM_156;

			// std::cout << "mesure " << bar;
			// std::cout << "beat " << beat << std::endl;
			
		}

		//MARQUE
		/* check for object visibility */
		//Detecte les marqueurs présents dans move[] en fonction de la mesure du morceau
		for( i = 0; i < move[bar].size(); i++ ) {
			k = -1;
			for( j = 0; j < marker_num; j++ ) {

				if( move[bar].at(i) == m_marker_info[j].id ) {
					/* you've found a pattern */
					printf("Found pattern: %d ", m_marker_info[j].id);
					switch(m_marker_info[j].id){

					case C: {
						printf("Chest"); 
						viewCountC++;
						break;	
							};

					case B: {
						printf("Back");
						viewCountB++;
						break;
							}
					case SR: {

						printf("Shoulder Right");
						viewCountSR++;
						break;
							 }
					case SL: {
						printf("Shoulder Left"); 
						viewCountSL++;
						break;
							 }
					case FR: {
						printf("Hand Right Front"); 
						viewCountFR++;
						break;
							 }
					case BR: {
						printf("Hand Right Back"); 
						viewCountBR++;
						break;
							 }
					case FL: {
						printf("Hand Left Front"); 
						viewCountFL++;
						break;
							 }
					case BL: {
						printf("Hand Left Back"); 
						viewCountBL++;
						break;
							 }

					};
					printf("\n");
					if( k == -1 ) k = j;
					else if( m_marker_info[k].cf < m_marker_info[j].cf ) k = j;
				}
			}

			if( k == -1 ) {
				m_config->marker[i].visible = 0;
				continue;
			}


			checkPosition();

			/* calculate the transform for each marker */
			if( m_config->marker[i].visible == 0 ) {
				arGetTransMat(&m_marker_info[k], m_config->marker[i].center, m_config->marker[i].width, m_config->marker[i].trans);
			}
			else {
				arGetTransMatCont(&m_marker_info[k], m_config->marker[i].trans, m_config->marker[i].center, m_config->marker[i].width, m_config->marker[i].trans);
			}
			m_config->marker[i].visible = 1;

		}
	}

	render();
	arVideoCapNext();

	delete []dataPtrFlipped;
}

//Gère le rendu de l'application
void Application::render()
{
	// ===== Affichage de l'image filmée par la caméra
	float zoomX = (float)m_windowsWidth / m_camImageWidth;
	float zoomY = (float)m_windowsWidth*0.75 / m_camImageHeight;

	if(m_camImage != NULL)
	{
		SDL_Surface * camImage = rotozoomSurfaceXY(m_camImage, 0.0f, zoomX, zoomY, 1);

		SDL_Rect pos;
		pos.x = 0;
		pos.y = 0;
		SDL_BlitSurface(camImage, NULL, m_screen, &pos);
		SDL_FreeSurface(camImage);
	}

	// ===== Affichage des éléments graphiques lors de la détection d'un marqueur
	drawMarkers();

	// ===== Affichage des textes
	SDL_Color green = {0, 255, 0};
	if(moveDone) drawText(20, 10, green, "+100!");
	char scoreFinal[256];
	char scoreChar[50];
	strcpy(scoreFinal,"Score : ");
	sprintf (scoreChar, "%d", score);
	strcat(scoreFinal,scoreChar);

	drawText(600, 20, green, scoreFinal);

	// ===== Affichage des élements de la chorégraphie
	updateInterface();

	// ===== Met à jour l'affichage
	SDL_Flip(m_screen);
	SDL_UpdateRect(m_screen, 0, 0, m_windowsWidth, m_windowsHeight);
}

//Dessine des images sur les marqueurs de m_markersToDraw
void Application::drawMarkers()
{
	for(int i = 0; i < m_markersToDraw.size(); ++i)
	{
		drawMarker(m_markersToDraw[i]);
	}
}

//Dessine une image à la position du marqueur idMarker
void Application::drawMarker(int idMarker)
{
	//Récupération des coordonnées du marqueur sur l'image
	SDL_Rect pos;
	pos.x = (m_marker_info[idMarker].vertex[0][0] + m_marker_info[idMarker].vertex[1][0] + m_marker_info[idMarker].vertex[2][0] + m_marker_info[idMarker].vertex[3][0]) / 4;
	pos.y = (m_marker_info[idMarker].vertex[0][1] + m_marker_info[idMarker].vertex[1][1] + m_marker_info[idMarker].vertex[2][1] + m_marker_info[idMarker].vertex[3][1]) / 4;

	//int z = abs(m_config->marker[idMarker].trans[2][2] - 200);
	//int maxZ = 1500;
	//double rapport = (maxZ-z)/maxZ;
	//float zoomX = (float) (m_images[0]->m_size->x * rapport) / m_images[0]->m_size->x;
	//float zoomY = (float) (m_images[0]->m_size->y * rapport) / m_images[0]->m_size->y;

	SDL_Surface * img = rotozoomSurfaceXY(m_images[0]->m_image, 0.0f, 1.0, 1.0, 1);
	SDL_BlitSurface(img, NULL, m_screen, &pos);
	SDL_FreeSurface(img);
}

//Dessine le texte s à la position (x, y) de la fenêtre
void Application::drawText(int x, int y, SDL_Color color, char* s)
{
	SDL_Surface * texte = TTF_RenderText_Blended(m_font, s, color);
	SDL_Rect pos;
	pos.x = x;
	pos.y = y;
	SDL_BlitSurface(texte, NULL, m_screen, &pos);
	SDL_FreeSurface(texte);
}

//Attribue la position (*i, *j) à une image et la trace. Si i ou j est nul, l'image sera tracée à sa position de base.
void Application::drawImage(int id, int * i, int * j)
{
	if( id < m_images.size() )
	{
		if(i != NULL)
		{
			m_images[id]->m_pos->x = *i;
		}
		if(j != NULL)
		{
			m_images[id]->m_pos->y = *j;
		}

		if(m_images[id]->m_image)
		{
			SDL_Surface * img = rotozoomSurface(m_images[id]->m_image, 0.0f, 1.0, 1);

			SDL_Rect pos;
			pos.x = m_images[id]->m_pos->x;
			pos.y = m_images[id]->m_pos->y - m_images[id]->m_size->y;
			SDL_BlitSurface(img, NULL, m_screen, &pos);
			SDL_FreeSurface(img);
		}
	}
}

//Gère les évènements SDL de l'application
void Application::checkEvents()
{
	SDL_Event event;
	if( SDL_PollEvent(&event) )
	{
		switch (event.type)
		{
		case SDL_QUIT:
			m_isRunning = false;
			break;
		case SDL_VIDEORESIZE:
			m_windowsWidth = event.resize.w;
			m_windowsHeight = event.resize.h;
			m_screen = SDL_SetVideoMode(m_windowsWidth, m_windowsHeight, 32, SDL_VIDEORESIZE | SDL_DOUBLEBUF);
			break;
		case SDL_KEYDOWN:
			switch(event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				m_isRunning = false;
				break;
			case SDLK_RETURN:
				if(!m_gameStarted)
				{
					countdownStart = clock();
					m_gameStarted = true;

				}
				break;
			}
			break;
		}
	}
}

void Application::initChoregraphy()
{
	//77 = nombre de mesures du morceau (+ ou -)
	//* 4 = signature rythmique du morceau. 
	//Avec le tableau move on peut potentiellement faire un chagement de mouvement à chaque temps du morceau

	for(int i = 0; i < 77; i++)
	{
		move[i].push_back(Marker::B);
		move[i].push_back(Marker::C);
		//move[i].push_back(Marker::BL);
		//move[i].push_back(Marker::BR);
		//move[i].push_back(Marker::FL);
		//move[i].push_back(Marker::SR);
		//move[i].push_back(Marker::SL);
		//move[i].push_back(Marker::FR);
	}

	//Ici on remplit les images des choregraphies
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
	imagesMove.push_back(2);
	imagesMove.push_back(3);
}

void Application::checkPosition()
{
	unsigned int threshold = 0; 
	std::vector<bool> posOK;

	int i =0;
	while(i < move[bar].size()){

		switch(move[bar].at(i)){
		case C : 
			if(viewCountC > threshold) posOK.push_back(true);
			else posOK.push_back(false);
			break;
		case B : 
			if(viewCountB > threshold) posOK.push_back(true);
			else posOK.push_back(false);
			break;
		case BL : 
			if(viewCountBL > threshold) posOK.push_back(true);
			else posOK.push_back(false);
			break;
		}//end switch

		i++;
	}//end while

	bool checker= true;
	for(int j =0; j < posOK.size(); j++){

		checker = checker && posOK.at(j);
	}

	if(checker && moveDone == false ){

		Mix_PlayChannel(1, validate, 1);
		score += 100;
		for(int i = 0; i < move[bar].size(); ++i)
		{
			m_markersToDraw.push_back(move[bar].at(i));
		}
		moveDone = true;
	}

	std::cout << "Check Pos" << checker << std::endl;
}