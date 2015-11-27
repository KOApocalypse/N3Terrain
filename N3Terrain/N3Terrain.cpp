/*
*/

#include "N3Terrain.h"

//-----------------------------------------------------------------------------
N3Terrain::N3Terrain(void) {
	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init: %s\n", SDL_GetError());
		system("pause");
		exit(-1);
	}

	m_pWindow = SDL_CreateWindow(
		"N3Terrain",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1024,
		720,
		SDL_WINDOW_OPENGL
	);

	if(m_pWindow == NULL) {
		printf("SDL_CreateWindow: %s\n", SDL_GetError());
		system("pause");
		exit(-1);
	}

	m_glCtx = SDL_GL_CreateContext(m_pWindow);

	if(m_glCtx == NULL) {
		printf("SDL_GL_CreateContext: %s\n", SDL_GetError());
		system("pause");
		exit(-1);
	}

	if(SDL_GL_SetSwapInterval(-1) != 0) { // NOTE: try for late swap tearing
		printf("SDL_GL_SetSwapInterval: %s\n", SDL_GetError());
		system("pause");
		exit(-1);
	}

	GLenum glError = glewInit();

	if(glError != GLEW_OK) {
		printf("glewInit: %s\n", glewGetErrorString(glError));
		system("pause");
		exit(-1);
	}

	//glEnable(GL_DEPTH_TEST);
}

//-----------------------------------------------------------------------------
N3Terrain::~N3Terrain(void) {
	SDL_GL_DeleteContext(m_glCtx);
	m_glCtx = NULL;
	
	SDL_DestroyWindow(m_pWindow);
	m_pWindow = NULL;

	SDL_Quit();
}
