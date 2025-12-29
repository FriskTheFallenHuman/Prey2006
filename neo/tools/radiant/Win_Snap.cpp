//==============================
//	win_snap.cpp
//==============================

//=================================================
//
//	SIKK - WINDOW MANAGEMENT
//
//	*ripped from QE5
//=================================================
#include "precompiled.h"
#pragma hdrstop

#include "qe3.h"

#define TOLERANCE	  5
#define MOVETOLERANCE 2

/*
==================
FindClosestBottom
==================
*/
bool FindClosestBottom( HWND h, LPRECT rect, LPRECT parentrect )
{
	int	 best;
	RECT temprect;

	best = parentrect->bottom;
	if( rect->bottom > best )
	{
		rect->bottom = best;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) )
		{
			best = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) )
		{
			best = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) )
		{
			best = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) )
		{
			best = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) )
		{
			best = temprect.top;
		}
	}
	// if ( h != g_pParentWnd->g_Inspectors ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->bottom - temprect.top ) < abs( rect->bottom - best ) ) {
	//		best = temprect.top;
	//	}
	// }
	if( abs( rect->bottom - best ) <= TOLERANCE )
	{
		rect->bottom = best;
		return true;
	}
	return false;
}

/*
==================
FindClosestTop
==================
*/
bool FindClosestTop( HWND h, LPRECT rect, LPRECT parentrect )
{
	int	 best;
	RECT temprect;

	best = parentrect->top;
	if( rect->top < best )
	{
		rect->top = best;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) )
		{
			best = temprect.bottom;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) )
		{
			best = temprect.bottom;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) )
		{
			best = temprect.bottom;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) )
		{
			best = temprect.bottom;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) )
		{
			best = temprect.bottom;
		}
	}
	// if ( h != g_qeglobals.d_hwndEntity ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->top - temprect.bottom ) < abs( rect->top - best ) ) {
	//		best = temprect.bottom;
	//	}
	// }
	if( abs( rect->top - best ) <= TOLERANCE )
	{
		rect->top = best;
		return true;
	}
	return false;
}

/*
==================
FindClosestLeft
==================
*/
bool FindClosestLeft( HWND h, LPRECT rect, LPRECT parentrect, int widthlimit )
{
	int	 best;
	RECT temprect;

	best = parentrect->left;
	if( rect->left < best && rect->right - best >= widthlimit )
	{
		rect->left = best;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit )
		{
			best = temprect.right;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit )
		{
			best = temprect.right;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit )
		{
			best = temprect.right;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit )
		{
			best = temprect.right;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit )
		{
			best = temprect.right;
		}
	}
	// if ( h != g_qeglobals.d_hwndEntity ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->left - temprect.right ) < abs( rect->left - best ) && rect->right - temprect.right >= widthlimit ) {
	//		best = temprect.right;
	//	}
	// }
	if( abs( rect->left - best ) <= TOLERANCE && rect->right - best >= widthlimit )
	{
		rect->left = best;
		return true;
	}
	return false;
}

/*
==================
FindClosestRight
==================
*/
bool FindClosestRight( HWND h, LPRECT rect, LPRECT parentrect, int widthlimit )
{
	int	 best;
	RECT temprect;

	best = parentrect->right;
	if( rect->right > best && best - rect->left >= widthlimit )
	{
		rect->right = best;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit )
		{
			best = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit )
		{
			best = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit )
		{
			best = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit )
		{
			best = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit )
		{
			best = temprect.left;
		}
	}
	// if ( h != g_qeglobals.d_hwndEntity ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->right - temprect.left ) < abs( rect->right - best ) && temprect.left - rect->left >= widthlimit ) {
	//		best = temprect.left;
	//	}
	// }
	if( abs( best - rect->right ) <= TOLERANCE && best - rect->left >= widthlimit )
	{
		rect->right = best;
		return true;
	}
	return false;
}

/*
==================
FindClosestHorizontal
==================
*/
bool FindClosestHorizontal( HWND h, LPRECT rect, LPRECT parentrect )
{
	long bestleft, bestright, width = rect->right - rect->left;
	RECT temprect;

	bestleft = parentrect->left;
	if( rect->left < bestleft )
	{
		rect->left	= bestleft;
		rect->right = rect->left + width;
		return true;
	}
	bestright = parentrect->right;
	if( rect->right > bestright )
	{
		rect->right = bestright;
		rect->left	= rect->right - width;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) )
		{
			bestleft = temprect.right;
		}
		if( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) )
		{
			bestright = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) )
		{
			bestleft = temprect.right;
		}
		if( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) )
		{
			bestright = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) )
		{
			bestleft = temprect.right;
		}
		if( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) )
		{
			bestright = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) )
		{
			bestleft = temprect.right;
		}
		if( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) )
		{
			bestright = temprect.left;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) )
		{
			bestleft = temprect.right;
		}
		if( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) )
		{
			bestright = temprect.left;
		}
	}
	// if ( h != g_qeglobals.d_hwndEntity ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->left - temprect.right ) < abs( rect->left - bestleft ) ) {
	//		bestleft = temprect.right;
	//	}
	//	if ( abs( rect->right - temprect.left ) < abs( rect->right - bestright ) ) {
	//		bestright = temprect.left;
	//	}
	// }
	if( abs( rect->left - bestleft ) <= MOVETOLERANCE && abs( rect->left - bestleft ) )
	{
		rect->left	= bestleft;
		rect->right = rect->left + width;
		return true;
	}
	if( abs( rect->right - bestright ) <= MOVETOLERANCE )
	{
		rect->right = bestright;
		rect->left	= rect->right - width;
		return true;
	}

	return false;
}

/*
==================
FindClosestVertical
==================
*/
bool FindClosestVertical( HWND h, LPRECT rect, LPRECT parentrect )
{
	long besttop, bestbottom, height = rect->bottom - rect->top;
	RECT temprect;

	besttop = parentrect->top;
	if( rect->top < besttop )
	{
		rect->top	 = besttop;
		rect->bottom = rect->top + height;
		return true;
	}
	bestbottom = parentrect->bottom;
	if( rect->bottom > bestbottom )
	{
		rect->bottom = bestbottom;
		rect->top	 = rect->bottom - height;
		return true;
	}
	if( h != g_pParentWnd->GetCamera()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetCamera()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) )
		{
			besttop = temprect.bottom;
		}
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) )
		{
			bestbottom = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetXYWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXYWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) )
		{
			besttop = temprect.bottom;
		}
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) )
		{
			bestbottom = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetXZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetXZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) )
		{
			besttop = temprect.bottom;
		}
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) )
		{
			bestbottom = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetYZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetYZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) )
		{
			besttop = temprect.bottom;
		}
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) )
		{
			bestbottom = temprect.top;
		}
	}
	if( h != g_pParentWnd->GetZWnd()->GetSafeHwnd() )
	{
		GetWindowRect( g_pParentWnd->GetZWnd()->GetSafeHwnd(), &temprect );
		if( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) )
		{
			besttop = temprect.bottom;
		}
		if( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) )
		{
			bestbottom = temprect.top;
		}
	}
	// if ( h != g_qeglobals.d_hwndEntity ) {
	//	GetWindowRect( g_qeglobals.d_hwndEntity, &temprect );
	//	if ( abs( rect->top - temprect.bottom ) < abs( rect->top - besttop ) ) {
	//		besttop = temprect.bottom;
	//	}
	//	}
	//	if ( abs( rect->bottom - temprect.top ) < abs( rect->bottom - bestbottom ) ) {
	//		bestbottom = temprect.top;
	//	}
	// }
	if( abs( rect->top - besttop ) <= MOVETOLERANCE && abs( rect->top - besttop ) )
	{
		rect->top	 = besttop;
		rect->bottom = rect->top + height;
		return true;
	}
	if( abs( rect->bottom - bestbottom ) <= MOVETOLERANCE )
	{
		rect->bottom = bestbottom;
		rect->top	 = rect->bottom - height;
		return true;
	}

	return false;
}

/*
==================
TryDocking
==================
*/
bool TryDocking( HWND h, long side, LPRECT rect, int widthlimit )
{
	RECT parentrect, statusrect, toolbarrect;

	GetWindowRect( g_pParentWnd->GetToolbarWnd()->GetSafeHwnd(), &toolbarrect );
	GetWindowRect( g_pParentWnd->GetStatusbarWnd()->GetSafeHwnd(), &statusrect );
	GetClientRect( g_pParentWnd->GetSafeHwnd(), &parentrect );
	parentrect.top	  = toolbarrect.bottom;
	parentrect.bottom = statusrect.top;

	//	Sys_Printf( "%d %d %d %d\n", parentrect.top, parentrect.bottom, parentrect.left, parentrect.right );
	//	Sys_Printf( "%d %d %d %d\n\n", rect->top, rect->bottom, rect->left, rect->right );

	switch( side )
	{
		case WMSZ_BOTTOM:
			if( FindClosestBottom( h, rect, &parentrect ) )
			{
				return true;
			}
			break;
		case WMSZ_BOTTOMLEFT:
			if( FindClosestBottom( h, rect, &parentrect ) + FindClosestLeft( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_BOTTOMRIGHT:
			if( FindClosestBottom( h, rect, &parentrect ) + FindClosestRight( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_LEFT:
			if( FindClosestLeft( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_RIGHT:
			if( FindClosestRight( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_TOP:
			if( FindClosestTop( h, rect, &parentrect ) )
			{
				return true;
			}
			break;
		case WMSZ_TOPLEFT:
			if( FindClosestTop( h, rect, &parentrect ) + FindClosestLeft( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_TOPRIGHT:
			if( FindClosestTop( h, rect, &parentrect ) + FindClosestRight( h, rect, &parentrect, widthlimit ) )
			{
				return true;
			}
			break;
		case WMSZ_LEFT | WMSZ_BOTTOMRIGHT:
			if( FindClosestHorizontal( h, rect, &parentrect ) + FindClosestVertical( h, rect, &parentrect ) )
			{
				return true;
			}
			break;
	}
	return false;
}