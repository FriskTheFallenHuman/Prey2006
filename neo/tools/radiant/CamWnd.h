/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma once

typedef enum {
	cd_wire,
	cd_solid,
	cd_texture
} camera_draw_mode;

typedef struct {
	int			width, height;

	idVec3		origin;
	idAngles	angles;

	camera_draw_mode	draw_mode;

	idVec3		color;			// background

	idVec3		forward, right, up;	// move matrix
	idVec3		vup, vpn, vright;	// view matrix
} camera_t;

class CXYWnd;

// CCamWnd window

class CCamWnd : public CDialogEx {
  DECLARE_DYNCREATE( CCamWnd );
public:
			CCamWnd();
	virtual ~CCamWnd();

protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT &cs );

public:
	void OnCancel() {}
	void ShiftTexture_BrushPrimit( face_t *f, int x, int y );
	void SetXYFriend( CXYWnd *pWnd );
	camera_t& Camera() { return m_Camera; };
	void Cam_MouseControl( float dtime );
	void Cam_ChangeFloor( bool up );
	void BuildRendererState();
	void ToggleRenderMode();
	void ToggleRebuildMode();
	void ToggleEntityMode();
	void ToggleSelectMode();
	void ToggleAnimationMode();
	void ToggleSoundMode();
	void SetProjectionMatrix();
	void UpdateCameraView();

	void BuildEntityRenderState( idEditorEntity *ent, bool update );
	bool GetRenderMode() { return renderMode; }
	bool GetRebuildMode() { return rebuildMode; }
	bool GetEntityMode() { return entityMode; }
	bool GetAnimationMode() { return animationMode; }
	bool GetSelectMode() { return selectMode; }
	bool GetSoundMode() { return soundMode;	}

	bool UpdateRenderEntities();
	void MarkWorldDirty();

	void SetView( const idVec3 &origin, const idAngles &angles ) {
		m_Camera.origin = origin;
		m_Camera.angles = angles;
	}

	void Cam_BuildMatrix();

	CXYWnd *m_pXYFriend;

protected:
	void Cam_Init();
	void Cam_PositionDrag();
	void Cam_MouseLook();
	void Cam_MouseDown( int x, int y, int buttons );
	void Cam_MouseUp( int x, int y, int buttons );
	void Cam_MouseMoved( int x, int y, int buttons );
	void InitCull();
	bool CullBrush( idEditorBrush *b, bool cubicOnly );
	void Cam_Draw();
	void DrawGrid();
	void Cam_Render();

	// game renderer interaction
	void	FreeRendererState();
	void	UpdateCaption();
	bool	BuildBrushRenderData( idEditorBrush *brush );
	void	DrawEntityData();

	qhandle_t	worldModelDef;
	idRenderModel	*worldModel;		// createRawModel of the brush and patch geometry
	bool	worldDirty;
	bool	renderMode;
	bool	rebuildMode;
	bool	entityMode;
	bool	selectMode;
	bool	animationMode;
	bool	soundMode;

	bool m_bMouseLook = false; // To track if we are in mouse look mode
	CPoint m_LastMousePos; // To store the last mouse position
	float m_MouseSensitivity = 0.5; // Sensitivity for mouse look

	camera_t m_Camera;
	int	m_nCambuttonstate;
	CPoint m_ptButton;
	CPoint m_ptCursor;
	CPoint m_ptLastCursor;
	face_t* m_pSide_select;
	idVec3 m_vCull1;
	idVec3 m_vCull2;
	int m_nCullv1[3];
	int m_nCullv2[3];
	bool m_bClipMode;
	idVec3 saveOrg;
	idAngles saveAng;
	bool saveValid;
	idVec3 m_vPressdelta;

protected:
	void EnableMouseLook(bool enable);
	void UpdateCameraOrientation(float dx, float dy);
	void UpdateCameraPosition(float dx, float dy, float dz);
	void OriginalMouseDown(UINT nFlags, CPoint point);
	void OriginalMouseUp(UINT nFlags, CPoint point);

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};