/*
*/

#include <string>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtx/perpendicular.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GL/glew.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_opengl.h"

const int TILE_SIZE = 4;
const int PATCH_TILE_SIZE = 8;

//-----------------------------------------------------------------------------
enum _D3DFORMAT {
	D3DFMT_UNKNOWN,
	D3DFMT_DXT1 = 827611204,
	D3DFMT_DXT3 = 861165636,
	D3DFMT_DXT5 = 894720068,
};

struct _N3TexHeader {
	char szID[4];
	int nWidth;
	int nHeight;
	int Format;
	bool bMipMap;
};

struct _N3Texture {
	GLuint m_lpTexture;
	_N3TexHeader m_Header;

	int compTexSize;
	unsigned char* compTexData;

	int m_iLOD;
	char* m_szName;
	char* m_szFileName;
};

struct _N3MapData {
	float fHeight;
	unsigned int bIsTileFull : 1;
	unsigned int Tex1Dir : 5;
	unsigned int Tex2Dir : 5;
	unsigned int Tex1Idx : 10;
	unsigned int Tex2Idx : 10;

	_N3MapData(void) {
		bIsTileFull = true;
		fHeight = FLT_MIN;
		Tex1Idx = 1023;
		Tex1Dir = 0;
		Tex2Idx = 1023;
		Tex2Dir = 0;
	}
};

typedef struct {
	SDL_bool up;
	SDL_bool down;
	SDL_bool left;
	SDL_bool right;
	SDL_bool space;
} Input;

//-----------------------------------------------------------------------------
void N3Init(void);
void N3Quit(void);

void SetVerts(float* verts, size_t n);
void SetTexture(int texInd1, int texInd2);
void N3LoadTexture(FILE* pFile, _N3Texture* pTex);

//-----------------------------------------------------------------------------
SDL_Window*   window;
SDL_GLContext context;
SDL_bool      running;
Input         pInput;
GLuint        shaderProgram;
GLuint        verBuffer;
GLuint        eleBuffer;
_N3Texture*   m_pTileTex;

//-----------------------------------------------------------------------------
float TileDirU[8][4] = {
	{ 0.0f, 1.0f, 0.0f, 1.0f }, //[down][RT, LT, RB, LB]
	{ 0.0f, 0.0f, 1.0f, 1.0f }, //[left][RT, LT, RB, LB]
	{ 1.0f, 0.0f, 1.0f, 0.0f }, //[up][RT, LT, RB, LB]
	{ 1.0f, 1.0f, 0.0f, 0.0f }, //[right][RT, LT, RB, LB]

	{ 1.0f, 0.0f, 1.0f, 0.0f }, //[down][LT, RT, LB, RB]
	{ 0.0f, 0.0f, 1.0f, 1.0f }, //[left][LT, RT, LB, RB]
	{ 0.0f, 1.0f, 0.0f, 1.0f }, //[up][LT, RT, LB, RB]
	{ 1.0f, 1.0f, 0.0f, 0.0f }, //[right][LT, RT, LB, RB]
};

float TileDirV[8][4] = {
	{ 0.0f, 0.0f, 1.0f, 1.0f }, //[down][RT, LT, RB, LB]
	{ 1.0f, 0.0f, 1.0f, 0.0f }, //[left][RT, LT, RB, LB]
	{ 1.0f, 1.0f, 0.0f, 0.0f }, //[up][RT, LT, RB, LB]
	{ 0.0f, 1.0f, 0.0f, 1.0f }, //[right][RT, LT, RB, LB]

	{ 0.0f, 0.0f, 1.0f, 1.0f }, //[down][LT, RT, LB, RB]
	{ 0.0f, 1.0f, 0.0f, 1.0f }, //[left][LT, RT, LB, RB]
	{ 1.0f, 1.0f, 0.0f, 0.0f }, //[up][LT, RT, LB, RB]
	{ 1.0f, 0.0f, 1.0f, 0.0f }, //[right][LT, RT, LB, RB]
};

//-----------------------------------------------------------------------------
int SDL_main(int argc, char* argv[]) {
	N3Init();

	FILE* fpMap = fopen("karus_start.gtd", "rb");
	if (fpMap == NULL) {
		printf("ER: File not found!\n\n");
		system("pause");
		return -1;
	}

	// NOTE: get the size of the map
	int m_ti_MapSize = 0;
	fread(&m_ti_MapSize, sizeof(int), 1, fpMap);
	printf("DB: m_ti_MapSize = %d\n\n", m_ti_MapSize);

	// NOTE: the number of patches which exist within the map
	int m_pat_MapSize = (m_ti_MapSize - 1) / PATCH_TILE_SIZE;
	printf("DB: m_pat_MapSize = %d\n\n", m_pat_MapSize);

	// NOTE: read in the mapdata
	_N3MapData* m_pMapData = new _N3MapData[m_ti_MapSize*m_ti_MapSize];
	fread(m_pMapData, sizeof(_N3MapData), m_ti_MapSize*m_ti_MapSize, fpMap);
	printf("DB: m_pMapData[0] = {\n");
	printf("DB:     fHeight     = %f\n", m_pMapData[0].fHeight);
	printf("DB:     bIsTileFull = %d\n", m_pMapData[0].bIsTileFull);
	printf("DB:     Tex1Dir     = %d\n", m_pMapData[0].Tex1Dir);
	printf("DB:     Tex2Dir     = %d\n", m_pMapData[0].Tex2Dir);
	printf("DB:     Tex1Idx     = %d\n", m_pMapData[0].Tex1Idx);
	printf("DB:     Tex2Idx     = %d\n", m_pMapData[0].Tex2Idx);
	printf("DB: }\n\n");

	// NOTE: read in the middle Y and radius for each patch
	float** m_ppPatchMiddleY = new float*[m_pat_MapSize];
	float** m_ppPatchRadius = new float*[m_pat_MapSize];

	for (int x = 0; x<m_pat_MapSize; x++) {
		m_ppPatchMiddleY[x] = new float[m_pat_MapSize];
		m_ppPatchRadius[x] = new float[m_pat_MapSize];

		for (int z = 0; z<m_pat_MapSize; z++) {
			fread(&(m_ppPatchMiddleY[x][z]), sizeof(float), 1, fpMap);
			fread(&(m_ppPatchRadius[x][z]), sizeof(float), 1, fpMap);
		}
	}
	printf(
		"DB: m_ppPatchMiddleY[0][0] = %f\n"\
		"DB: m_ppPatchRadius[0][0] = %f\n\n",
		m_ppPatchMiddleY[0][0],
		m_ppPatchRadius[0][0]
	);

	// NOTE: read in the grass attributes
	unsigned char* m_pGrassAttr = new unsigned char[m_ti_MapSize*m_ti_MapSize];
	fread(m_pGrassAttr, sizeof(unsigned char), m_ti_MapSize*m_ti_MapSize, fpMap);
	printf("DB: m_pGrassAttr[0] = %d\n", m_pGrassAttr[0]);

	// NOTE: "set" the grass number
	unsigned char* m_pGrassNum = new unsigned char[m_ti_MapSize*m_ti_MapSize];
	memset(m_pGrassNum, 5, sizeof(unsigned char)*m_ti_MapSize*m_ti_MapSize);
	printf("DB: m_pGrassNum[0] = %d\n\n", m_pGrassNum[0]);

	// NOTE: read in the grass file name
	char* m_pGrassFileName = new char[MAX_PATH];
	fread(m_pGrassFileName, sizeof(char), MAX_PATH, fpMap);
	printf("DB: m_pGrassFileName = \"%s\"\n\n", m_pGrassFileName);

	// NOTE: if there isn't a grass filename then zero out the grass attr
	if (strcmp(m_pGrassFileName, "") == 0) {
		memset(m_pGrassAttr, 0, sizeof(unsigned char)*m_ti_MapSize*m_ti_MapSize);
	} else {
		/* CN3Terrain::LoadGrassInfo(void) */
	}

	//-------------------------------------------------------------------------
	/* Start CN3Terrain::LoadTileInfo(hFile) */
	//-------------------------------------------------------------------------

	int m_NumTileTex;
	fread(&m_NumTileTex, sizeof(int), 1, fpMap);
	printf("DB: m_NumTileTex = %d\n", m_NumTileTex);

	// NOTE: load in all the textures needed for the terrain
	m_pTileTex = new _N3Texture[m_NumTileTex];
	memset(m_pTileTex, 0x00, m_NumTileTex*sizeof(_N3Texture));

	// NOTE: load in the number of texture files needed
	int NumTileTexSrc = 0;
	fread(&NumTileTexSrc, sizeof(int), 1, fpMap);
	printf("DB: NumTileTexSrc = %d\n\n", NumTileTexSrc);

	// NOTE: load in all the texture file names
	char** SrcName = new char*[NumTileTexSrc];
	for (int i = 0; i<NumTileTexSrc; i++) {
		SrcName[i] = new char[MAX_PATH];
		fread(SrcName[i], MAX_PATH, 1, fpMap);
	}
	printf("DB: SrcName[(NumTileTexSrc-1)] = \"%s\"\n\n",
		SrcName[(NumTileTexSrc - 1)]
	);

	// NOTE: SrcIdx is the index to the filename
	// NOTE: TileIdx is the index to the texture within the file (that's why
	// we have to skip)

	// NOTE: associate the textures with their tile texture objects
	FILE* hTTGFile;
	short SrcIdx, TileIdx;

	for (int i = 0; i<m_NumTileTex; i++) {

		fread(&SrcIdx, sizeof(short), 1, fpMap);
		fread(&TileIdx, sizeof(short), 1, fpMap);

		if (i == (m_NumTileTex - 1)) {
			printf(
				"DB: [i==(m_NumTileTex-1)]\n"\
				"DB: SrcIdx = %d, TileIdx = %d\n",
				SrcIdx, TileIdx
			);
		}

		// NOTE: read in the texture file
		hTTGFile = fopen(SrcName[SrcIdx], "rb");
		if (hTTGFile == NULL) {
			printf("ER: Cannot load texture: \"%s\"\n", SrcName[SrcIdx]);
			system("pause");
			return -1;
		}

		if (i == (m_NumTileTex - 1)) {
			printf(
				"DB: SrcName[%d] = \"%s\"\n\n",
				SrcIdx, SrcName[SrcIdx]
			);
		}

		//---------------------------------------------------------------------
		/* CN3Texture::SkipFileHandle(hFile) */
		//-------------------------------------------------------------------------

		// NOTE: skip forward in the file based on the texture compression
		// format
		for (int j = 0; j<TileIdx; j++) {
			int nL = 0;
			fread(&nL, sizeof(int), 1, hTTGFile);

			char* m_szName = new char[(nL + 1)];
			memset(m_szName, 0, sizeof(char)*(nL + 1));

			if (nL > 0) {
				memset(m_szName, 0, (nL + 1));
				fread(m_szName, sizeof(char), nL, hTTGFile);
			}

			delete m_szName;
			m_szName = NULL;

			_N3TexHeader HeaderOrg;
			fread(&HeaderOrg, sizeof(_N3TexHeader), 1, hTTGFile);

			if (
				HeaderOrg.Format == D3DFMT_DXT1 ||
				HeaderOrg.Format == D3DFMT_DXT3 ||
				HeaderOrg.Format == D3DFMT_DXT5
				) {
				// NOTE: basically skipping textures for tiles within the file
				// that aren't the tile we need for m_pTileTex[i]
				int iSkipSize = 0;

				int iWTmp = HeaderOrg.nWidth;
				int iHTmp = HeaderOrg.nHeight;

				if (HeaderOrg.bMipMap) {
					for (; iWTmp >= 4 && iHTmp >= 4; iWTmp /= 2, iHTmp /= 2)
					{
						if (HeaderOrg.Format == D3DFMT_DXT1) iSkipSize += iWTmp*iHTmp / 2;
						else iSkipSize += iWTmp*iHTmp;
					}

					iWTmp = HeaderOrg.nWidth / 2; iHTmp = HeaderOrg.nHeight / 2;
					for (; iWTmp >= 4 && iHTmp >= 4; iWTmp /= 2, iHTmp /= 2)
						iSkipSize += iWTmp*iHTmp * 2;
				}
				else {
					if (HeaderOrg.Format == D3DFMT_DXT1)
						iSkipSize += HeaderOrg.nWidth*HeaderOrg.nHeight / 2;
					else
						iSkipSize += iSkipSize += HeaderOrg.nWidth*HeaderOrg.nHeight;

					iSkipSize += HeaderOrg.nWidth*HeaderOrg.nHeight * 2;
					if (HeaderOrg.nWidth >= 1024) iSkipSize += 256 * 256 * 2;
				}

				fseek(hTTGFile, iSkipSize, SEEK_CUR);
			}
			else {
				// NOTE: right now I'm not supporting the other formats that KO
				// supports (i.e., D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4,
				// D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8)

				printf("ER: Unsupported Texture format!\n\n");
				system("pause");
				return -1;
			}
		}

		//-------------------------------------------------------------------------
		/* CN3Texture::SkipFileHandle(hFile) */
		//-------------------------------------------------------------------------

		m_pTileTex[i].m_iLOD = 0;
		N3LoadTexture(hTTGFile, &m_pTileTex[i]);

		fclose(hTTGFile);
	}

	for (int i = 0; i<NumTileTexSrc; i++) {
		delete[] SrcName[i];
		SrcName[i] = NULL;
	}

	delete[] SrcName;

	int NumLightMap;
	fread(&NumLightMap, sizeof(int32_t), 1, fpMap);
	printf("DB: NumLightMap = %d\n\n", NumLightMap);

	if (NumLightMap>0) {
		printf("ER: NumLightMap > 0! Need to implement this.\n\n");
		exit(-1);
	}

	// Load River

	int m_iRiverCount;
	fread(&m_iRiverCount, sizeof(int32_t), 1, fpMap);
	printf("DB: m_iRiverCount = %d\n\n", m_iRiverCount);

	if (m_iRiverCount>0) {
		printf("ER: m_iRiverCount > 0! Need to implement this.\n\n");
		exit(-1);
	}

	// Load Pond

	int m_iPondMeshNum;
	fread(&m_iPondMeshNum, sizeof(int32_t), 1, fpMap);
	printf("DB: m_iPondMeshNum = %d\n\n", m_iPondMeshNum);

	if (m_iPondMeshNum>0) {
		printf("WR: m_iPondMeshNum > 0! Ignoring this...\n\n");
	}

	//-------------------------------------------------------------------------
	/* End CN3Terrain::LoadTileInfo(hFile) */
	//-------------------------------------------------------------------------

	fclose(fpMap);

	/* SET SHADER PROGRAM */
	// ========================================================================

	// NOTE: source code for the vertex shader
	const GLchar* vertSource = {
		"#version 110\n"\
		"\n"\
		"attribute vec3 pos;\n"\
		"attribute vec2 texcoord1;\n"\
		"attribute vec2 texcoord2;\n"\
		"\n"\
		"varying vec2 fragTexcoord1;\n"\
		"varying vec2 fragTexcoord2;\n"\
		"\n"\
		"uniform mat4 model;\n"\
		"uniform mat4 view;\n"\
		"uniform mat4 proj;\n"\
		"\n"\
		"void main() {\n"\
		"fragTexcoord1 = texcoord1;\n"\
		"fragTexcoord2 = texcoord2;\n"\
		"gl_Position = proj*view*model*vec4(pos, 1.0);\n"\
		"}\n"\
		"\0"
	};

	// NOTE: allocate vertex shader program
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);

	// NOTE: load the vertex shader's source code
	glShaderSource(vertShader, 1, &vertSource, NULL);

	// NOTE: compile the vertex shader's source code
	glCompileShader(vertShader);

	// NOTE: get the status of the compilation
	GLint status;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &status);

	// NOTE: if the compilation failed print the error
	if (status == GL_FALSE) {
		char buffer[512];
		glGetShaderInfoLog(vertShader, 512, NULL, buffer);
		printf("glCompileShader: Vertex\n%s\n", buffer);
		exit(-1);
	}

	// NOTE: source code for the fragment shader
	const GLchar* fragSource = {
		"#version 110\n"\
		"\n"\
		"varying vec2 fragTexcoord1;\n"\
		"varying vec2 fragTexcoord2;\n"\
		"uniform sampler2D tex1;\n"\
		"uniform sampler2D tex2;\n"\
		"\n"\
		"void main() {\n"\
		"vec4 t0 = texture2D(tex1, fragTexcoord1)*vec4(1.0, 1.0, 1.0, 1.0);\n"\
		"vec4 t1 = texture2D(tex2, fragTexcoord2)*vec4(1.0, 1.0, 1.0, 1.0);\n"\
		"if(t0==t1) gl_FragColor = t0;"\
		"else gl_FragColor = (t0+t1);\n"\
		"}\n"\
		"\0"
	};

	// NOTE: allocate fragment shader program
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	// NOTE: load the fragment shader's source code
	glShaderSource(fragShader, 1, &fragSource, NULL);

	// NOTE: compile the vertex shader's source code
	glCompileShader(fragShader);

	// NOTE: get the status of the compilation
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &status);

	// NOTE: if the compilation failed print the error
	if (status == GL_FALSE) {
		char buffer[512];
		glGetShaderInfoLog(fragShader, 512, NULL, buffer);
		printf("glCompileShader: Fragment\n%s\n", buffer);
		exit(-1);
	}

	// NOTE: create a shader program out of the vertex and fragment shaders
	shaderProgram = glCreateProgram();

	// NOTE: attach the vertex and fragment shaders
	glAttachShader(shaderProgram, vertShader);
	glAttachShader(shaderProgram, fragShader);

	// NOTE: link the shader program
	glLinkProgram(shaderProgram);

	// NTOE: get the status of linking the program
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);

	// NOTE: if the program failed to link print the error
	if (status == GL_FALSE) {
		char buffer[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, buffer);
		printf("glLinkProgram: \n%s\n", buffer);
		exit(-1);
	}

	// NOTE: use the newly compiled shader program
	glUseProgram(shaderProgram);

	/* END SET SHADER PROGRAM */
	// ========================================================================

	// NOTE: allocate an array buffer on the GPU
	glGenBuffers(1, &verBuffer);

	// NOTE: bind to the array buffer so that we may send our data to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, verBuffer);

	// NOTE: get a pointer to the position attribute variable in the shader
	// program
	GLint posAttrib = glGetAttribLocation(shaderProgram, "pos");

	// NOTE: specify the stride (spacing) and offset for array buffer which
	// will be used in place of the attribute variable in the shader program
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);

	// NOTE: enable the attribute
	glEnableVertexAttribArray(posAttrib);

	// NOTE: get a pointer to the position attribute variable in the shader
	// program
	GLint texAttrib1 = glGetAttribLocation(shaderProgram, "texcoord1");
	GLint texAttrib2 = glGetAttribLocation(shaderProgram, "texcoord2");

	// NOTE: specify the stride (spacing) and offset for array buffer which
	// will be used in place of the attribute variable in the shader program
	glVertexAttribPointer(texAttrib1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(3 * sizeof(float)));
	glVertexAttribPointer(texAttrib2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(5 * sizeof(float)));

	// NOTE: enable the attribute
	glEnableVertexAttribArray(texAttrib1);
	glEnableVertexAttribArray(texAttrib2);

	// NOTE: allocate a GPU buffer for the element data
	glGenBuffers(1, &eleBuffer);

	// ========================================================================

	// NOTE: push all the terrain information to the GPU to save on draw time

	int* gl_buf_offsets = (int*) malloc((m_ti_MapSize * m_ti_MapSize) * sizeof(int));

	int buffer_len = 28 * (m_ti_MapSize * m_ti_MapSize);
	float* vertex_info = (float*) malloc(buffer_len * sizeof(float));

	int gl_os = 0;
	int offset = 0;

	for (int pX = 1; pX<(m_ti_MapSize-1); ++pX) {
		for (int pZ = 1; pZ<(m_ti_MapSize-1); ++pZ) {
			if (pX<1 || pX>(m_ti_MapSize - 1)) continue;
			if (pZ<1 || pZ>(m_ti_MapSize - 1)) continue;

			_N3MapData MapData = m_pMapData[(pX*m_ti_MapSize) + pZ];

			float u10, u11, u12, u13;
			float v10, v11, v12, v13;
			float u20, u21, u22, u23;
			float v20, v21, v22, v23;

			u10 = TileDirU[MapData.Tex1Dir][2];
			u11 = TileDirU[MapData.Tex1Dir][0];
			u12 = TileDirU[MapData.Tex1Dir][1];
			u13 = TileDirU[MapData.Tex1Dir][3];

			v10 = TileDirV[MapData.Tex1Dir][2];
			v11 = TileDirV[MapData.Tex1Dir][0];
			v12 = TileDirV[MapData.Tex1Dir][1];
			v13 = TileDirV[MapData.Tex1Dir][3];

			u20 = TileDirU[MapData.Tex2Dir][2];
			u21 = TileDirU[MapData.Tex2Dir][0];
			u22 = TileDirU[MapData.Tex2Dir][1];
			u23 = TileDirU[MapData.Tex2Dir][3];

			v20 = TileDirV[MapData.Tex2Dir][2];
			v21 = TileDirV[MapData.Tex2Dir][0];
			v22 = TileDirV[MapData.Tex2Dir][1];
			v23 = TileDirV[MapData.Tex2Dir][3];

			float x0, y0, z0;
			float x1, y1, z1;
			float x2, y2, z2;
			float x3, y3, z3;

			x0 = (float)(pX*TILE_SIZE);
			y0 = m_pMapData[(pX*m_ti_MapSize) + pZ].fHeight;
			z0 = (float)(pZ*TILE_SIZE);

			x1 = (float)(pX*TILE_SIZE);
			y1 = m_pMapData[(pX*m_ti_MapSize) + pZ + 1].fHeight;
			z1 = (float)((pZ + 1)*TILE_SIZE);

			x2 = (float)((pX + 1)*TILE_SIZE);
			y2 = m_pMapData[((pX + 1)*m_ti_MapSize) + pZ + 1].fHeight;
			z2 = (float)((pZ + 1)*TILE_SIZE);

			x3 = (float)((pX + 1)*TILE_SIZE);
			y3 = m_pMapData[((pX + 1)*m_ti_MapSize) + pZ].fHeight;
			z3 = (float)(pZ*TILE_SIZE);

			float vertices[] = {
				x0, y0, z0, u10, v10, u20, v20, // Top-left
				x1, y1, z1, u11, v11, u21, v21, // Top-right
				x2, y2, z2, u12, v12, u22, v22, // Bottom-right
				x3, y3, z3, u13, v13, u23, v23  // Bottom-left
			}; size_t n = sizeof(vertices) / sizeof(float);

			gl_buf_offsets[(pX*m_ti_MapSize) + pZ] = offset;
			for (int i = 0; i < n; ++i) vertex_info[offset++] = vertices[i];
		}
	}

	SetVerts(vertex_info, offset);

	/* TESTING */
	// ========================================================================

	glm::mat4 model;
	float angle = (float)M_PI / 50.0f;
	model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

	GLint uniModel = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

	float pDist = 155.0f;
	glm::mat4 view = glm::lookAt(
		glm::vec3(pDist, pDist, pDist),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 proj = glm::perspective(45.0f, 800.0f / 600.0f, 0.5f, 1000.0f);

	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

	glm::vec3 m_position(106.96f, 80.0f, 206.13f);
	glm::vec3 m_direction(0.929f, 0.0f, 0.368f);

	/* END TESTING */
	// ========================================================================

	SDL_Event e = {};
	while (e.type != SDL_QUIT) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				// NOTE: check for keyup
			case SDL_KEYUP: {
				switch (e.key.keysym.sym) {
				case SDLK_UP: {pInput.up = SDL_FALSE; } break;
				case SDLK_DOWN: {pInput.down = SDL_FALSE; } break;
				case SDLK_LEFT: {pInput.left = SDL_FALSE; } break;
				case SDLK_RIGHT: {pInput.right = SDL_FALSE; } break;
				case SDLK_SPACE: {pInput.space = SDL_FALSE; } break;
				}
			} break;

				// NOTE: check for keydown
			case SDL_KEYDOWN: {
				switch (e.key.keysym.sym) {
				case SDLK_UP: {pInput.up = SDL_TRUE; } break;
				case SDLK_DOWN: {pInput.down = SDL_TRUE; } break;
				case SDLK_LEFT: {pInput.left = SDL_TRUE; } break;
				case SDLK_RIGHT: {pInput.right = SDL_TRUE; } break;
				case SDLK_SPACE: {pInput.space = SDL_TRUE; } break;
				}
			} break;
			}
		}

		if (pInput.up && !pInput.space) m_position += m_direction;
		if (pInput.down && !pInput.space) m_position -= m_direction;

		if (pInput.left) {
			m_direction = glm::rotate(m_direction, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		if (pInput.right) {
			m_direction = glm::rotate(m_direction, -angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		if (pInput.space && pInput.up) m_position.y += 1.0f;
		if (pInput.space && pInput.down) m_position.y -= 1.0f;

		view = glm::lookAt(m_position, m_position + m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
		GLint uniView = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		/* === */

		// NOTE: clear the screen buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* === */

		static int lastPatZ = 0;
		static int lastPatX = 0;

		int curPatZ = (int)(m_position.z / ((float)(TILE_SIZE*PATCH_TILE_SIZE)));
		int curPatX = (int)(m_position.x / ((float)(TILE_SIZE*PATCH_TILE_SIZE)));

		int pZ, pX;
		for (pX = curPatX - 8; pX<curPatX + 8; ++pX) {
			for (pZ = curPatZ - 8; pZ<curPatZ + 8; ++pZ) {
				if (pX<1 || pX>(m_pat_MapSize-1)) continue;
				if (pZ<1 || pZ>(m_pat_MapSize-1)) continue;

				int m_ti_LBPoint_x = (pX*PATCH_TILE_SIZE);
				int m_ti_LBPoint_z = (pZ*PATCH_TILE_SIZE);

				int iz, ix;
				for (ix = 0; ix<PATCH_TILE_SIZE; ++ix) {
					for (iz = 0; iz<PATCH_TILE_SIZE; ++iz) {
						int tx = ix + m_ti_LBPoint_x;
						int tz = iz + m_ti_LBPoint_z;

						_N3MapData MapData = m_pMapData[(tx*m_ti_MapSize) + tz];

						SetTexture(MapData.Tex1Idx, MapData.Tex2Idx);

						int i1 = gl_buf_offsets[(tx*m_ti_MapSize) + tz];
						int i2 = (i1 / 28);
						int i3 = (i2 * 6);

						glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i3*sizeof(GLuint)));
					}
				}
			}
		}

		// NOTE: swap the front and back buffers
		SDL_GL_SwapWindow(window);
	}

	N3Quit();
	return 0;
}

//-----------------------------------------------------------------------------
void N3LoadTexture(FILE* pFile, _N3Texture* pTex) {
	// NOTE: length of the texture name
	int nL = 0;
	fread(&nL, sizeof(int), 1, pFile);

	pTex->m_szName = new char[(nL + 1)];

	if (nL > 0) {
		memset(pTex->m_szName, 0, (nL + 1));
		fread(pTex->m_szName, sizeof(char), nL, pFile);
	}

	// NOTE: read in the texture header
	fread(&pTex->m_Header, sizeof(_N3TexHeader), 1, pFile);

	// NOTE: the textures contain multiple mipmap data "blocks"
	switch (pTex->m_Header.Format) {
	case D3DFMT_DXT1: {
		pTex->compTexSize = (pTex->m_Header.nWidth*pTex->m_Header.nHeight / 2);
	} break;
	case D3DFMT_DXT3: {
		pTex->compTexSize = (pTex->m_Header.nWidth*pTex->m_Header.nHeight);
	} break;
	case D3DFMT_DXT5: {
		pTex->compTexSize = (pTex->m_Header.nWidth*pTex->m_Header.nHeight * 2);
		printf("ER: D3DFMT_DXT5 tex; need to verify size!\n\n");
		exit(-1);
	} break;
	}

	int nMMC = 1;
	if (pTex->m_Header.bMipMap) {
		// NOTE: calculate the number of MipMap which are possible with this
		// texture
		nMMC = 0;
		for (
			int nW = pTex->m_Header.nWidth, nH = pTex->m_Header.nHeight;
			nW >= 4 && nH >= 4;
			nW /= 2, nH /= 2
			) nMMC++;
	}
	else {
		// NOTE: not a mipmap
		printf("ER: Need to implement non-mipmap textures!\n\n");
		exit(-1);
	}

	// NOTE: this is a little different because we have mipmaps for each LOD so
	// we have to skip the LOD mipmaps we don't want and grab the ones we do
	// want
	if (pTex->m_iLOD > 0) {
		// NOTE: need to skip of the file pointer until we reach our LOD
		// mipmaps
		printf("ER: m_iLOD > 0 need to skip to the right level!\n\n");
		exit(-1);
	}

	// NOTE: get the first mipmap data for LOD = 0
	if (pTex->compTexData) {
		// NOTE: right now this is global - meaning I can only display one
		// texture
		delete pTex->compTexData;
		pTex->compTexData = NULL;
	}

	pTex->compTexData = new unsigned char[pTex->compTexSize];
	fread(pTex->compTexData, sizeof(unsigned char), pTex->compTexSize, pFile);

	// NOTE: allocate an OpenGL texture
	glGenTextures(1, &pTex->m_lpTexture);

	// NOTE: set the texture to unit 0
	glActiveTexture(GL_TEXTURE0);

	// NOTE: bind to the texture so that we may send our data to the GPU
	glBindTexture(GL_TEXTURE_2D, pTex->m_lpTexture);

	// NOTE: send the pixels to the GPU (will have to convert enums from dxd to
	// opengl)
	GLenum texFormat;
	switch (pTex->m_Header.Format) {
	case D3DFMT_DXT1: {
		texFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	} break;
	case D3DFMT_DXT3: {
		texFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	} break;
	case D3DFMT_DXT5: {
		texFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	} break;
	default: {
		printf("ER: Unknown texture format!\n");
		exit(-1);
	} break;
	}

	glCompressedTexImage2D(
		GL_TEXTURE_2D, 0, texFormat,
		pTex->m_Header.nWidth, pTex->m_Header.nHeight, 0,
		pTex->compTexSize, pTex->compTexData
		);

	// NOTE: generate the mipmaps for scaling
	glGenerateMipmapEXT(GL_TEXTURE_2D);
}

//-----------------------------------------------------------------------------
void SetVerts(float* verts, size_t n) {
	// NOTE: bind to the array buffer so that we may send our data to the GPU
	glBindBuffer(GL_ARRAY_BUFFER, verBuffer);

	// NOTE: send our vertex data to the GPU and set as STAIC
	glBufferData(GL_ARRAY_BUFFER, n*sizeof(float), verts, GL_STATIC_DRAW);

	// NOTE: allocate the elements for the vertex object buffer
	size_t quads = (n / 28);
	GLuint* elems = (GLuint*)malloc(quads * 6 * sizeof(GLuint));
	memset(elems, 0x00, quads * 6 * sizeof(GLuint));

	for (int i = 0; i<quads; ++i) {
		GLuint elements[] = {
			0 + i * 4, 1 + i * 4, 2 + i * 4,
			2 + i * 4, 3 + i * 4, 0 + i * 4
		};

		memcpy(elems + i * 6, elements, 6 * sizeof(GLuint));
	}

	// NOTE: bind to the element buffer so that we may send our data to the GPU
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBuffer);

	// NOTE: send our element data to the GPU and set as STAIC
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, quads * 6 * sizeof(GLuint), elems, GL_STATIC_DRAW);

	//free(elems);
}

//-----------------------------------------------------------------------------
void SetTexture(int texInd1, int texInd2) {
	// NOTE: set the texture to unit 0
	glActiveTexture(GL_TEXTURE0);

	// NOTE: bind to the texture so that we may send our data to the GPU
	glBindTexture(GL_TEXTURE_2D, m_pTileTex[texInd1].m_lpTexture);

	// NOTE: bind the uniform "tex" in the fragment shader to the unit 0
	// texture
	glUniform1i(glGetUniformLocation(shaderProgram, "tex1"), 0);

	// NOTE: not all the tiles have two textures!
	if (m_pTileTex[texInd2].m_Header.Format == 0) {
		glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 0);
		return;
	}

	// NOTE: set the texture to unit 1
	glActiveTexture(GL_TEXTURE1);

	// NOTE: bind to the texture so that we may send our data to the GPU
	glBindTexture(GL_TEXTURE_2D, m_pTileTex[texInd2].m_lpTexture);

	// NOTE: bind the uniform "tex" in the fragment shader to the unit 1
	// texture
	glUniform1i(glGetUniformLocation(shaderProgram, "tex2"), 1);
}

//-----------------------------------------------------------------------------
void N3Init(void) {
	// NOTE: initialize SDL2 library
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init: %s\n", SDL_GetError());
		exit(-1);
	}

	// NOTE: create game window
	window = SDL_CreateWindow(
		"N3Terrain",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		1024,
		720,
		SDL_WINDOW_OPENGL
		);

	// NOTE: check for window
	if (window == NULL) {
		printf("SDL_CreateWindow: %s\n", SDL_GetError());
		exit(-1);
	}

	// NOTE: create an OpenGL context for the window
	context = SDL_GL_CreateContext(window);

	// NOTE: check context
	if (context == NULL) {
		printf("SDL_GL_CreateContext: %s\n", SDL_GetError());
		exit(-1);
	}

	// NOTE: set the buffer swap interval to get vsync
	if (SDL_GL_SetSwapInterval(1) != 0) {
		printf("SDL_GL_SetSwapInterval: %s\n", SDL_GetError());
		exit(-1);
	}

	// NTOE: initialize the OpenGL library function calls
	GLenum glError = glewInit();

	// NOTE: check for error
	if (glError != GLEW_OK) {
		printf("glewInit: %s\n", glewGetErrorString(glError));
		exit(-1);
	}

	// NOTE: enable the depth test
	glEnable(GL_DEPTH_TEST);
}

//-----------------------------------------------------------------------------
void N3Quit(void) {
	// NOTE: free the OpenGL context
	SDL_GL_DeleteContext(context);
	context = NULL;

	// NOTE: free the SDL2 window
	SDL_DestroyWindow(window);
	window = NULL;

	// NOTE: quit the SDL2 library
	SDL_Quit();
}
