#ifndef __N3TERRAIN_H_
#define __N3TERRAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "GL/glew.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"

//-----------------------------------------------------------------------------
class N3Terrain {
private:
	SDL_Window*   m_pWindow;
	SDL_GLContext m_glCtx;

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
