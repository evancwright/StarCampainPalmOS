
#include "StdAfx.h"
#include "StarCampaign.h"

-void NewGame();

extern bool gbGameOver;
extern BOOL bAutoBuild;
extern BUILDPREF gBuildFirst;


PLANET gPlanets[kNumPlanets];
EMPIRE gEmpires[kMaxEmpires+1]; //leave room for the player

int gNumEmpires=3; 
int gSequence[kMaxEmpires+1];
SHIP * GetShip( std::list<SHIP*> &l, int j);

//#define kLogicalBoardSquares 15
 
char const *kPlayerName = "Terrans";

char const *kPlanetNames[] =  
	{ 
		"Anna", 
		"Orion", 
		"Nena", 
		"Vega",
		"Draco",
		"Fizbin",
		"Ceti",
		"Tycho",
		"Spica",
		"Lyra",

		//new names

		"Balfour", 
		"Mizar", 
		"Alcor", 
		"Deneb",
		"Al Nath",
		"Rigel",
		"Delphina",
		"Kepler",
		"Sigma",
		"Sol"

	};

char const *kEmpireNames[] = 
	{
		"Cyborgs",
		"Vians",
		"Beltans",
		"Deltans",
	};


/*interface functions defined in starter.c*/
 
extern void AlertDefeat();
extern void AlertEmpireDefeated(char const *, char const *);
extern void AlertPlanetLost(char const *attacker, char const *defender, char const *planet);
extern void AlertAttackFailed(char const *defender, char const *planet);
extern void AlertPlayerAttacked(char * attacker, char *planet);
extern void AlertPlanetCaptured(const char *  planet);

void UpdateInfrastructure();
void EmpireTakesPlanet(EMPIRE *e, int i, int defender_strength);


void DeleteSomeShips(EMPIRE *ruler, PLANET *planet, int how_many);
int AvgDist(EMPIRE *e, PLANET *p);
//void DisplayDBErr(Err err);
void FindTargetForEmpire(EMPIRE *e);
void UpdateEmpires();
int AttackShipsAt(EMPIRE *e, int num_ships, int opposing_ships, int i);
void AttackPlanet(EMPIRE *e, int num_ships, int i);
void DeleteShipList( std::list<SHIP*> &list);
PLANET * GetPlanet( std::list<PLANET*> &l, int j);
void DeleteShip( std::list<SHIP*> &list, SHIP *ship );
void DeletePlanet( std::list<PLANET*> &list, PLANET *p);
void Revolt();
PLANET *GetPlanet( int x, int y );
 

/**
*If this empire if the player's the 3rd arg should be TRUE
*/
void InitEmpire(EMPIRE *e, char const *name, char player)
{
   e->name = name;
//   InitList(&(e->planets));
//   InitList(&(e->ships));
   e->player=(char)player;
   e->defeated=FALSE;
   e->current_target=NULL;
}

void InitShip(EMPIRE *e, SHIP *p, short x, short y)
{
   p->cur_x=x;
   p->cur_y=y;
   p->dest_x=-1;
   p->dest_y=-1;
   p->owner = e;
}

void InitPlanet(PLANET *p, short x, short y, char *name, int size, EMPIRE *ruler)
{
   p->x=x;
   p->y=y;
   p->name = name;
   p->size=size;
   p->ruler=ruler;
   p->factories=0;
   p->forts=0;
   p->current_ship=0;
   p->built_this_turn=0;
}

/*
*Loops through each empire and updates it.
*/

void UpdateEmpires()
{
	int i=0;
	for (; i < gNumEmpires+1; i++) /*player plus computer empires*/
	{
		UpdateEmpire(&gEmpires[i]);
	}
}


void UpdateEmpire(EMPIRE *e)
{
   PLANET *best_target=0;
   //int i=0;

   //for each planet, update infrastructure, forts, production
   //if not player, loop through all untargeted ships, set destinations.

   std::list< PLANET *>::iterator i;
//    ResetIterator(&(e->planets));

	for (i=e->planets.begin(); i != e->planets.end(); i++)
	{
		 //get item
		 //PLANET * p = (PLANET *)GetNextItem(&(e->planets)); 
 		 //UpdateProduction(e, p);
		UpdateProduction(e, *i);
	}

   if (e->player==FALSE)
   {
		std::list< SHIP *>::iterator i;
//	   ResetIterator(&(e->ships));
	   best_target=NULL;

	   for (i=e->ships.begin(); i != e->ships.end(); i++)
	   {
		  SHIP *ship = *i;
		  		  
		  if (ship->dest_x==-1 && ship->dest_y==-1)
		  {//find someone to attack
		  
			if (e->current_target==NULL)
			{
				FindTargetForEmpire(e);
			}	
			 ship->dest_x = e->current_target->x;
			 ship->dest_y = e->current_target->y;
		  } 
	   }
   }//end not player
   else
   {
	   if ( bAutoBuild )
	   {
		   UpdateInfrastructure();
	   }
   }
}


/*
* Picks a random order in which to traverse the
* list of empires, then updates all ships.
*/

void ExecuteAllAttacks()
{
	int i;

	int *sequence = gSequence;

	if (sequence)
	{//update in random order
		GenerateRandomSequence(gSequence, gNumEmpires+1);
	
		for (i=0; i < gNumEmpires+1; i++)
		{
			if (gEmpires[0].defeated == FALSE)
			{
				ExecuteAttacks(&gEmpires[sequence[i]]);
			}
		}

		//MemPtrFree(sequence);
	}
	else
	{//update in linear order
		for (i=0; i < gNumEmpires+1; i++)
		{
			if (gEmpires[0].defeated == FALSE)
				ExecuteAttacks(&gEmpires[i]);
		}
	}
}

/**
 *Loops through empire's ships list. Moves all initialized ships
 */
void UpdateShips(EMPIRE *e)
{
//	int i=0;
	SHIP *ship;

//	ResetIterator(&(e->ships));
	std::list< SHIP* >::iterator i;

   for (i = e->ships.begin(); i != e->ships.end(); i++)
   {
      ship = (SHIP *)*i;
      if (ship!=NULL)
	  {
	     //just use simple up, dn, left, right for now
		if (ship->dest_x!=-1 && ship->dest_y!=-1)
		{
	         UpdateShip(ship);
		}
	  }
   }
}

/*
* Moves a ship to its destination.
* destination does not have to be valid
* uses u,d,l,r for now. at least they
* will move diagonally
*/

void UpdateShip(SHIP *p)
{
int i;
   if (p->cur_x > p->dest_x)
      p->cur_x--;
   else if (p->cur_x < p->dest_x)
      p->cur_x++;

   if (p->cur_y > p->dest_y)
      p->cur_y--;
   else if (p->cur_y < p->dest_y)
      p->cur_y++;

	//see if the planet is still hostile, if not,
	//unset the ships destination

	if (p->cur_x == p->dest_x && p->cur_y == p->dest_y)
	{
		for (i=0; i < kNumPlanets; i++)
		{
			if (gPlanets[i].x==p->cur_x && gPlanets[i].y==p->cur_y)
			{
				if (gPlanets[i].ruler==p->owner)
				{
					p->dest_x = -1;
					p->dest_y = -1;
				}
					break;
			}
		}
	}
	
}

/*
*This function returns the number of ships on station at the target planet which
*are owned by the argument empire
*/
int CountShipsOnStation(EMPIRE *e, PLANET *p)
{
 //  int i=0;
   int count=0;
   int x = p->x;
   int y = p->y;

//   ResetIterator(&(e->ships));
   std::list<SHIP*>::iterator i; 

   for (i = e->ships.begin(); i != e->ships.end(); i++)
   {
      SHIP* s = (SHIP*)*i;

     if ( (s->dest_x == x && s->dest_y == y) || //destination == planet
          (s->dest_x == -1 && s->dest_y == -1) ) //destination not set
	 { 
		 if (s->cur_x == x && s->cur_y == y) //currently at planet
	         count++;
	 }
   }  
   
   return count;
}


/*
*This function returns the number of ships on station at the target planet which
*are owned by the argument empire
*/
int CountShipsToDraw(EMPIRE *e, PLANET *p)
{
//   int i=0;
   int count=0;
   int x = p->x;
   int y = p->y;

//   ResetIterator(&(e->ships));
   std::list<SHIP*>::iterator i;	
   
   for (i=e->ships.begin(); i != e->ships.end(); i++)
   {
      SHIP* s = (SHIP*)*i;
 
 	    //if src and dest are same and planet is hostile, don't draw
 	
 		if ((s->cur_x == x && s->cur_y == y) && (s->dest_x == x && s->dest_y == y) && p->ruler==e)
		{
 			count++;
		}
	    else if (	s->dest_x==-1 && s->dest_y==-1   &&
	    			s->cur_x == x && s->cur_y == y 
	    		)
		{
	    	count++;
		}
   }  
   
   return count;
}


/**
 * Determines how many ships are on station to attack, sums their strength,
 * then attacks.
 * This function should be invoked after all ships have been assigned
 * destinations
 */
  void ExecuteAttacks(EMPIRE *e)
  {
	  int i=0;
	  int k=0;
	  //EMPIRE *defender;
	  //int defender_strength;
	  int opposing_ships;

      for (i=0; i < kNumPlanets; i++)
      {
           int num_ships = CountShipsOnStation(e, &gPlanets[i]);

			//count the number of ships owned by the planet's
			//owner in that sector
			
			if (num_ships > 0 && gPlanets[i].ruler!=NULL && gPlanets[i].ruler != e)
			{
				opposing_ships = CountShipsOnStation(gPlanets[i].ruler, &gPlanets[i]);
		
				if (opposing_ships > 0)
					num_ships = AttackShipsAt(e, num_ships, opposing_ships, i);
			
			}//end hostile planet
								
			//now execute the ground attack					
								
		    if (num_ships > 0)
        	{//attack the planet
          		AttackPlanet(e, num_ships, i);
            }//end ships on station > 0 

      }//end loop through planets
  }



 void CreateGalaxy()
 {
 	 PositionPlanets();
	 CreateEmpires(); /*player + three others */
 }

 /*
 *Sets the x,y positions 
 */
 void PositionPlanets()
 {
	 int i=0,j=0;
	 int x,y;
	 char grid[kLogicalBoardSquares][kLogicalBoardSquares];	

	//logical board size is now 

	 for (i=0; i < 9; i++)
	 	for (j=0; j < 8; j++)
	 		grid[i][j]=FALSE;	

	 for (i=0; i < kNumPlanets; i++)
	 {
		//position it
  
		while (1)
		{
			x = rand()%(kLogicalBoardSquares-3); //0 <= 14
			y = (rand()% (kLogicalBoardSquares-2) )+1; //1 to <= 14
						
			//already occupied
			if (grid[x][y]==TRUE)
			{
				continue;
			}

			//don't put two planet "on top of" each other
			
			if (grid[x][y-1]==TRUE) //planet above us
			{
				continue;
			}

			if (y < kLogicalBoardSquares-1)
				if (grid[x][y+1]==TRUE) //below
					continue;
			
			//check left and right	
		
			if (x+1 < kLogicalBoardSquares)
				if (grid[x+1][y]==TRUE)
					continue;	
				

			if (x>0)
				if (grid[x-1][y]==TRUE)
					continue;	
				
			//due to the planet labels
			//check below and 
	
				
			if ((y+1) < kLogicalBoardSquares && (x+2) < kLogicalBoardSquares )
			{//below below right
				if (grid[x+1][y+1]==TRUE)
					continue;
				if (grid[x+2][y+1]==TRUE)
					continue;
			}
			
			
			//check below left
			//TBD

			if (y > 0 && x > 2)
			{//check up left
				if (grid[x-1][y-1]==TRUE)
					continue;
				if (grid[x-2][y-1]==TRUE)
					continue;			
			}	
				
			
			//check up right
			//TBD
			break;	
					
		}
		
		grid[x][y]=TRUE;
				
		//set the data
		InitPlanet(&gPlanets[i], x, y, (char*)kPlanetNames[i], (rand()%3)+1, NULL);

	 }

 }


 /*
 * This function intializes all the empires, even
 * if they are not used.  The first slot in the
 * array of empires is reserved for the player's
 * empire.
 * each empire starts the game with one planet.
 */

 void CreateEmpires()
 {
 	 
	SHIP *ship; //players initial ship
	
	 //leave first slot for player empire
	 int sequence[kNumPlanets];

 
	//initialize all empires (we may not use them all)

	 int i=1;
	 for (; i <= gNumEmpires; i++) //1 to <=3
	 {
		InitEmpire(&gEmpires[i], (char const *)kEmpireNames[i-1], (char)FALSE);
	 }
	 //create player empire

	 InitEmpire(&gEmpires[0], (char const *)kPlayerName, (char)TRUE);
	
	 //assign each empire a home planet

	 GenerateRandomSequence2(sequence, 0, kNumPlanets);
	 
	 for (i=0; i <= gNumEmpires; i++)
	 {
		//give the player a big planet
		
		if (i==0)
		{
			gPlanets[sequence[i]].size=3;
		}

		gEmpires[i].planets.push_back(&gPlanets[sequence[i]]);
//		AddItem(&(gEmpires[i].planets), 
		gPlanets[sequence[i]].ruler=&gEmpires[i];

		
		gEmpires[i].index=i; //used for saving game

	 }

  //give each empire a ship to start with

	 for (i=0; i <= gNumEmpires; i++)
	 {
	 //	PLANET * target;
	 	PLANET * p= (PLANET*)GetPlanet( gEmpires[i].planets, 0);
	 	
	 	ship = new SHIP;	
		  	
	  	
	  	if (ship!=0)
	  	{//check for null pointers
	  	
		  	InitShip(&gEmpires[i], ship, p->x,p->y);

			//if the empire is a computer empire, pick a target
			//for the ship
			if (i!=0)
			{
				 FindTargetForEmpire(&gEmpires[i]);
	
				 // Set attack dest would set a seperate 
				 //target for every ship.
				 // SetAttackDest(ship);
		
				 ship->dest_x = gEmpires[i].current_target->x;
				 ship->dest_y = gEmpires[i].current_target->y;
			}
		
		  	gEmpires[i].ships.push_back(ship);

	  	}//end ship allocated
	  	
 	}//end for		
 	
 }

/*
*Cleans up memory, this function should be called by the
*interface.
*/
void EndGame()
{
	DeleteAllShips();
	DeletePlanetLists();
}

 /*
 * The only dynamic memory in the game are the ships
 * Traverse empires and delete ship lists.
 */
 void DeleteAllShips()
 {
	 int i;
	 for (i=0; i < kMaxEmpires; i++)
	 {
		 if (gEmpires[i].ships.size() > 0)
		 {

			DeleteShipList( gEmpires[i].ships );			
		 }
	 }
 }

 /*
 * Deletes all elements from the planet lists in the
 * empires.  The planets are statically allocated,
 * but the list nodes need to be deallocated
 */
 void DeletePlanetLists()
 {
	 int i;
	 for (i=0; i < kMaxEmpires; i++)
	 {
		while ( gEmpires[i].planets.size() > 0 )
		{ 
		//	void *item = GetItem(&(gEmpires[i].planets), 0);
		//	DeleteItem(&(gEmpires[i].planets), item);
			gEmpires[i].planets.pop_back();
			
		}
	 }
 }


void UpdateAllShips()
{
	int i;
	for (i=0; i < gNumEmpires+1; i++)
		UpdateShips(&gEmpires[i]);
}



void UpdateProduction(EMPIRE *e, PLANET *p)
{
	if (e->player == FALSE)
	{
		if (p->factories < p->size)
		{
			p->factories++;
		}
		else if (p->forts < p->size)
		{
			p->forts++;
		}	 
	}//end not player
	
	BuildShip(e, p);
}


/*
* This function updates the ship production for the
* argument planet
*/
void BuildShip(EMPIRE *e, PLANET *p)
{
	p->current_ship += (float)(p->factories/3.0);
         
	if (p->current_ship >= kTurnsPerShip)
	{
		SHIP *new_ship;
		p->current_ship=1;
            
		//allocate a new ship
		new_ship = new SHIP;

		if (new_ship!=0)
		{
			//initialize it
			InitShip(e, new_ship, p->x, p->y);
	
			//add it to empire's list
			e->ships.push_back(new_ship);
		}
	} 
}

/*
* This function finds the most advantageous target for
* an empire to sieze.  Larger planets are ranked more 
* highly than smaller ones.  Unoccupied planets are
* ranked more highly than occupied ones.  Occupied 
* planets lose points for having forts.
*/


void FindTargetForEmpire(EMPIRE *e)
{
	//find target with best production to cost ratio
	double score[kNumPlanets];
	double benefit;
	double cost;
	int best_index=-1;
	int best_range;
	int new_range;
	int i=0; 

	for (i; i < kNumPlanets; i++)
	{
		if (gPlanets[i].ruler != e)
		{
			benefit = gPlanets[i].size;	
			cost = gPlanets[i].forts;
			
			if (cost==0)
			{
				score[i] = 1+gPlanets[i].size; //this way planets will go after free planets first
			}
			else
			{
				score[i] = (benefit/(1+cost));
			}

			if (best_index==-1 || score[i] >= score[best_index] )
			{
				//check to see if the new planet is closer
			
				new_range = AvgDist(e, &gPlanets[i]);
				
				if (new_range < best_range || best_index==-1)
				{
					best_range = new_range;
					best_index = i;
				}
			}
			
		}//end not already in empire

	}//end for

	e->current_target =  (PLANET_PTR) &gPlanets[best_index];
}




/**************************************************************
* FUNCTION: DeleteShipsAt
*
* DESCRIPTION: This function deletes all of an empires ships
* whose destination and current position 
* are the argument planet.
*
* PARAMETERS: Ptr to Empire which owns ships, and ptr planet
* where the ships are located
*
* RETURN: None.
***************************************************************/

void DeleteShipsAt(EMPIRE *e, PLANET *p)
{
	int i=0,j,k;

//	 ResetIterator(&(e->ships));

	 k=e->ships.size();

     for (i=0, j=0; i < k; j++, i++) //number of traversals is fixed
     {
//		  SHIP *ship = (SHIP *)GetItem(&(e->ships), j); 
		  SHIP *ship = (SHIP *)GetShip(e->ships, j); 
			
		  /*check cur x,y so ships in transit are not deleted*/

		  if (ship->dest_x == p->x && ship->dest_y == p->y &&
			  ship->cur_x == p->x && ship->cur_y == p->y
			  )
          {
//			DeleteItem(&(e->ships), ship); //remove it from list 
//			delete(ship);
			DeleteShip(e->ships, ship); //remove it from list 

			//list is shorter now			
			j--; 
		  }  
   
     } //end delete ships
}


/*
* Sets the destination for all of the player's 
* ships at sx,sy to dx, dy.  This function will
* be linked into the Palm interface.
*/

void TargetPlayerShips(short sx, short sy, short dx, short dy)
{
	int count=0;
	BOOL bPlayLaunchSound = FALSE;
	//int i=0;
	SHIP *ship=0;
	//ResetIterator(&(gEmpires[0].ships));
	std::list<SHIP*>::iterator i;

	for ( i=gEmpires[0].ships.begin(); i != gEmpires[0].ships.end(); i++ ) 
	{
		 ship = (SHIP *)*i;

		if ( ship->cur_x == sx && ship->cur_y == sy )
		{//found ship at correct planet

			if ( (ship->dest_x == -1 && ship->dest_y == -1) || 
				 (ship->dest_x == sx && ship->dest_y == sy) )
			{//dest not already set
				ship->dest_x = dx;
				ship->dest_y = dy;
				count++;
				bPlayLaunchSound = TRUE;
			}
		}
	}

	if ( bPlayLaunchSound )
	{
		PlayASound("IDR_LAUNCH_WAVE");
	}

}



/********************************************
 * FUNCTION: LoadGame()
 *
 * DESCRIPTION: make sure current lists are cleaned up, 
 *				and user is prompted
 *
 * PARAMETERS: Ref of db to read from
 *
 * RETURN: TRUE if success, FALSE if error
 *
 *********************************************/

 
 


/*******************************************************
* These functions replace the pointers in the structures
* with indicies to facilitate loading and storing.
*******************************************************/

void EmpireToEmpireData(EMPIRE_FILE_DATA *d, int index)
{
	d->defeated = gEmpires[index].defeated;
	d->player = gEmpires[index].player;
}

void PlanetToPlanetData(PLANET_FILE_DATA *d, int index)
{
	d->current_ship = gPlanets[index].current_ship;
	d->factories = gPlanets[index].factories;
	d->forts = gPlanets[index].forts;
	d->built_this_turn = gPlanets[index].built_this_turn;

	if (gPlanets[index].ruler==NULL)
	{
		d->ruler_index = -1;
	}
	else
	{
		d->ruler_index = (short)(gPlanets[index].ruler->index);
	}
	
	d->size = gPlanets[index].size;
	d->x = gPlanets[index].x;
	d->y = gPlanets[index].y;
}


void ShipToShipData(SHIP *p, SHIP_FILE_DATA *d)
{
	d->cur_x = p->cur_x;
	d->cur_y = p->cur_y;
	d->dest_x = p->dest_x;
	d->dest_y = p->dest_y;
	d->owner_index = p->owner->index;
}

/*
* These utility function convert saved data
* back into game data
*/

void EmpireDataToEmpire(EMPIRE_FILE_DATA *d, int index)
{
	gEmpires[index].defeated = d->defeated;
	gEmpires[index].player = d->player;
	gEmpires[index].index = index;
	
	//Set the empire name
	if (index==0)
	{
		gEmpires[index].name = kPlayerName;
	}
	else
	{
		gEmpires[index].name = kEmpireNames[index-1];
	}
	gEmpires[index].current_target=NULL;
}

void PlanetDataToPlanet(PLANET_FILE_DATA *d, int i)
{
	gPlanets[i].current_ship = d->current_ship;
	gPlanets[i].factories = d->factories;
	gPlanets[i].forts = d->forts;
	gPlanets[i].built_this_turn = d->built_this_turn;
	gPlanets[i].size = d->size;
	gPlanets[i].x = d->x;
	gPlanets[i].y = d->y;
	gPlanets[i].name = kPlanetNames[i];
	
	//Set ruler

	if (d->ruler_index!=-1)
	{
		gPlanets[i].ruler = &gEmpires[d->ruler_index];
		gEmpires[d->ruler_index].planets.push_back(&gPlanets[i]);
	}
	else
	{
		gPlanets[i].ruler=NULL;
	}
}

/*
* This function recreates a ship from a
* record in the saved game
*/

void ShipDataToShip(SHIP_FILE_DATA *d)
{
	SHIP *ship = new SHIP;

	if (ship!=0)
	{
		ship->cur_x = d->cur_x;
		ship->cur_y = d->cur_y;
		ship->dest_x = d->dest_x;
		ship->dest_y = d->dest_y;
		ship->owner = &gEmpires[d->owner_index];
	
		//add it to empire
		gEmpires[d->owner_index].ships.push_back(ship);
	}
}

/*
* This function counts the total number of ships in 
* the game
*/

int GetShipCount()
{
	int i=0;
	int count=0;

	for (; i <= gNumEmpires; i++)
		count += gEmpires[i].ships.size();
	
	return count;
}





void UpdateGame()
{
int i;
 UpdateEmpires(); //this will produce ships, assign them targets
 UpdateAllShips(); //move ships
 ExecuteAllAttacks();//make them attack

 for (i=0; i < kNumPlanets; i++)
 {
 	gPlanets[i].built_this_turn=0;
 }
	
}


void DeleteSomeShips(EMPIRE *ruler, PLANET *planet, int how_many)
{
int k;
SHIP *ship=0;

	for (k=0; k < how_many; k++)
	{
//		ResetIterator(&(ruler->ships));
		std::list<SHIP*>::iterator n;

		for (n=ruler->ships.begin(); n != ruler->ships.end(); n++)
		{
			ship = (SHIP*)*n;
								
			if ( 
				(ship->cur_x == planet->x && ship->cur_y == planet->y) 												
				&&
				(	
					(ship->dest_x == planet->x && ship->dest_y == planet->y) 
					||
					(ship->dest_x == -1 && ship->dest_y == -1) 
				)
			   )
			{
//				DeleteItem(&(ruler->ships), ship);
//				delete(ship);
				DeleteShip( ruler->ships, ship );
				break; //reset iterator, delete next ship
			}
		}	
	}

}


/************************************************************
* This function computes the average distance to planet from 
* the other planets in the empire
*************************************************************/
int AvgDist(EMPIRE *e, PLANET *p)
{
	PLANET *temp;
	int dx,dy;	
	int total=0;
//	ResetIterator(&(e->planets));
	std::list<PLANET*>::iterator i;

	for (i=e->planets.begin(); i != e->planets.end(); i++)
	{
		temp = (PLANET*)*i;
		dx = p->x - temp->x;
		dy = p->y - temp->y;
		total += (dx*dx)+(dy*dy);		
	}

	total /= e->planets.size();
	
	return total;
}


int AttackShipsAt(EMPIRE *e, int num_ships, int opposing_ships, int i)
{
	int kills_per_ship;
					
	//figure out some random amount
	//of damage base on the number of
	//attacking ships
					
	if (e==&gEmpires[0])
		kills_per_ship=1;
	else
	{
		if (rand()%2==0)
			kills_per_ship = (rand()%2)+1;
		else
			kills_per_ship = 1;
	}
					
	if (opposing_ships >= num_ships*kills_per_ship)
	{
		//delete all the attacking ships
		DeleteSomeShips(e, &gPlanets[i], num_ships);
						
		//and the selected number of defending ships
		DeleteSomeShips(gPlanets[i].ruler, &gPlanets[i], min(opposing_ships, kills_per_ship*num_ships));

		num_ships = 0;
					
		//there are no ships left over for a ground attck, so 
		//display the dialog now 
					
												
		if (e == &gEmpires[0])
			AlertAttackFailed(gPlanets[i].ruler->name, gPlanets[i].name);
						
		if (gPlanets[i].ruler == &gEmpires[0])
		 	AlertPlayerAttacked((char*)(e->name), (char*)(gPlanets[i].name));
		 	
	}//end all attacking ships destroyed	
	else
	{  //all defending ships destroyed
	
		//delete some of the attacking ships
		DeleteSomeShips(e, &gPlanets[i], opposing_ships);
						
		//delete all defending ships
		DeleteSomeShips(gPlanets[i].ruler, &gPlanets[i], opposing_ships);
		num_ships -= opposing_ships; 
						
					
		if (num_ships <= 0)
		{
			//there are no ships left over for a ground attck, so 
			//display the dialog now 
							
			if (e == &gEmpires[0])
				AlertAttackFailed(gPlanets[i].ruler->name, gPlanets[i].name);
							
			if (gPlanets[i].ruler == &gEmpires[0])
			 	AlertPlayerAttacked((char*)(e->name), (char*)(gPlanets[i].name));
		}							
	} //end all defending ships destroyed
					
	return num_ships;
}



void AttackPlanet(EMPIRE *e, int num_ships, int i)
{
	int ff;
    int defender_strength;
    EMPIRE *defender;
    int attack_strength;
    
    defender = gPlanets[i].ruler;
          		
	//fudge the computer empires, by occaisionally
	//giving them an extra ship 
	           		
	attack_strength = num_ships;

	if (e!=&gEmpires[0])
	{
		ff = rand()%5; 
				 		
	    if (ff==3)
			ff=1;
		else if (ff==4)
			ff=2;
	
   		attack_strength+=ff;
    }
    					       		      			
    if (gPlanets[i].ruler!=e)
    {//don't attack planets we already own

		if (gPlanets[i].ruler!=NULL)
		{
	
			  defender_strength = gPlanets[i].forts;
			  defender = gPlanets[i].ruler;
   
			  if (attack_strength > defender_strength)
			  {//empire takes planet
					 EmpireTakesPlanet(e,i,defender_strength);

			  }//end attacker takes planet
			   else
			   { //attack failed, produce damage to defenses

				 if (e == &gEmpires[0])
				 	AlertAttackFailed(defender->name, gPlanets[i].name);
				 else if (defender == &gEmpires[0])
				 	AlertPlayerAttacked((char*)(e->name), (char *)( gPlanets[i].name));
						 	 
		         gPlanets[i].forts = (char)(gPlanets[i].forts-attack_strength);

 			 	 //don't have negative forts or factories
				 if (gPlanets[i].forts < 0)
				 {
					 //take the surplus damage and destroy some factories too					 	
				 	gPlanets[i].factories+=gPlanets[i].forts;
				 	gPlanets[i].forts=0;
						 	
				 	if (gPlanets[i].factories<0)
				 		gPlanets[i].factories=0;
					 	
				 }
						 
						 
				 /*destroy all the attackers ships*/

		         DeleteShipsAt(e, &gPlanets[i]);

	           }    
	        }
		    else
			{//planet uninhabited. Claim it, keep the ships
				  
				gPlanets[i].ruler=e;

				//add it to the empire's list
				//AddItem(&(e->planets), &gPlanets[i]);
				e->planets.push_back( &gPlanets[i] );

				if (e==&gEmpires[0])
				{
					AlertPlanetCaptured((char*)(gPlanets[i].name));
				}
				else
				{
					e->current_target=NULL; //need to send ships somewhere else
				}		
			}//end planet uninhabited

       }//end carry out attack

}


void EmpireTakesPlanet(EMPIRE *e, int i, int defender_strength)
{

	EMPIRE *defender = gPlanets[i].ruler;
	
	//only display a dialog if the player was involved.
	
	if (e==&gEmpires[0] || defender==&gEmpires[0])
		AlertPlanetLost(e->name, defender->name, gPlanets[i].name);

	//set new ruler
	
	gPlanets[i].ruler=e;
      
	//add it to planet list

	//AddItem(&(e->planets), &gPlanets[i]);
	e->planets.push_back( &gPlanets[i] );

	//Remove it from defenders list

//	DeleteItem(&(defender->planets),  &gPlanets[i]);
	DeletePlanet( defender->planets,  &gPlanets[i]);
	
	//loop through the attacker's ships and delete the
	//appropriate number, 
	//for the computer, all attacking ships
	//should be deleted?
						
	if (e==&gEmpires[0])
		DeleteSomeShips(e, &gPlanets[i], defender_strength);
	else
	    DeleteShipsAt(e, &gPlanets[i]); //avoid floaters
						 
	//if the defenders owns no more planets, its ships will
	//still keep getting updated. when they attack, they will be
	//deallocated.
   
	if (defender->player==FALSE)
	{
		if (defender->planets.size()==0)
		{//empire has been wiped out
		//GENERATE ALERT

			AlertEmpireDefeated(e->name, defender->name);
			defender->defeated=TRUE;
								
			//delete all the empire's ships
			DeleteShipList(defender->ships);
		}  
	 }
	 else
	 {
		 if (gEmpires[0].planets.size()==0)
		 {
			defender->defeated=TRUE;
			AlertDefeat();
			DeleteShipsAt(e, &gPlanets[i]); //delete computer empire's ships
								
			//delete all the players ships
								
			DeleteShipList(gEmpires[0].ships);
			
			NewGame();

			return; //no point in processing more
		 }
	 }
    			
     gPlanets[i].forts=0;		
   
   	 e->current_target=NULL; //pick new planet to attack


}

/*
void DisplayDBErr(Err err)
{
			 if (err == dmErrAlreadyExists )
					FrmCustomAlert(1900, "dmErrAlreadyExists", " ", " ");
			 else if ( err == dmErrAlreadyOpenForWrites)
					FrmCustomAlert(1900, "dmErrAlreadyOpenForWrites", " ", " ");
			 else if (err == dmErrCantFind)
					FrmCustomAlert(1900, "dmErrCantFind ", " ", " ");
			 else if (err == dmErrCantOpen)
						FrmCustomAlert(1900, "dmErrCantOpen", " ", " ");
			 else if (err == dmErrIndexOutOfRange)
						FrmCustomAlert(1900, "dmErrIndexOutOfRange", " ", " ");
			 else if ( err == dmErrCorruptDatabase )
					FrmCustomAlert(1900, "dmErrCorruptDatabase", " ", " ");
			 else if ( err == dmErrDatabaseOpen  )
					FrmCustomAlert(1900, "dmErrDatabaseOpen ", " ", " ");
			 else if (err == dmErrInvalidParam)
						FrmCustomAlert(1900, "dmErrInvalidParam", " ", " ");
			 else if (err == dmErrMemError)
						FrmCustomAlert(1900, "dmErrMemError", " ", " ");
			 else if (err == dmErrDatabaseNotProtected)
						FrmCustomAlert(1900, "dmErrDatabaseNotProtected", " ", " ");
			 else if (err == dmErrMemError )
						FrmCustomAlert(1900, "dmErrMemError ", " ", " ");
			else if (err == dmErrNoOpenDatabase )
						FrmCustomAlert(1900, "dmErrNoOpenDatabase ", " ", " ");
			else if (err == dmErrNotRecordDB )
						FrmCustomAlert(1900, "dmErrNotRecordDB ", " ", " ");
			else if (err == dmErrNotValidRecord )
						FrmCustomAlert(1900, "dmErrNotValidRecord", " ", " ");
			 else if ( err == dmErrInvalidDatabaseName  )
					FrmCustomAlert(1900, "dmErrInvalidDatabaseName ", " ", " ");
			 else if ( err == memErrChunkLocked  )
					FrmCustomAlert(1900, "memErrChunkLocked ", " ", " ");
			 else if ( err == dmErrNotValidRecord  )
					FrmCustomAlert(1900, "dmErrNotValidRecord ", " ", " ");
			 else if ( err == dmErrReadOnly  )
					FrmCustomAlert(1900, "dmErrReadOnly", " ", " ");
			 else if ( err == dmErrOpenedByAnotherTask  )
					FrmCustomAlert(1900, "dmErrOpenedByAnotherTask", " ", " ");
			 else if ( err == dmErrRecordArchived  )
					FrmCustomAlert(1900, "dmErrRecordArchived", " ", " ");
			 else if ( err == dmErrRecordDeleted   )
					FrmCustomAlert(1900, "dmErrRecordDeleted ", " ", " ");
			 else if ( err == dmErrRecordInWrongCard   )
					FrmCustomAlert(1900, "dmErrRecordInWrongCard ", " ", " ");

}
*/

/*
* This function returns the sum of the number of planets
* controlled by the computer controlled empires. This function
* is used to determine if the player has defeated all the 
* computer empires.
*/


int CountEnemyPlanets()
{
	return gEmpires[1].planets.size() + 
		gEmpires[2].planets.size() +
		gEmpires[3].planets.size();
	
}

void DeleteShipList(std::list<SHIP*> &list)
{
	try
	{
		while (list.size()!=0)
		{
			
			SHIP *ship = GetShip(list, 0 );
			list.remove(ship);
 			delete(ship);
		}
	}
	catch ( exception &e)
	{
		int x=1;
	}
}
		   
SHIP * GetShip( std::list<SHIP*> &l, int j)
{
	int x;
	std::list<SHIP*>::iterator i;
	for (  x=0, i= l.begin(); x < j; i++, x++)
	{
		;
	}

	return (SHIP*) *i;
}

PLANET * GetPlanet( std::list<PLANET*> &l, int j)
{
	int x;
	std::list<PLANET*>::iterator i;
	for (  x=0, i= l.begin(); x < j; i++, x++)
	{
		;
	}

	return (PLANET*) *i;
}

void DeleteShip( std::list<SHIP*> &list, SHIP *ship )
{
	std::list<SHIP*>::iterator i;
	for ( i=list.begin(); i != list.end(); i++ )
	{
		if ( *i == ship )
		{
			SHIP *temp_ptr = *i;
			list.erase(i);
			delete( temp_ptr );
			return;
		}
	}
}

void DeletePlanet( std::list<PLANET*> &list, PLANET *p)
{
	std::list<PLANET*>::iterator i;
	for ( i=list.begin(); i != list.end(); i++ )
	{
		if ( *i == p )
		{
			list.erase(i);
			return;
		}
	}
}

void GenerateRandomSequence(int *array, int size )
{
	int i;
	int temp;
	int cell1, cell2;



	for (i=0; i < size; i++)
	{
		array[i]=i;
	}

	//randomize
	
	for (i=0; i < 20; i++)
	{
		cell1 = rand()%size;
		cell2 = rand()%size;

		temp = array[cell1];
		array[cell1] = array[cell2];
		array[cell2]=temp;
	}
}


void GenerateRandomSequence2(int *array, int from , int to )
{
	int i;
	int temp;
	int cell1, cell2;
	int size = to-from;

	for (i=0; i < size; i++)
		array[i]=from+i;

	//randomize
	
	for (i=0; i < 20; i++)
	{
		cell1 = rand()%size;
		cell2 = rand()%size;

		temp = array[cell1];
		array[cell1] = array[cell2];
		array[cell2]=temp;
	}
}


PLANET * GetPlanet( int x, int y )
{
	for ( int i=0; i < kNumPlanets; i++ )
	{
		if ( gPlanets[i].x == x && gPlanets[i].y == y )
		{
			return &gPlanets[i];
		}
	}

	return 0;
}


//handles the automatic build of forts and factories
void UpdateInfrastructure()
{
	std::list<PLANET *>::iterator it;
	for ( it = gEmpires[0].planets.begin();
		  it != gEmpires[0].planets.end();
		  it++
		)
	{
		PLANET *p = (*it);

		if (gBuildFirst == FACTORIES_FIRST)
		{
			if ( p->factories < p->size )
			{
				p->factories++;
				p->built_this_turn = TRUE;
			}
			else if ( p->forts < p->size )
			{
				p->forts++;
				p->built_this_turn = TRUE;
			}
		}
		else if (gBuildFirst == FORTS_FIRST)
		{
			if ( p->forts < p->size )
			{
				p->forts++;
				p->built_this_turn = TRUE;
			}
			else if ( p->factories < p->size )
			{
				p->factories++;
				p->built_this_turn = TRUE;
			}
		}
		else if (gBuildFirst == ALTERNATE_FIRST)
		{
			if (p->forts <= p->factories)
			{
				if (p->forts < p->size)
				{
					p->forts++;
					p->built_this_turn = TRUE;
				}
			}
			else
			{//factories < forts
				if (p->factories < p->size)
				{
					p->factories++;
					p->built_this_turn = TRUE;
				}
			}

		}
	}
}

//pick a planet in the player's empire
//remove it from the player's list and display a warning
void Revolt()
{
}