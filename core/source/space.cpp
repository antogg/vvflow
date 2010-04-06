#include "space.h"
#include "math.h"
#include "stdio.h"
#include <stdlib.h>
#include "iostream"
using namespace std;

#define M_2PI 6.283185308 		// = 2*PI

Space::Space(bool CreateVortexes,
				bool CreateBody, 
				bool CreateHeat,
				double (*sInfSpeedX)(double Time),
				double (*sInfSpeedY)(double Time),
				double (*sRotationV)(double Time))
{
	if ( CreateVortexes ) VortexList = new TList(); else VortexList = NULL;
	if ( CreateBody ) BodyList = new TList(); else BodyList = NULL;
	if ( CreateHeat ) HeatList = new TList(); else HeatList = NULL;
	BodyControlLayer = NULL;

	InfSpeedX = sInfSpeedX;
	InfSpeedY = sInfSpeedY;
	RotationV = sRotationV;
	ForceX = ForceY = 0;
	Time = Angle = BodyY = 0;
}

int Space::ConstructCircle(long BodyListSize)
{
	if (!BodyList) return -1;
	
	TVortex Vort; ZeroVortex(Vort);
	double dfi = M_2PI/BodyListSize;
	double fi = 0;
	BodyControlLayer = new int[BodyListSize];

	for ( int i=0; i<BodyListSize; i++ )
	{
		fi= dfi*i; // don't use += here, cuz it causes systematic error;
		Vort.rx = cos(fi);
		Vort.ry = sin(fi);
		BodyList->Copy(&Vort);
	}

	return 0;
}

int Space::LoadVorticityFromFile(const char* filename)
{
	if ( !VortexList ) return -1;
	FILE *fin;
	char line[255];

	fin = fopen(filename, "r");
	if (!fin) { cerr << "No file called " << filename << endl; return -1; } 
	TVortex Vort; ZeroVortex(Vort);
	while ( !feof(fin) )
	{
		char* err = fgets(line, 255, fin);
		sscanf(line, "%lf\t%lf\t%lf\n", &Vort.rx, &Vort.ry, &Vort.g);
		VortexList->Copy(&Vort);
	}
	fclose(fin);
	return 0;
}

int Space::LoadHeatFromStupidFile(const char* filename, double g)
{
	if ( !HeatList ) return -1;
	FILE *fin;
	char line[255];

	fin = fopen(filename, "r");
	if (!fin) { cerr << "No file called " << filename << endl; return -1; }
	TVortex Vort; InitVortex(Vort, 0, 0, g);
	while ( !feof(fin) )
	{
		char* err = fgets(line, 255, fin);
		sscanf(line, "%lf\t%lf\n", &Vort.rx, &Vort.ry);
		HeatList->Copy(&Vort);
	}
	fclose(fin);
	return 0;
}

int Space::LoadHeatFromFile(const char* filename)
{
	if ( !HeatList ) return -1;
	FILE *fin;
	char line[255];

	fin = fopen(filename, "r");
	if (!fin) { cerr << "No file called " << filename << endl; return -1; }
	TVortex Vort; ZeroVortex(Vort);
	while ( !feof(fin) )
	{
		char* err = fgets(line, 255, fin);
		sscanf(line, "%lf\t%lf\t%lf\n", &Vort.rx, &Vort.ry, &Vort.g);
		HeatList->Copy(&Vort);
	}
	fclose(fin);
	return 0;
}

int Space::Save(const char *filename)
{
	long zero = 0;
	FILE * pFile;
	pFile = fopen(filename, "w");
	if (!pFile) return -1;

	#define SaveList(List) 																\
		if (List) 																		\
		{ 																				\
			fwrite(&(List->size), sizeof(long), 1, pFile); 								\
			fwrite(List->Elements, sizeof(TVortex), List->size, pFile); 				\
		} else { fwrite(&zero, sizeof(long), 1, pFile); } 								\

	SaveList(VortexList)
	SaveList(BodyList)
	SaveList(HeatList)

	fclose(pFile);
	return 0;
}

int Space::Load(const char *filename)
{
	long size;
	if (VortexList) delete VortexList;
	if (BodyList) delete BodyList;
	if (HeatList) delete HeatList;
	delete [] BodyControlLayer;

	FILE * pFile;
	pFile = fopen(filename, "r");
	if (!pFile) return -1;

	size_t err;
	#define LoadList(List) 													\
		err = fread(&size, sizeof(long), 1, pFile); 								\
		printf("%ld ", size); 												\
		if (size) 															\
		{ 																	\
			List = (TList*)malloc(sizeof(TList)); 							\
			List->maxsize = size; 											\
			List->size = size; 												\
			List->Elements = (TVortex*)malloc(size*sizeof(TVortex)); 		\
			err = fread(List->Elements, sizeof(TVortex), size, pFile); 			\
		}

	LoadList(VortexList)
	LoadList(BodyList)
	LoadList(HeatList)

	if (BodyList) BodyControlLayer = new int[BodyList->size];

	fclose(pFile);
	return 0;
}

double Space::Integral()
{
	if (!VortexList) return 0;
	double Summ = 0;
	int lsize = VortexList->size;
	TVortex* Vort = VortexList->Elements;

	for ( int i=0; i<lsize; i++ )
	{
		Summ += Vort->g * (Vort->rx*Vort->rx + Vort->ry*Vort->ry);
		Vort++;
	}

	return Summ;
}

double Space::gsumm()
{
	if (!VortexList) return 0;
	double Summ = 0;
	int lsize = VortexList->size;
	TVortex* Vort = VortexList->Elements;

	for ( int i=0; i<lsize; i++ )
	{
		Summ += Vort->g;
		Vort++;
	}

	return Summ;
}

void Space::HydroDynamicMomentum(double &ResX, double &ResY)
{
	ResX=ResY=0;
	if (!VortexList) return;
	int lsize = VortexList->size;
	TVortex* Vort = VortexList->Elements;

	for ( int i=0; i<lsize; i++ )
	{
		ResX += Vort->g * Vort->rx;
		ResY += Vort->g * Vort->ry;
		Vort++;
	}
}
