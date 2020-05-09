#ifndef __STAR_CAMPAIGN_H_
#define __STAR_CAMPAIGN_H_

#include <list>
#include "resource.h"

typedef struct Planet * PLANET_PTR;
typedef struct Ship * SHIP_PTR;

#define kLogicalBoardSquares 20


typedef enum BUILDPREF { FORTS_FIRST, FACTORIES_FIRST, ALTERNATE_FIRST };

typedef struct Empire
{
	char const *name;
	char defeated;
	char player;
	short index;
	std::list <PLANET_PTR>planets;
	std::list <SHIP_PTR>ships;
	PLANET_PTR current_target;
} EMPIRE, *EMPIRE_PTR, **EMPIRE_HANLDE;

typedef struct _EMPIRE_FILE_DATA
{
	char name[20];
	char defeated;
	char player;
	short index;
} EMPIRE_FILE_DATA;

typedef struct Planet
{
	char const *name;
	EMPIRE *ruler;
	char size;
	char forts;
	char factories;
	float current_ship;
	short x;
	short y;
	short own_index;
	char built_this_turn;
} PLANET;

typedef struct _PLANET_FILE_DATA
{
	short own_index;
	short ruler_index;
	char size;
	char forts;
	char factories;
	char built_this_turn;
	float current_ship;
	short x;
	short y;
} PLANET_FILE_DATA;

typedef struct Ship
{
	short cur_x;
	short cur_y;
	short dest_x;
	short dest_y;
	EMPIRE *owner;
} SHIP, *SHIP_PTR, **SHIP_HANDLE;

typedef struct SHIP_FILE_DATA
{
	short cur_x;
	short cur_y;
	short dest_x;
	short dest_y;
	int owner_index;
} SHIP_FILE_DATA, *SHIP_FILE_DATA_PTR, **SHIP_FILE_DATA_HANDLE;

/*
typedef struct _HEADER
{
char name[5][20];
char used[5];
} HEADER;
*/

//const int kNumPlanets=10;
const int kNumPlanets=20;
const int kMaxEmpires=5;
const int kTurnsPerShip=3;
const int kIconSize=32;
const int gNumempires=3; //code adds 1

int CountShipsToDraw(EMPIRE *, PLANET*);
int CountShipsOnStation(EMPIRE *e, PLANET *p);
void InitEmpire(EMPIRE *e, char const *name, char player);
void InitShip(EMPIRE *e, SHIP_PTR p, short x, short y);
void InitPlanet(PLANET *p, short x, short y, char *name, int size, EMPIRE *ruler);
void UpdateGame();
void UpdateEmpire(EMPIRE *e);
void UpdateShip(SHIP *p);
void UpdateShips(EMPIRE *e);
void SetAttackDest(SHIP *s);
void CreateGalaxy();
void PositionPlanets();
void CreateEmpires();
void EndGame();
void DeleteAllShips();
void DeletePlanetLists();
void ExecuteAllAttacks();
void ExecuteAttacks(EMPIRE *);
void UpdateAllShips();
void UpdateProduction(EMPIRE *e, PLANET *p);
void BuildShip(EMPIRE *e, PLANET *p);
void DeleteShipsAt(EMPIRE *e, PLANET *p);
int CheckGameStatus();
int PromptNewGame();
int CountEnemyPlanets();
void FindTargetForEmpire(EMPIRE *e);
void DisplayGameStatus();
void TargetPlayerShips(short sx, short sy, short dx, short dy);
PLANET *GetPlanetInfo(short x, short y);
void SaveGame( void );
void LoadGame( void );
void EmpireToEmpireData(EMPIRE_FILE_DATA *, int index);
void ShipToShipData(SHIP *, SHIP_FILE_DATA *);
void PlanetToPlanetData( PLANET_FILE_DATA *, int index);
void EmpireDataToEmpire(EMPIRE_FILE_DATA *, int index);
void ShipDataToShip(SHIP_FILE_DATA *);
void PlanetDataToPlanet(PLANET_FILE_DATA *, int index);
void UpdateGame();
int GetShipCount();
void GenerateRandomSequence(int *, int);
void GenerateRandomSequence2(int *, int, int);
void PlayASound( LPCSTR sxResName );

#endif