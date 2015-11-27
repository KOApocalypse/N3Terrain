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

	m_pMapData = NULL;

	m_ti_MapSize = 0;
	m_pat_MapSize = 0;

	m_ppPatchRadius = NULL;
	m_ppPatchMiddleY = NULL;

	float TileDirU[8][4] = {
		{0.0f, 1.0f, 0.0f, 1.0f}, // [down]  [RT, LT, RB, LB]
		{0.0f, 0.0f, 1.0f, 1.0f}, // [left]  [RT, LT, RB, LB]
		{1.0f, 0.0f, 1.0f, 0.0f}, // [up]    [RT, LT, RB, LB]
		{1.0f, 1.0f, 0.0f, 0.0f}, // [right] [RT, LT, RB, LB]

		{1.0f, 0.0f, 1.0f, 0.0f}, // [down]  [LT, RT, LB, RB]
		{0.0f, 0.0f, 1.0f, 1.0f}, // [left]  [LT, RT, LB, RB]
		{0.0f, 1.0f, 0.0f, 1.0f}, // [up]    [LT, RT, LB, RB]
		{1.0f, 1.0f, 0.0f, 0.0f}, // [right] [LT, RT, LB, RB]
	};
	memcpy(m_fTileDirU, TileDirU, 8*4*sizeof(float));

	float TileDirV[8][4] = {
		{0.0f, 0.0f, 1.0f, 1.0f}, // [down]  [RT, LT, RB, LB]
		{1.0f, 0.0f, 1.0f, 0.0f}, // [left]  [RT, LT, RB, LB]
		{1.0f, 1.0f, 0.0f, 0.0f}, // [up]    [RT, LT, RB, LB]
		{0.0f, 1.0f, 0.0f, 1.0f}, // [right] [RT, LT, RB, LB]

		{0.0f, 0.0f, 1.0f, 1.0f}, // [down]  [LT, RT, LB, RB]
		{0.0f, 1.0f, 0.0f, 1.0f}, // [left]  [LT, RT, LB, RB]
		{1.0f, 1.0f, 0.0f, 0.0f}, // [up]    [LT, RT, LB, RB]
		{1.0f, 0.0f, 1.0f, 0.0f}, // [right] [LT, RT, LB, RB]
	};
	memcpy(m_fTileDirV, TileDirV, 8*4*sizeof(float));
}

//-----------------------------------------------------------------------------
N3Terrain::~N3Terrain(void) {
	SDL_GL_DeleteContext(m_glCtx);
	m_glCtx = NULL;
	
	SDL_DestroyWindow(m_pWindow);
	m_pWindow = NULL;

	SDL_Quit();

	delete m_pMapData;

	for(int x=0; x<m_pat_MapSize; ++x) {
		delete m_ppPatchRadius[x];
		delete m_ppPatchMiddleY[x];
	}

	delete m_ppPatchRadius;
	delete m_ppPatchMiddleY;
}

//-----------------------------------------------------------------------------
void N3Terrain::Load(FILE* pFile) {
	fread(&m_ti_MapSize, sizeof(uint32_t), 1, pFile);
	printf("DB: m_ti_MapSize = %d\n\n", m_ti_MapSize);

	m_pat_MapSize = (m_ti_MapSize-1)/PATCH_TILE_SIZE;
	printf("DB: m_pat_MapSize = %d\n\n", m_pat_MapSize);

	m_pMapData = new N3MapData[m_ti_MapSize*m_ti_MapSize];
	fread(m_pMapData, sizeof(N3MapData), m_ti_MapSize*m_ti_MapSize, pFile);
	printf("DB: m_pMapData[0] = {\n");
	printf("DB:     fHeight     = %f\n", m_pMapData[0].fHeight);
	printf("DB:     bIsTileFull = %d\n", m_pMapData[0].bIsTileFull);
	printf("DB:     Tex1Dir     = %d\n", m_pMapData[0].Tex1Dir);
	printf("DB:     Tex2Dir     = %d\n", m_pMapData[0].Tex2Dir);
	printf("DB:     Tex1Idx     = %d\n", m_pMapData[0].Tex1Idx);
	printf("DB:     Tex2Idx     = %d\n", m_pMapData[0].Tex2Idx);
	printf("DB: }\n\n");

	m_ppPatchRadius = new float*[m_pat_MapSize];
	m_ppPatchMiddleY = new float*[m_pat_MapSize];

	for(int x=0; x<m_pat_MapSize; ++x) {
		m_ppPatchRadius[x] = new float[m_pat_MapSize];
		m_ppPatchMiddleY[x] = new float[m_pat_MapSize];

		for(int z=0; z<m_pat_MapSize; ++z) {
			fread(&(m_ppPatchMiddleY[x][z]), sizeof(float), 1, pFile);
			fread(&(m_ppPatchRadius[x][z]), sizeof(float), 1, pFile);
		}
	}
	printf("DB: m_ppPatchMiddleY[0][0] = %f\nDB: m_ppPatchRadius[0][0] = %f\n\n",
		m_ppPatchMiddleY[0][0], m_ppPatchRadius[0][0]
	);
}
