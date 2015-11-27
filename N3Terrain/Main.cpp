/*
*/

#include "N3Terrain.h"

//-----------------------------------------------------------------------------
int SDL_main(int argc, char** argv) {

	N3Terrain& demo = N3Terrain::GetRef();

	FILE* pFile = fopen("karus_start.gtd", "rb");
	if(pFile == NULL) {
		printf("ER: Could not find file.\n");
		system("pause");
		exit(-1);
	}

	demo.Load(pFile);

	fclose(pFile);
	
	printf("Testing!\n");
	system("pause");

	return 0;
}
