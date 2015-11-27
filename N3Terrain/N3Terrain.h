#ifndef __N3TERRAIN_H_
#define __N3TERRAIN_H_

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "GL/glew.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"

const int TILE_SIZE = 4;
const int PATCH_TILE_SIZE = 8;

//-----------------------------------------------------------------------------
struct N3MapData {
	float fHeight;
	uint32_t bIsTileFull: 1;
	uint32_t Tex1Dir: 5;
	uint32_t Tex2Dir: 5;
	uint32_t Tex1Idx: 10;
	uint32_t Tex2Idx: 10;

	N3MapData(void) {
		bIsTileFull = true;
		fHeight = FLT_MIN;
		Tex1Idx = 1023;
		Tex1Dir = 0;
		Tex2Idx = 1023;
		Tex2Dir = 0;
	}
};

//-----------------------------------------------------------------------------
class N3Terrain {
private:
	SDL_Window*   m_pWindow;
	SDL_GLContext m_glCtx;

public:
	N3MapData* m_pMapData;

	uint32_t m_ti_MapSize;
	uint32_t m_pat_MapSize;

	float** m_ppPatchRadius;
	float** m_ppPatchMiddleY;

	float m_fTileDirU[8][4];
	float m_fTileDirV[8][4];

public:
	void Load(FILE* pFile);

private:
	N3Terrain(N3Terrain const&);
	void operator = (N3Terrain const&);

private:
	N3Terrain(void);
	~N3Terrain(void);

public:
	static N3Terrain& GetRef(void) {
		static N3Terrain instance;
		return instance;
	}
};

#endif
