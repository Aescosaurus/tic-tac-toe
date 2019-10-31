// TicTacToe.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TicTacToe.h"
#include <windowsx.h>
#include <string>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass( HINSTANCE hInstance );
BOOL                InitInstance( HINSTANCE,int );
LRESULT CALLBACK    WndProc( HWND,UINT,WPARAM,LPARAM );
INT_PTR CALLBACK    About( HWND,UINT,WPARAM,LPARAM );

int APIENTRY wWinMain( _In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow )
{
	UNREFERENCED_PARAMETER( hPrevInstance );
	UNREFERENCED_PARAMETER( lpCmdLine );

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW( hInstance,IDS_APP_TITLE,szTitle,MAX_LOADSTRING );
	LoadStringW( hInstance,IDC_TICTACTOE,szWindowClass,MAX_LOADSTRING );
	MyRegisterClass( hInstance );

	// Perform application initialization:
	if( !InitInstance( hInstance,nCmdShow ) )
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators( hInstance,MAKEINTRESOURCE( IDC_TICTACTOE ) );

	MSG msg;

	// Main message loop:
	while( GetMessage( &msg,nullptr,0,0 ) )
	{
		if( !TranslateAccelerator( msg.hwnd,hAccelTable,&msg ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	return ( int )msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass( HINSTANCE hInstance )
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof( WNDCLASSEX );

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon( hInstance,MAKEINTRESOURCE( IDI_TICTACTOE ) );
	wcex.hCursor = LoadCursor( nullptr,IDC_ARROW );
	wcex.hbrBackground = ( HBRUSH )( GetStockObject( GRAY_BRUSH ) );
	wcex.lpszMenuName = MAKEINTRESOURCEW( IDC_TICTACTOE );
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon( wcex.hInstance,MAKEINTRESOURCE( IDI_SMALL ) );

	return RegisterClassExW( &wcex );
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance( HINSTANCE hInstance,int nCmdShow )
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW( szWindowClass,szTitle,WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,0,CW_USEDEFAULT,0,nullptr,nullptr,hInstance,nullptr );

	if( !hWnd )
	{
		return FALSE;
	}

	ShowWindow( hWnd,nCmdShow );
	UpdateWindow( hWnd );

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//

static constexpr int cellSize = 100;
HBRUSH hbr1;
HBRUSH hbr2;
int playerTurn = 1;
int gameBoard[9] = { 0 };
int winner = 0;
int wins[3];

bool GetGameBoardRect( HWND hWnd,RECT* pRect )
{
	RECT rc;
	if( GetClientRect( hWnd,&rc ) )
	{
		const int width = rc.right - rc.left;
		const int height = rc.bottom - rc.top;

		pRect->left = ( width - cellSize * 3 ) / 2;
		pRect->top = ( height - cellSize * 3 ) / 2;
		pRect->right = pRect->left + cellSize * 3;
		pRect->bottom = pRect->top + cellSize * 3;

		return( true );
	}

	SetRectEmpty( pRect );
	return( false );
}

void DrawLine( HDC hdc,int x1,int y1,int x2,int y2 )
{
	MoveToEx( hdc,x1,y1,nullptr );
	LineTo( hdc,x2,y2 );
}

int GetCellNumberFromPoint( HWND hWnd,int x,int y )
{
	POINT pt{ x,y };
	RECT rc;

	if( GetGameBoardRect( hWnd,&rc ) )
	{
		if( PtInRect( &rc,pt ) )
		{
			// User clicked inside game board.
			// Normalize (0 to cellSize * 3).
			x = pt.x - rc.left;
			y = pt.y - rc.top;

			const int column = x / cellSize;
			const int row = y / cellSize;

			// Convert to index (0 to 8).
			return( row * 3 + column );
		}
	}
	return( -1 ); // Outside game board = failure.
}

bool GetCellRect( HWND hWnd,int index,RECT* pRect )
{
	RECT rcBoard;

	SetRectEmpty( pRect );

	if( index >= 0 && index <= 8 &&
		GetGameBoardRect( hWnd,&rcBoard ) )
	{
		// Convert index from 0 to 8 to x,y pair.
		const int x = index % 3; // Row number.
		const int y = index / 3; // Column number.

		pRect->left = rcBoard.left + x * cellSize + 1;
		pRect->top = rcBoard.top + y * cellSize + 1;
		pRect->right = pRect->left + cellSize - 1;
		pRect->bottom = pRect->top + cellSize - 1;

		return( true );
	}

	return( false );
}

// Returns:
//  0 - No winner.
//  1 - Player 1 wins.
//  2 - Player 2 wins.
//  3 - Cat's game.
int GetWinner( int wins[3] )
{
	const int cells[] = { 0,1,2,3,4,5,6,7,8,
		0,3,6,1,4,7,2,5,8,
		0,4,8,2,4,6 };

	// Check for winner.
	for( int i = 0; i < ARRAYSIZE( cells ); i += 3 )
	{
		if( gameBoard[cells[i]] != 0 &&
			gameBoard[cells[i]] == gameBoard[cells[i + 1]] &&
			gameBoard[cells[i]] == gameBoard[cells[i + 2]] )
		{
			// We have a winner!
			wins[0] = cells[i];
			wins[1] = cells[i + 1];
			wins[2] = cells[i + 2];

			return( gameBoard[cells[i]] );
		}
	}

	// See if we have any cells left empty.
	for( int i = 0; i < ARRAYSIZE( gameBoard ); ++i )
	{
		// Continue playing.
		if( gameBoard[i] == 0 ) return( 0 );
	}

	return( 3 );
}

void ShowTurn( HWND hWnd,HDC hdc )
{
	static const WCHAR szTurn1[] = L"Turn: Player 1";
	static const WCHAR szTurn2[] = L"Turn: Player 2";

	const WCHAR* pszTurnText = nullptr;
	switch( winner )
	{
	case 0: // Continue to play.
		pszTurnText = playerTurn == 1 ? szTurn1 : szTurn2;
		break;
	case 1: // Player 1 wins.
		pszTurnText = L"Player 1 is the winner!";
		break;
	case 2: // Player 2 wins.
		pszTurnText = L"Player 2 is the winner!";
		break;
	case 3: // It's a draw
		pszTurnText = L"It's a draw!";
	}
	RECT rc;

	if( pszTurnText != nullptr && GetClientRect( hWnd,&rc ) )
	{
		rc.top = rc.bottom - 48;
		FillRect( hdc,&rc,HBRUSH( GetStockObject( GRAY_BRUSH ) ) );
		SetTextColor( hdc,RGB( 255,255,255 ) );
		SetBkMode( hdc,TRANSPARENT );
		DrawText( hdc,pszTurnText,lstrlen( pszTurnText ),
			&rc,DT_CENTER );
	}
}

LRESULT CALLBACK WndProc( HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam )
{
	switch( message )
	{
	case WM_CREATE:
	{
		hbr1 = CreateSolidBrush( RGB( 255,0,0 ) );
		hbr2 = CreateSolidBrush( RGB( 0,0,255 ) );
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD( wParam );
		// Parse the menu selections:
		switch( wmId )
		{
		case ID_FILE_NEWGAME:
		{
			const int action = MessageBox( hWnd,
				L"Are you sure you want to start a new game?",
				L"New Game",MB_YESNO | MB_ICONQUESTION );
			if( action == IDYES )
			{
				// Reset and start a new game.
				playerTurn = 1;
				winner = 0;
				ZeroMemory( gameBoard,sizeof( gameBoard ) );

				// Force a paint message.
				InvalidateRect( hWnd,nullptr,true ); // Post WM_PAINT to wndProc.
				UpdateWindow( hWnd ); // Force immediate handling of WM_PAINT.
			}
		}
		break;
		case IDM_ABOUT:
			DialogBox( hInst,MAKEINTRESOURCE( IDD_ABOUTBOX ),hWnd,About );
			break;
		case IDM_EXIT:
			DestroyWindow( hWnd );
			break;
		default:
			return DefWindowProc( hWnd,message,wParam,lParam );
		}
	}
	break;
	case WM_LBUTTONDOWN:
	{
		const int xPos = GET_X_LPARAM( lParam );
		const int yPos = GET_Y_LPARAM( lParam );

		// Only handle clicks if it is a player turn.
		if( playerTurn == 0 ) break;

		const int index = GetCellNumberFromPoint( hWnd,xPos,yPos );
		HDC hdc = GetDC( hWnd );

		if( hdc != nullptr )
		{
			// WCHAR temp[100];
			// wsprintf( temp,L"Index = %d",index );
			// TextOut( hdc,xPos,yPos,temp,lstrlen( temp ) );

			// std::string msg = "Index = " + std::to_string( index );
			// TextOutA( hdc,xPos,yPos,msg.c_str(),msg.length() );

			// Get cell dimension from index.
			if( index != -1 )
			{
				RECT rcCell;
				if( gameBoard[index] == 0 &&
					GetCellRect( hWnd,index,&rcCell ) )
				{
					gameBoard[index] = playerTurn;

					FillRect( hdc,&rcCell,
						playerTurn == 1 ? hbr1 : hbr2 );

					// Check for a winner.
					winner = GetWinner( wins );

					if( winner == 1 || winner == 2 )
					{
						// We have a winner!
						MessageBox( hWnd,winner == 1
							? L"Player 1 wins!"
							: L"Player 2 wins!",
							L"You win!",
							MB_OK | MB_ICONINFORMATION );

						playerTurn = 0;
					}
					else if( winner == 3 )
					{
						// It's a draw!
						MessageBox( hWnd,
							L"It's a cat's game!",
							L"It's a draw!",
							MB_OK | MB_ICONEXCLAMATION );

						playerTurn = 0;
					}
					else if( winner == 0 )
					{
						playerTurn = playerTurn == 1 ? 2 : 1;
					}

					// Display turn.
					ShowTurn( hWnd,hdc );
				}
			}

			ReleaseDC( hWnd,hdc );
		}
	}
	break;
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pMinMax = ( MINMAXINFO* )lParam;

		pMinMax->ptMinTrackSize.x = cellSize * 5;
		pMinMax->ptMinTrackSize.y = cellSize * 5;
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint( hWnd,&ps );
		// TODO: Add any drawing code that uses hdc here...
		RECT rc;
		if( GetGameBoardRect( hWnd,&rc ) )
		{
			RECT rcClient;
			if( GetClientRect( hWnd,&rcClient ) )
			{
				const WCHAR szPlayer1[] = L"Player 1";
				const WCHAR szPlayer2[] = L"Player 2";

				SetBkMode( hdc,TRANSPARENT );

				// Draw Player 1 and Player 2 text.
				SetTextColor( hdc,RGB( 255,255,0 ) );
				TextOut( hdc,16,16,szPlayer1,ARRAYSIZE( szPlayer1 ) );
				SetTextColor( hdc,RGB( 0,0,255 ) );
				TextOut( hdc,rcClient.right - 72,16,szPlayer2,ARRAYSIZE( szPlayer2 ) );

				// Display turn.
				ShowTurn( hWnd,hdc );
			}

			FillRect( hdc,&rc,HBRUSH( GetStockObject( WHITE_BRUSH ) ) );
			// Rectangle( hdc,rc.left,rc.top,rc.right,rc.bottom );
		}

		for( int i = 0; i < 4; ++i )
		{
			// Draw vertical lines.
			DrawLine( hdc,rc.left + i * cellSize,rc.top,
				rc.left + i * cellSize,rc.bottom );
			// Draw horizontal lines.
			DrawLine( hdc,rc.left,rc.top + i * cellSize,
				rc.right,rc.top + i * cellSize );
		}

		// Draw all occupied cells.
		RECT rcCell;
		for( int i = 0; i < ARRAYSIZE( gameBoard ); ++i )
		{
			if( gameBoard[i] != 0 &&
				GetCellRect( hWnd,i,&rcCell ) )
			{
				FillRect( hdc,&rcCell,gameBoard[i] == 1 ? hbr1 : hbr2 );
			}
		}
		// 
		EndPaint( hWnd,&ps );
	}
	break;
	case WM_DESTROY:
		DeleteObject( hbr1 );
		DeleteObject( hbr2 );

		PostQuitMessage( 0 );
		break;
	default:
		return DefWindowProc( hWnd,message,wParam,lParam );
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About( HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam )
{
	UNREFERENCED_PARAMETER( lParam );
	switch( message )
	{
	case WM_INITDIALOG:
		return ( INT_PTR )TRUE;

	case WM_COMMAND:
		if( LOWORD( wParam ) == IDOK || LOWORD( wParam ) == IDCANCEL )
		{
			EndDialog( hDlg,LOWORD( wParam ) );
			return ( INT_PTR )TRUE;
		}
		break;
	}
	return ( INT_PTR )FALSE;
}
