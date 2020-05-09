// StarCampaign.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <time.h>
#include <string>
#include <iostream>
#include <fstream>
#include "StarCampaign.h"
#include <Windows.h>
#include <Mmsystem.h>
#include <Commdlg.h>
#include <windowsx.h> //pen macros

LRESULT CALLBACK WindowHandler( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK RulePlanetDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PlanetsDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam);


void AlertAttackFailed(char const *defender, char const *planet);
void AlertPlanetLost(char const *attacker, char const *defender, char const *planet);
void AlertEmpireDefeated(char const *attacker, char const *def);
void AlertPlayerAttacked(char * attacker, char *planet); 
void AlertPlanetCaptured(char *const planet);
void AlertDefeat();
void AlertVictory();
void DrawBoard( HDC hdc );
void InitGraphics( HINSTANCE hInstance, HDC hdc );
void CleanupGraphics( HWND hwnd );
void HandleMouseDown(int x, int y);
void NextTurn();
void HandleMouseUp(int x, int y );
void HandleRightButtonUp( int x, int y );
void HandleMouseMove( int x, int y);
void NewGame();
void Refresh();
void CreateStars();
void SaveGame( void );
void LoadGame( void );
BOOL SaveGame( const char *filename );
BOOL LoadGame( const char *filename );
PLANET *GetPlanet( int x, int y );


const char *szWinName = "EvansWindow";
const int kBoardSize = (kLogicalBoardSquares*kIconSize);


BUILDPREF gBuildFirst = FORTS_FIRST;
BOOL bAutoBuild = TRUE;

HWND hwnd;
HMENU ghMenu;
bool gbGameOver = false;
bool bFirst = true;
BOOL gbEnableSounds = TRUE;
HBITMAP gPlanetBitmaps[3];
HBITMAP gEmpireBitmaps[4];
HBITMAP gIntroBmp;

HDC gPlanetDCs[3];
HDC gEmpireDCs[4];
HDC		gOffscreen;
HDC		gIntroDC;

extern PLANET gPlanets[kNumPlanets];
extern EMPIRE gEmpires[5];
extern int gNumEmpires;

int gMouseX = -1;
int gMouseY = -1;
int gTimer = 0;


struct STAR
{
	int x, y;
};

const int kNumStars = 150;
STAR gStars[kNumStars];

HINSTANCE ghInst;

BOOL bDragging = FALSE;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	ghInst=hInstance;

	srand(clock());

	MSG msg;
	WNDCLASSEX wcl;

	wcl.hInstance = hInstance;
	wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = WindowHandler;
	wcl.style = 0; //default style
	wcl.cbSize = sizeof( WNDCLASSEX );
	wcl.hIcon = LoadIcon( NULL, MAKEINTRESOURCE( IDI_APP_ICON ) );
	wcl.hIconSm = LoadIcon( NULL, MAKEINTRESOURCE( IDI_APP_ICON ) );
	wcl.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcl.lpszMenuName = "IDR_MENU";

	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground =  (HBRUSH)GetStockObject(WHITE_BRUSH);

	//register the window

	if (!RegisterClassEx(&wcl)) return 0;

	ghMenu = LoadMenu( hInstance, MAKEINTRESOURCE(IDR_MENU) );

	hwnd = CreateWindow( szWinName, 
		"Star Campaign v1.2", 
		WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION,
		//WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		kBoardSize,
		kBoardSize + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU),
		HWND_DESKTOP, //no parent
		ghMenu, //no menu
		hInstance,
		NULL, // no args
		);

	InitGraphics( hInstance, GetDC( hwnd ) );
	CreateGalaxy();

	
	ShowWindow( hwnd, SW_SHOW );
	UpdateWindow( hwnd );



	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CleanupGraphics( hwnd );
	return 0;
}

LRESULT CALLBACK WindowHandler( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch( msg )
	{
		case WM_PAINT:

			hdc = BeginPaint( hwnd, &ps );

			if ( bFirst )
			{
				//draw intro screen
				BitBlt( hdc, 0, 0, kBoardSize, kBoardSize, gIntroDC, 0, 0, SRCCOPY );
				//start timer
				//PostMessage( hwnd, WM_TIMER, 0 , 0 );
				gTimer = ::SetTimer( hwnd, WM_TIMER, 5000, 0 );
			}
			else
			{

				Refresh();
			}

			EndPaint(hwnd, &ps);
			return true;
			break;

		case WM_TIMER:
			KillTimer( hwnd , gTimer );
			bFirst = false;
			Refresh();
			return 0;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
		 	return 0;
			break;

		case WM_LBUTTONDOWN:
			HandleMouseDown( LOWORD(lParam), HIWORD(lParam) );
			return 0;
			break;

		case WM_LBUTTONUP:
			HandleMouseUp( LOWORD(lParam), HIWORD(lParam) );
			return 0;
			break;

		case WM_RBUTTONDOWN:
			HandleRightButtonUp( LOWORD(lParam), HIWORD(lParam) );
			return 0;
			break;

		case WM_MOUSEMOVE:
			if ( bDragging )
			{
				HandleMouseMove( LOWORD(lParam), HIWORD(lParam) );
				return 0;

			}
			else
			{
				return DefWindowProc( hwnd, msg, wParam, lParam );
			}
			break;

		case WM_KEYUP:
			if ( wParam == VK_SPACE )
			{
				NextTurn();
			}
			return 0;
			break;


		case WM_COMMAND:
			switch(LOWORD(wParam)) 
			{
	            case IDM_NEW:
					NewGame();
					break;

				case IDM_SAVE:
					SaveGame();	
					break;

				case IDM_LOAD:
					LoadGame();	
					break;

				case IDM_OPTIONS_SOUND:
					if ( gbEnableSounds )
					{
						gbEnableSounds = FALSE;
						CheckMenuItem( ghMenu, IDM_OPTIONS_SOUND, MF_UNCHECKED );
					}
					else
					{
						gbEnableSounds = TRUE;
						CheckMenuItem( ghMenu, IDM_OPTIONS_SOUND, MF_CHECKED );
					}
					break;

				case IDM_OPTIONS_PLANETS:
					DialogBox(ghInst, MAKEINTRESOURCE(IDD_PLANETS), hwnd, PlanetsDlgProc);
					break;


				case IDM_HELP_ABOUT:
					DialogBox(ghInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
					break;

				case IDM_EXIT:
//				exit(0);
				PostQuitMessage(0);
				return 0;
				break;

			}
			return 0;
			break;

			

		default:
			return DefWindowProc( hwnd, msg, wParam, lParam );
			break;
	}

}


void AlertAttackFailed(char const *defender, char const *planet)
{
	std::string text;
	text = "The " + std::string(defender) + " have repelled your attack on " + std::string(planet) + ".";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}

void AlertPlanetLost(char const *attacker, char const *defender, char const *planet)
{
	std::string text;
	text = "The " + std::string(attacker) + " have defeated the " + std::string(defender) + " at " + std::string(planet) + ".";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );

}

void AlertEmpireDefeated(char const *attacker, char const *def)
{
	std::string text;
	text = "The " + std::string(def) + " have anihilated by the " + std::string(attacker) + ".";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}

void AlertPlayerAttacked(char * attacker, char *planet)
{
	std::string text;
	text = "An attack on " + std::string(planet) + " by the " + std::string(attacker) + " has failed.";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}

void AlertPlanetCaptured(const char *planet)
{
	std::string text;
	text = "Your forces have captured " + std::string(planet) + ".";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}

void AlertDefeat()
{
	std::string text;
 	text = "Your empire has been crushed!";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}

void AlertVictory()
{
	std::string text;
	text = "You have conquered the galaxy!";
	MessageBox( hwnd, text.c_str(), "Alert", MB_OK );
}


void InitGraphics( HINSTANCE hInstance, HDC hdc )
{
	CreateStars();

	gPlanetBitmaps[0] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_SMALL_PLANET ) );
	gPlanetBitmaps[1] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_MEDIUM_PLANET ) );
	gPlanetBitmaps[2] = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_LARGE_PLANET ) );

	gEmpireBitmaps[0] =  LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_EMPIRE_1 ) );
	gEmpireBitmaps[1] =  LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_EMPIRE_2 ) );
	gEmpireBitmaps[2] =  LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_EMPIRE_3 ) );
	gEmpireBitmaps[3] =  LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_EMPIRE_4 ) );

	gIntroBmp = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_INTRO ) );

	//load the bitmaps into device contexts



	//planets
	gPlanetDCs[0] = CreateCompatibleDC( hdc );
	gPlanetDCs[1] = CreateCompatibleDC( hdc );
	gPlanetDCs[2] = CreateCompatibleDC( hdc );
	
	SelectObject( gPlanetDCs[0], gPlanetBitmaps[0]);
	SelectObject( gPlanetDCs[1], gPlanetBitmaps[1]);
	SelectObject( gPlanetDCs[2], gPlanetBitmaps[2]);



	//empires

	gEmpireDCs[0] = CreateCompatibleDC( hdc );
	gEmpireDCs[1] = CreateCompatibleDC( hdc );
	gEmpireDCs[2] = CreateCompatibleDC( hdc );
	gEmpireDCs[3] = CreateCompatibleDC( hdc );
	
	SelectObject( gEmpireDCs[0], gEmpireBitmaps[0]);
	SelectObject( gEmpireDCs[1], gEmpireBitmaps[1]);
	SelectObject( gEmpireDCs[2], gEmpireBitmaps[2]);
	SelectObject( gEmpireDCs[3], gEmpireBitmaps[3]);

	//intro

	gIntroDC = CreateCompatibleDC( hdc );
	gOffscreen = CreateCompatibleDC( hdc );
	HBITMAP bmp = CreateCompatibleBitmap( hdc, kBoardSize, kBoardSize );
	SelectObject( gOffscreen, bmp );
	SelectObject( gIntroDC, gIntroBmp );
	
}

void CleanupGraphics( HWND hwnd )
{
	//release dcs;
	ReleaseDC( hwnd, gPlanetDCs[0] );
	ReleaseDC( hwnd, gPlanetDCs[1] );
	ReleaseDC( hwnd, gPlanetDCs[2] );

	ReleaseDC( hwnd, gEmpireDCs[0] );
	ReleaseDC( hwnd, gEmpireDCs[1] );
	ReleaseDC( hwnd, gEmpireDCs[2] );
	ReleaseDC( hwnd, gEmpireDCs[3] );

	ReleaseDC( hwnd, gIntroDC );
}


void DrawBoard( HDC hdc )
{
	//paint the background black
	RECT rect;
	HBRUSH black_brush = ( HBRUSH )GetStockObject( BLACK_BRUSH );
//	HBRUSH old_Brush = ( HBRUSH )SelectObject( GetDC( hwnd ), black_brush );
	rect.bottom = (kLogicalBoardSquares*kIconSize);
	rect.top = 0;
	rect.left = 0;
	rect.right = (kLogicalBoardSquares*kIconSize);
	char str[10];

	HFONT font = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
	FillRect( hdc, &rect, black_brush );

	::SetBkColor( hdc, RGB( 0, 0, 0 ) );
	::SetBkMode( hdc, TRANSPARENT );
	::SetTextColor( hdc, RGB( 255, 255, 255) );
	SelectObject( hdc, font );

	::SelectObject( hdc, GetStockObject( WHITE_BRUSH ) );
	for ( int j=0; j < kNumStars; j++ )
	{
		SetPixel( hdc, gStars[j].x, gStars[j].y, RGB( 255,255,255 ) );
	}

	for ( int i=0; i < kNumPlanets; i++ )
	{
		int x = (gPlanets[i].x * kIconSize);
		int y = (gPlanets[i].y * kIconSize);

		if ( gPlanets[i].size > 0 && gPlanets[i].size <= 3) 
		{
			BitBlt(
				hdc, 
				gPlanets[i].x * kIconSize,
				gPlanets[i].y * kIconSize,
				kIconSize,
				kIconSize,
				gPlanetDCs[gPlanets[i].size-1],
				0,
				0,
				SRCCOPY
				);


			if ( gPlanets[i].ruler != NULL )
			{//draw empire icon
				BitBlt( 
						hdc,
						x + (kIconSize/2 - kIconSize/4),
						y - kIconSize/2,
						kIconSize/2, 
						kIconSize/2,
						gEmpireDCs[ gPlanets[i].ruler->index ],
						0,
						0,
						SRCCOPY
						);
			}

		}
		

			//if the player has any ships on station, display the number
		//to the right of the planet
		
		int ships =  CountShipsToDraw(&gEmpires[0], &gPlanets[i]);

		if (ships >  0)
		{
			sprintf(str, "%d", ships );
			::TextOut( hdc, x+kIconSize, y + kIconSize/2, str, strlen( str ) );
		}


		::TextOut( hdc, x+(kIconSize*2)/3, y+kIconSize*(10/16), gPlanets[i].name, strlen( gPlanets[i].name ) );

	}//end for
}


void HandleMouseDown(int x, int y)
{
	//determine if the pen was clicked on a planet
	//each string must be kept in a seperate buffer
	//because the text fields in the dialog for
	//will reference them by pointer without making 
	//a copy.
	int i;
	x = x/kIconSize;
	y = y/kIconSize;

//	gMouseX = x;
//	gMouseY = y;
	
	//check to see if the mouse  was clicked to the right
	//of a planet with ships
		
	for (i=0; i < kNumPlanets; i++)
	{
		//clicked the right of a planet
		if ( x-1 == gPlanets[i].x && y == gPlanets[i].y)
		{
			{
				if (CountShipsToDraw(&gEmpires[0], &gPlanets[i]) > 0)
				{
					gMouseX = x-1;
					gMouseY = y;
					bDragging = TRUE;
					Refresh();
					return;
				}	
			}
		}
	}		


	//no ships were dragged

}

void NextTurn()
{
//	if (InNextButton(x,y))
	{
		int num_enemy_planets;
	 
		UpdateGame();
		Refresh();
			
		//see if the player has been defeated
		
		num_enemy_planets = CountEnemyPlanets();
		
		if (gEmpires[0].planets.size()==kNumPlanets || 
			gEmpires[0].planets.size()==0 || 
			num_enemy_planets==0)
		{//game is over
			
			if (gEmpires[0].planets.size()==kNumPlanets || num_enemy_planets==0)
			{
				AlertVictory();
			}
				
			NewGame();

		}
			
		return;
				
	}	
	

}

void HandleMouseUp( int x, int y )
{
	int i;
	x = x/kIconSize;
	y = y/kIconSize;

	//check next button
	
	//check planets
	
	for (i=0; i < kNumPlanets; i++)
	{
		if (gPlanets[i].x == x && gPlanets[i].y == y)
		{//planet at selected location		
			TargetPlayerShips(gMouseX, gMouseY, x, y);			
		}
	}

	bDragging = FALSE;
	Refresh();
}


void HandleRightButtonUp( int x, int y )
{
	x/=kIconSize;
	y/=kIconSize;

	for ( int i=0; i < kNumPlanets; i++)
	{
		if ( gPlanets[i].x == x  && gPlanets[i].y == y )
		{
			if ( gPlanets[i].ruler != NULL  &&
				 gPlanets[i].ruler == &gEmpires[0]
				 )
			{

				gMouseX = x;
				gMouseY = y;

				//display rule planet
				DialogBox(ghInst, MAKEINTRESOURCE(IDD_RULE_PLANET), hwnd, RulePlanetDlgProc);

			}
			else
			{	//display planet info

			}
			return;
		}

	}
}

BOOL CALLBACK RulePlanetDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[56];
	PLANET *p =0;

	switch( message )
	{
	  case WM_INITDIALOG:
		p = GetPlanet( gMouseX, gMouseY );
  	    if ( p!= NULL )
		{

			sprintf( buf, "Planet: %s", p->name );
			SetDlgItemText( hwnd, IDC_EDIT_PLANET_NAME, buf);

			sprintf( buf, "%d", p->factories );
			SetDlgItemText( hwnd, IDC_EDIT_NUM_FACTORIES, buf);

			sprintf( buf, "%d", p->forts );
			SetDlgItemText( hwnd, IDC_EDIT_NUM_FORTS, buf);

		}
		else
		{
			SetDlgItemText( hwnd, IDC_EDIT_NUM_FORTS, "NULL");
		}


		if ( p->built_this_turn == 1 )
		{
			EnableWindow( GetDlgItem( hwnd, IDC_FACTORY_BUTTON ), false );
			EnableWindow( GetDlgItem( hwnd, IDC_FORT_BUTTON ), false );
		}

		if ( p->forts == p->size )
		{
			EnableWindow( GetDlgItem( hwnd, IDC_FORT_BUTTON ), false );
		}

		if ( p->factories == p->size )
		{
			EnableWindow( GetDlgItem( hwnd, IDC_FACTORY_BUTTON ), false );
		}

		return TRUE;
		break;

	  case WM_QUIT: //from close push button
	  case WM_CLOSE:
		  
		   EndDialog(hwnd, (LOWORD(wParam)));
		   return TRUE;
		   break;


	  case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
            {
                if (LOWORD(wParam) == IDOK)
				{
					EndDialog(hwnd, (LOWORD(wParam)));
					return TRUE;
				}
                else if (LOWORD(wParam) == IDC_FACTORY_BUTTON)
				{
					p = GetPlanet( gMouseX, gMouseY );
  				    if ( p!= NULL )
					{
						p->factories++;
						sprintf( buf, "%d", p->factories );
						SetDlgItemText( hwnd, IDC_EDIT_NUM_FACTORIES, buf);
					}


					EnableWindow( GetDlgItem( hwnd, IDC_FACTORY_BUTTON ), false );
					EnableWindow( GetDlgItem( hwnd, IDC_FORT_BUTTON ), false );
					p->built_this_turn = true;

				 	return TRUE;
				}
                else if (LOWORD(wParam) == IDC_FORT_BUTTON)
				{
					PLANET *p = GetPlanet( gMouseX, gMouseY );
  				    if ( p!= NULL )
					{
						p->forts++;
						sprintf( buf, "%d", p->forts );
						SetDlgItemText( hwnd, IDC_EDIT_NUM_FORTS, buf);
						p->built_this_turn = true;

					}

					EnableWindow( GetDlgItem( hwnd, IDC_FACTORY_BUTTON ), false );
					EnableWindow( GetDlgItem( hwnd, IDC_FORT_BUTTON ), false );
				 
					
				 	return TRUE;
				}
				
            }
 	 
			break;

	}//end switch


	return 0;
}

void NewGame()
{
	EndGame();
	CreateStars();
	CreateGalaxy();
	Refresh();
}


BOOL CALLBACK AboutDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	switch( message )
	{

		case WM_COMMAND:
			EndDialog(hwnd, (LOWORD(wParam)));
			return 0;
			break;

		default:
			return 0;
			break;
	}

}


BOOL CALLBACK PlanetsDlgProc(HWND hwnd,UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND btn;

	switch ( message )
	{
		case WM_INITDIALOG:
			//set up the initial dialog state
			
			btn = GetDlgItem(hwnd, IDC_BUILD_CHK );
			
			if (bAutoBuild == TRUE)
			{
				SendMessage(btn, BM_SETCHECK, BST_CHECKED, 0 );
			//	SendMessage(GetDlgItem(hwnd, IDC_FORTS_RADIO), BM_SETSTATE, WM_ENABLE, 1);	
			//	SendMessage(GetDlgItem(hwnd, IDC_FACTORIES_RADIO), BM_SETSTATE,WM_ENABLE, 1 );
			//	SendMessage(GetDlgItem(hwnd, IDC_ALTERNATE_RADIO), BM_SETSTATE, WM_ENABLE, 1 );

				EnableWindow((HWND) GetDlgItem(hwnd, IDC_FORTS_RADIO), TRUE); 
				EnableWindow((HWND) GetDlgItem(hwnd,IDC_FACTORIES_RADIO), TRUE); 
				EnableWindow((HWND) GetDlgItem(hwnd,IDC_ALTERNATE_RADIO), TRUE); 

				//check the first radio button
				SendMessage(GetDlgItem(hwnd, IDC_FORTS_RADIO), BM_SETCHECK, BST_CHECKED, 1 );

			}
			else
			{
				SendMessage(btn, BM_SETCHECK, BST_UNCHECKED, 0 );

				//disable the radio buttons
//				SendMessage(GetDlgItem(hwnd, IDC_FORTS_RADIO), BM_SETSTATE, WM_ENABLE, 0);	
//				SendMessage(GetDlgItem(hwnd, IDC_FACTORIES_RADIO), BM_SETSTATE,WM_ENABLE, 0 );
//				SendMessage(GetDlgItem(hwnd, IDC_ALTERNATE_RADIO), BM_SETSTATE, WM_ENABLE, 0 );

				EnableWindow((HWND) GetDlgItem(hwnd, IDC_FORTS_RADIO), FALSE); 
				EnableWindow((HWND) GetDlgItem(hwnd,IDC_FACTORIES_RADIO), FALSE); 
				EnableWindow((HWND) GetDlgItem(hwnd,IDC_ALTERNATE_RADIO), FALSE); 


			}

			//set up the radio buttons
			
			
			return 0;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
            {
                if (LOWORD(wParam) == IDOK)
				{
					EndDialog(hwnd, (LOWORD(wParam)));
					return TRUE;
				}
				else if (LOWORD(wParam) == IDCANCEL)
				{
					EndDialog(hwnd, (LOWORD(wParam)));
					return TRUE;
				}
                else if (LOWORD(wParam) == IDC_BUILD_CHK)
				{
					if (bAutoBuild == TRUE)
					{
						
						EnableWindow((HWND) GetDlgItem(hwnd, IDC_FORTS_RADIO), FALSE); 
						EnableWindow((HWND) GetDlgItem(hwnd,IDC_FACTORIES_RADIO), FALSE); 
						EnableWindow((HWND) GetDlgItem(hwnd,IDC_ALTERNATE_RADIO), FALSE); 

						bAutoBuild = FALSE;
					}
					else
					{	
						EnableWindow((HWND) GetDlgItem(hwnd, IDC_FORTS_RADIO), TRUE); 
						EnableWindow((HWND) GetDlgItem(hwnd,IDC_FACTORIES_RADIO), TRUE); 
						EnableWindow((HWND) GetDlgItem(hwnd,IDC_ALTERNATE_RADIO), TRUE); 

						bAutoBuild = TRUE;
					}

				 	return TRUE;
				}
                else if (LOWORD(wParam) == IDC_FORTS_RADIO)
				{
					SendMessage(GetDlgItem(hwnd, IDC_FORTS_RADIO), BM_SETCHECK, BST_CHECKED, 0 );
					gBuildFirst = FORTS_FIRST;
				 	return TRUE;
				}
                else if (LOWORD(wParam) == IDC_FACTORIES_RADIO)
				{
					SendMessage(GetDlgItem(hwnd, IDC_FACTORIES_RADIO), BM_SETCHECK, BST_CHECKED, 0 );
					gBuildFirst = FACTORIES_FIRST;
				 	return TRUE;
				}
                else if (LOWORD(wParam) == IDC_ALTERNATE_RADIO)
				{
					SendMessage(GetDlgItem(hwnd, IDC_ALTERNATE_RADIO), BM_SETCHECK, BST_CHECKED, 0 );
					gBuildFirst = ALTERNATE_FIRST;
				 	return TRUE;
				}
				
            }

			return 0;
			break;

		case WM_QUIT: //from close push button
		case WM_CLOSE:	  
		   EndDialog(hwnd, (LOWORD(wParam)));
		   return TRUE;
		   break;

		default:
			return 0;
			break;
	}
}

void CreateStars()
{
	for ( int i=0; i < kNumStars; i++)
	{
		gStars[i].x = rand()%kBoardSize;
		gStars[i].y = rand()%kBoardSize;
	}

}

//opens the dialog to get the file name
//then calls the other save game function
void SaveGame()
{
	char buf[256];
	sprintf(buf, "savedgame.sav");


	OPENFILENAME ofn;
	memset( &ofn, 0, sizeof( ofn ) );
	ofn.lStructSize = sizeof( ofn );
	ofn.hInstance = NULL;
	ofn.lpstrFilter = ".sav";
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = 256;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER;
  	 
	if ( GetSaveFileName( &ofn ) )
	{
		SaveGame( ofn.lpstrFile );
	}

}

BOOL SaveGame( const char *filename )
{
	int offset=0;
	int i=0;
	int j=0;
	int num_ships;
	PLANET_FILE_DATA pfd;
	EMPIRE_FILE_DATA efd;
	SHIP_FILE_DATA sfd;
	SHIP *ship;
	int index=0;
	
	num_ships = GetShipCount();

	/*
	//DONT HAVE TO WORRY ABOUT BLOCK SIZE

	block_size = sizeof(EMPIRE_FILE_DATA)*(gNumEmpires+1) + 
				 sizeof(PLANET_FILE_DATA)*kNumPlanets + 
				 sizeof(SHIP_FILE_DATA)*num_ships + 
				 sizeof(num_ships) +
				 sizeof(gNumEmpires) + 
				 sizeof(int);

		*/
	//get record 0 (prev saved game), and 
	//delete it
	
	try
	{
		
		std::fstream fs( filename, std::ios::out );
	
		//store all planet info, always 10 planets
	
		for (i=0; i < kNumPlanets; i++)
		{
			PlanetToPlanetData(&pfd, i);			
			fs.write( (const char *)&pfd, sizeof( PLANET_FILE_DATA ) );
//			offset += sizeof(PLANET_FILE_DATA);
		}
	
		//write num empires		
		//DmWrite(block, offset, &gNumEmpires, sizeof(gNumEmpires));
		fs.write( (const char*)&gNumEmpires, sizeof( gNumEmpires ) );
			
		offset += sizeof(gNumEmpires);

		//write empire data

		for (i=0; i <= gNumEmpires; i++)
		{
			EmpireToEmpireData(&efd,i);			
			//DmWrite(block, offset, &efd, sizeof(efd));
			fs.write( (const char *)&efd, sizeof( efd ) );
			offset += sizeof(EMPIRE_FILE_DATA);
		}

		//write number of ships

		//DmWrite( block, offset,&num_ships, sizeof( num_ships ) );
		fs.write( (const char*)&num_ships, sizeof( num_ships ) );
		offset += sizeof( num_ships );

		//write all ships

		for (i=0; i <= gNumEmpires; i++)
		{//each empire

			std::list< SHIP* >::iterator it;

			for ( it = gEmpires[i].ships.begin(); it != gEmpires[i].ships.end(); it++)
			{
				ship = (SHIP*)*it;

				ShipToShipData(ship, &sfd);			
				//DmWrite(block, offset, &sfd, sizeof(sfd));
				fs.write( (const char *) &sfd, sizeof( sfd ) );
				offset += sizeof( SHIP_FILE_DATA );
			}
		}
	
 		fs.close();	
	}
	catch ( exception &e )
	{
	 	return FALSE;
	}

	return TRUE;
	 

}

void LoadGame()
{
	char buf[256];
	sprintf(buf, "savedgame.sav");


	OPENFILENAME ofn;
	memset( &ofn, 0, sizeof( ofn ) );
	ofn.lStructSize = sizeof( ofn );
	ofn.hInstance = NULL;
	ofn.lpstrFilter = ".sav";
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = buf;
	ofn.nMaxFile = 256;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER;

	if ( GetOpenFileName( &ofn ) )
	{
		EndGame(); //delete all the existing lists
		LoadGame( ofn.lpstrFile );
	}

}

BOOL LoadGame( const char *filename )
{
	try
	{
		int i=0;
		int num_ships;
		PLANET_FILE_DATA pfd;
		SHIP_FILE_DATA sfd;
		EMPIRE_FILE_DATA efd;
		int offset=0;
	
		std::ifstream is( filename, std::ios::in );

		//read planets
		
		for (i=0; i < kNumPlanets; i++)
		{
			is.read( (char*)&pfd, sizeof(pfd) );
			offset += sizeof(pfd);
			PlanetDataToPlanet(&pfd, i);
		}

		//read number of empires

		is.read((char*)&gNumEmpires, sizeof(gNumEmpires));
		offset += sizeof(gNumEmpires);

		//read the empires

		for (i=0; i <= gNumEmpires; i++)
		{
			is.read((char*)&efd, sizeof(efd));
			offset += sizeof(EMPIRE_FILE_DATA);
			EmpireDataToEmpire(&efd, i);
		}

		//read number of ships

		is.read((char*)&num_ships,  sizeof(num_ships));
		offset += sizeof(num_ships);	

		for (i=0; i < num_ships; i++)
		{
			is.read((char*) &sfd, sizeof(sfd));
			offset += sizeof(sfd);	
			ShipDataToShip(&sfd);
		}

		is.close();
	}
	catch ( exception &e )
	{
		return FALSE;
	}

	return TRUE;
}

void PlayASound( LPCSTR szResName )
{
	try
	{ 
		if ( gbEnableSounds )
		{
			PlaySound( szResName, ghInst, SND_RESOURCE | SND_ASYNC );
		}
	}
	catch ( exception & e )
	{
	}

}

void HandleMouseMove(int x, int y)
{
	POINT pt;
	DrawBoard(gOffscreen);	

	HPEN hpen = (HPEN) GetStockPen( WHITE_PEN );
	
	HPEN oldPen = (HPEN)SelectObject( gOffscreen, (HPEN)hpen );

	::MoveToEx(gOffscreen, gMouseX*kIconSize + kIconSize/2, gMouseY*kIconSize  + kIconSize/2, &pt);
	LineTo(gOffscreen, x, y);
	SelectObject( gOffscreen, (HPEN)oldPen );


	BitBlt( GetDC( hwnd ), 0, 0, kBoardSize, kBoardSize, gOffscreen, 0, 0, SRCCOPY );		 
 
}

void Refresh()
{
	DrawBoard( gOffscreen );
	BitBlt( GetDC( hwnd ), 0, 0, kBoardSize, kBoardSize, gOffscreen, 0, 0, SRCCOPY );		 
}

