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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef _QERTYPE_H
#define _QERTYPE_H

class texdef_t
{
public:
	texdef_t()
	{
		name	 = "";
		shift[0] = shift[1] = 0.0;
		rotate				= 0;
		scale[0] = scale[1] = 0;
		value				= 0;
	}
	~texdef_t()
	{
		if( name && name[0] ) { delete[] name; }
		name = NULL;
	}

	void SetName( const char* p )
	{
		if( name && name[0] ) { delete[] name; }
		if( p && p[0] ) { name = strcpy( new char[strlen( p ) + 1], p ); }
		else { name = ""; }
	}

	texdef_t& operator=( const texdef_t& rhs )
	{
		if( &rhs != this )
		{
			SetName( rhs.name );
			shift[0] = rhs.shift[0];
			shift[1] = rhs.shift[1];
			rotate	 = rhs.rotate;
			scale[0] = rhs.scale[0];
			scale[1] = rhs.scale[1];
			value	 = rhs.value;
		}
		return *this;
	}
	// char	name[128];
	char* name;
	float shift[2];
	float rotate;
	float scale[2];
	int	  value;
};

// Timo
// new brush primitive texdef
// typedef struct brushprimit_texdef_s
//{
//	float	coords[2][3];
//} brushprimit_texdef_t;

class brushprimit_texdef_t
{
public:
	float coords[2][3];
	brushprimit_texdef_t()
	{
		memset( &coords, 0, sizeof( coords ) );
		coords[0][0] = 1.0;
		coords[1][1] = 1.0;
	}
};

class texturewin_t
{
public:
	texturewin_t()
	{
		memset( &brushprimit_texdef.coords, 0, sizeof( brushprimit_texdef.coords ) );
		brushprimit_texdef.coords[0][0] = 1.0;
		brushprimit_texdef.coords[1][1] = 1.0;
	}

	~texturewin_t() { }
	int					 width, height;
	int					 originy;
	// add brushprimit_texdef_t for brush primitive coordinates storage
	brushprimit_texdef_t brushprimit_texdef;
	int					 m_nTotalHeight;
	// surface plugin, must be casted to a IPluginTexdef*
	void*				 pTexdef;
	texdef_t			 texdef;
};

//++timo texdef and brushprimit_texdef are static
// TODO : do dynamic ?
struct face_t
{
	face_t*				 next;
	face_t*				 original; // used for vertex movement
	idVec3				 planepts[3];
	idVec3				 orgplanepts[3]; // used for arbitrary rotation
	texdef_t			 texdef;

	idPlane				 plane;
	idPlane				 originalPlane;
	bool				 dirty;

	idWinding*			 face_winding;

	idVec3				 d_color;
	const idMaterial*	 d_texture;

	// Timo new brush primit texdef
	brushprimit_texdef_t brushprimit_texdef;
};

struct curveVertex_t
{
	idVec3 xyz;
	float  sideST[2];
	float  capST[2];
};

struct sideVertex_t
{
	curveVertex_t v[2];
};

struct idEditorBrush;

struct patchMesh_t
{
	int					  width, height; // in control points, not patches
	int					  horzSubdivisions;
	int					  vertSubdivisions;
	bool				  explicitSubdivisions;
	int					  contents, flags, value, type;
	const idMaterial*	  d_texture;
	idDrawVert*			  verts;
	// idDrawVert *ctrl;
	idEditorBrush*		  pSymbiot;
	bool				  bSelected;
	bool				  bOverlay;
	int					  nListID;
	int					  nListIDCam;
	int					  nListSelected;

	idDict*				  epairs;

	ID_INLINE idDrawVert& ctrl( int col, int row )
	{
		if( col < 0 || col >= width || row < 0 || row >= height )
		{
			common->Warning( "patchMesh_t::ctrl: control point out of range" );
			return verts[0];
		}
		else { return verts[row * width + col]; }
	}
};

enum
{
	LIGHT_TARGET,
	LIGHT_RIGHT,
	LIGHT_UP,
	LIGHT_RADIUS,
	LIGHT_X,
	LIGHT_Y,
	LIGHT_Z,
	LIGHT_START,
	LIGHT_END,
	LIGHT_CENTER
};

#define MAX_FLAGS 8

typedef struct trimodel_t
{
	idVec3 v[3];
	float  st[3][2];
} trimodel;

// eclass show flags

#define ECLASS_LIGHT		  0x00000001
#define ECLASS_ANGLE		  0x00000002
#define ECLASS_PATH			  0x00000004
#define ECLASS_MISCMODEL	  0x00000008
#define ECLASS_PLUGINENTITY	  0x00000010
#define ECLASS_PROJECTEDLIGHT 0x00000020
#define ECLASS_WORLDSPAWN	  0x00000040
#define ECLASS_SPEAKER		  0x00000080
#define ECLASS_PARTICLE		  0x00000100
#define ECLASS_ROTATABLE	  0x00000200
#define ECLASS_CAMERAVIEW	  0x00000400
#define ECLASS_MOVER		  0x00000800
#define ECLASS_ENV			  0x00001000
#define ECLASS_LIQUID		  0x00002000

enum EVAR_TYPES
{
	EVAR_STRING,
	EVAR_INT,
	EVAR_FLOAT,
	EVAR_BOOL,
	EVAR_COLOR,
	EVAR_MATERIAL,
	EVAR_MODEL,
	EVAR_GUI,
	EVAR_SOUND
};

struct evar_t
{
	int	  type;
	idStr name;
	idStr desc;
};

struct eclass_t
{
	eclass_t*	   next;
	idStr		   name;
	bool		   fixedsize;
	idVec3		   mins, maxs;
	idVec3		   color;
	texdef_t	   texdef;
	idStr		   comments;
	idStr		   desc;

	idRenderModel* modelHandle;
	idRenderModel* entityModel;

	unsigned int   nShowFlags;
	idStr		   defMaterial;
	idDict		   args;
	idDict		   defArgs;
	idList<evar_t> vars;
};

extern eclass_t* eclass;

/*
** window bits
*/
#define W_CAMERA	  0x0001
#define W_XY		  0x0002
#define W_XY_OVERLAY  0x0004
#define W_Z			  0x0008
#define W_TEXTURE	  0x0010
#define W_Z_OVERLAY	  0x0020
#define W_CONSOLE	  0x0040
#define W_ENTITY	  0x0080
#define W_CAMERA_ICON 0x0100
#define W_XZ		  0x0200 //--| only used for patch vertex manip stuff
#define W_YZ		  0x0400 //--|
#define W_MEDIA		  0x1000
#define W_GAME		  0x2000
#define W_ALL		  0xFFFFFFFF

// used in some Drawing routines
enum class ViewType
{
	YZ,
	XZ,
	XY
};

#endif
