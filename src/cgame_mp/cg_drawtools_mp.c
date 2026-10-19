// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
// All text goes through the renderer's font painter; there is no bitmap charset.

#include <math.h>
#include <stdlib.h>

#include "cg_local.h"

#ifndef Vector4Copy
#define Vector4Copy( a,b )      ( (b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3] )
#endif
#ifndef Vector4Average
#define Vector4Average( v,b,s,o )   ( (o)[0]=((v)[0]*(1-(s)))+((b)[0]*(s)), \
                                      (o)[1]=((v)[1]*(1-(s)))+((b)[1]*(s)), \
                                      (o)[2]=((v)[2]*(1-(s)))+((b)[2]*(s)), \
                                      (o)[3]=((v)[3]*(1-(s)))+((b)[3]*(s)) )
#endif

#ifndef BIGCHAR_WIDTH
#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16
#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16
#endif
#ifndef YAW
#define PITCH   0
#define YAW     1
#define ROLL    2
#endif
#ifndef STAT_HEALTH
#define STAT_HEALTH     0
#endif

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	// scale for screen sizes
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
	*w *= cgs.screenXScale;
	*h *= cgs.screenYScale;
}

/*
================
G_AdjustCoordinates

CG_AdjustFrom640 for a single point
================
*/
void G_AdjustCoordinates( float *x, float *y ) {
	*x *= cgs.screenXScale;
	*y *= cgs.screenYScale;
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 1, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
==============
CG_FillRectGradient
==============
*/
void CG_FillRectGradient( float x, float y, float width, float height, const float *color, const float *gradcolor, int gradientType ) {
	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPicGradient( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader,
					   gradcolor, gradientType );

	trap_R_SetColor( NULL );
}

/*
==============
CG_FilledBar
	Generic routine for pretty much all status indicators that show a fractional
	value to the player by virtue of how full a drawn box is.

flags:
	left		- 1
	center		- 2		// direction is 'right' by default and orientation is 'horizontal'
	vert		- 4
	nohudalpha	- 8		// don't adjust bar's alpha value by the cg_hudalpha value
	bg			- 16	// background contrast box (bg set with bgColor of 'NULL' means use default bg color (1,1,1,0.25)
	spacing		- 32	// some bars use different sorts of spacing when drawing both an inner and outer box

	lerp color	- 256	// use an average of the start and end colors to set the fill color
==============
*/

// TODO: these flags will be shared, but it was easier to work on stuff if I wasn't changing header files a lot
#define BAR_LEFT        0x0001
#define BAR_CENTER      0x0002
#define BAR_VERT        0x0004
#define BAR_NOHUDALPHA  0x0008
#define BAR_BG          0x0010
// different spacing modes for use w/ BAR_BG
#define BAR_BGSPACING_X0Y5  0x0020
#define BAR_BGSPACING_X0Y0  0x0040

#define BAR_LERP_COLOR  0x0100

#define BAR_BORDERSIZE 1    // RTCW used 2

void CG_FilledBar( float x, float y, float w, float h, const float *startColor, float *endColor, const float *bgColor, float frac, int flags ) {
	vec4_t scolor;                                          // works on a copy of startColor
	vec4_t backgroundcolor = {1, 1, 1, 0.25f}, colorAtPos;  // colorAtPos is the lerped color if necessary
	int indent = BAR_BORDERSIZE;

	Vector4Copy( startColor, scolor );

	if ( ( flags & BAR_BG ) && bgColor ) { // BAR_BG set, and color specified, use specified bg color
		Vector4Copy( bgColor, backgroundcolor );
	}

	// hud alpha
	if ( !( flags & BAR_NOHUDALPHA ) ) {
		scolor[3] *= cg_hudAlpha.value;
		if ( endColor ) {
			endColor[3] *= cg_hudAlpha.value;
		}
		backgroundcolor[3] *= cg_hudAlpha.value;
	}

	if ( flags & BAR_LERP_COLOR ) {
		Vector4Average( scolor, endColor, frac, colorAtPos );
	}

	// background
	if ( ( flags & BAR_BG ) ) {
		// draw background at full size and shrink the remaining box to fit inside with a border.  (alternate border may be specified by a BAR_BGSPACING_xx)
		CG_FillRect(   x,
					   y,
					   w,
					   h,
					   backgroundcolor );

		if ( flags & BAR_BGSPACING_X0Y0 ) {          // fill the whole box (no border)

		} else if ( flags & BAR_BGSPACING_X0Y5 ) {   // spacing created for weapon heat
			indent *= 3;
			y += indent;
			h -= ( 2 * indent );

		} else {                                // default spacing of 1 unit on each side
			x += indent;
			y += indent;
			w -= ( 2 * indent );
			h -= ( 2 * indent );
		}
	}

	// adjust for horiz/vertical and draw the fractional box
	if ( flags & BAR_VERT ) {
		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			y += ( h * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			y += ( h * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			CG_FillRect( x, y, w, h * frac, colorAtPos );
		} else {
			CG_FillRect( x, y, w, h * frac, scolor );
		}

	} else {

		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			x += ( w * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			x += ( w * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			CG_FillRect( x, y, w * frac, h, colorAtPos );
		} else {
			CG_FillRect( x, y, w * frac, h, scolor );
		}
	}

}

/*
=================
CG_HorizontalPercentBar
=================
*/
void CG_HorizontalPercentBar( float x, float y, float width, float height, float percent ) {
	vec4_t bgcolor = {0.5f, 0.5f, 0.5f, 0.3f},
		   color = {1.0f, 1.0f, 1.0f, 0.3f};
	CG_FilledBar( x, y, width, height, color, NULL, bgcolor, percent, BAR_BG | BAR_NOHUDALPHA );
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenXScale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

/*
================
CG_DrawTopBottom
================
*/
void CG_DrawTopBottom( float x, float y, float w, float h, float size ) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	size *= cgs.screenYScale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}

/*
================
CG_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	vec4_t hudAlphaColor;

	Vector4Copy( color, hudAlphaColor );
	hudAlphaColor[3] *= cg_hudAlpha.value;

	trap_R_SetColor( hudAlphaColor );

	CG_DrawTopBottom( x, y, width, height, size );
	CG_DrawSides( x, y, width, height, size );

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

static const float cg_quadTexCoords[8] = { 0, 0,  1, 0,  1, 1,  0, 1 };
static const float cg_quadCorners[8] = { -1, -1,  1, -1,  1, 1,  -1, 1 };

/*
================
CG_DrawRotatedPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRotatedPic( float x, float y, float width, float height, float angle, qhandle_t hShader ) {
	float verts[8];
	float angleRadians, sinAngle, cosAngle;
	float center[2], rWidth, rHeight;
	rfloat rot[2][2];	// keep: rfloat and the single cast below match original codegen
	int i;

	CG_AdjustFrom640( &x, &y, &width, &height );

	// keep the parentheses: the original multiplies twice
	angleRadians = ( angle * (float)M_PI ) * ( 1.0f / 180.0f );
	SinCos( angleRadians, &sinAngle, &cosAngle );

	rWidth = width * 0.5f;
	rHeight = height * 0.5f;
	center[0] = x + rWidth;
	center[1] = y + rHeight;

	rot[0][0] = rWidth * cosAngle;
	rot[0][1] = rWidth * sinAngle;
	rot[1][0] = (float)( sinAngle * rHeight );
	rot[1][1] = rHeight * cosAngle;

	for ( i = 0; i < 4; i++ ) {
		verts[i * 2 + 0] = cg_quadCorners[i * 2 + 0] * rot[0][0] + center[0]
						   - cg_quadCorners[i * 2 + 1] * rot[0][1];
		verts[i * 2 + 1] = cg_quadCorners[i * 2 + 1] * rot[1][1]
						   + cg_quadCorners[i * 2 + 0] * rot[1][0] + center[1];
	}

	trap_R_DrawQuadPic( verts, cg_quadTexCoords, hShader );
}

/*
================
CG_DrawRotatedQuadPic

Rotate a caller-supplied quad about the origin, translate it to x,y and draw it
=================
*/
void CG_DrawRotatedQuadPic( float x, float y, const float *verts, const float *texCoords, float angle, qhandle_t hShader ) {
	float outVerts[8];
	float angleRadians, sinAngle, cosAngle;
	rfloat pos[2], rot[2][2];	// keep: rfloat and the single cast below match original codegen
	int i;

	angleRadians = ( angle * (float)M_PI ) * ( 1.0f / 180.0f );
	SinCos( angleRadians, &sinAngle, &cosAngle );

	rot[0][0] = cgs.screenXScale * cosAngle;
	rot[0][1] = cgs.screenXScale * sinAngle;
	rot[1][0] = cgs.screenYScale * sinAngle;
	rot[1][1] = cgs.screenYScale * cosAngle;

	pos[0] = cgs.screenXScale * x;
	pos[1] = (float)( cgs.screenYScale * y );

	for ( i = 0; i < 4; i++ ) {
		outVerts[i * 2 + 0] = rot[0][0] * verts[i * 2] + pos[0] - rot[0][1] * verts[i * 2 + 1];
		outVerts[i * 2 + 1] = rot[1][0] * verts[i * 2] + rot[1][1] * verts[i * 2 + 1] + pos[1];
	}

	trap_R_DrawQuadPic( outVerts, texCoords, hShader );
}

/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow.  forceColor is unused: the
text painter handles the color escapes.

If virtualScreen is false, coordinates and sizes are real pixels and are
scaled back to the 640 by 480 virtual resolution.
==================
*/
void CG_DrawStringExt( float x, float y, const char *string, const float *setColor,
					   qboolean forceColor, qboolean shadow, float charWidth, float charHeight,
					   int maxChars, qboolean virtualScreen ) {
	vec4_t color = { 1, 1, 1, 1 };
	float scale;
	rfloat sd, rx, ry;

	// keep: rfloat casts and reciprocal multiplies match original codegen
	y = (rfloat)charHeight * 0.8f + y;
	sd = (rfloat)charHeight * ( 1.0f / 48.0f );

	if ( !virtualScreen ) {
		rx = 1.0f / (rfloat)cgs.screenXScale;
		x = x * rx;
		ry = 1.0f / (rfloat)cgs.screenYScale;
		y *= ry;
		charWidth = rx * charWidth;
		sd *= (float)ry;
	}

	scale = sd;

	if ( !setColor ) {
		setColor = color;
	}

	trap_R_Text_Paint( x, y, 5, scale, setColor, string, charWidth, maxChars,
					   shadow ? 3 : 0 );
}

/*
==================
CG_DrawBigString
==================
*/
void CG_DrawBigString( float x, float y, const char *s, float alpha ) {
	float color[4];

	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = alpha;
	trap_R_Text_Paint( x, y + BIGCHAR_HEIGHT - 2, 4, BIGCHAR_HEIGHT / 48.0f,
					   color, s, BIGCHAR_WIDTH, 0, 3 );
}

/*
==================
CG_DrawBigStringColor
==================
*/
void CG_DrawBigStringColor( float x, float y, const char *s, const float *color ) {
	trap_R_Text_Paint( x, y + BIGCHAR_HEIGHT - 2, 4, BIGCHAR_HEIGHT / 48.0f,
					   color, s, BIGCHAR_WIDTH, 0, 3 );
}

/*
==================
CG_DrawSmallString
==================
*/
void CG_DrawSmallString( float x, float y, const char *s, float alpha ) {
	float color[4];

	color[0] = 1.0;
	color[1] = 1.0;
	color[2] = 1.0;
	color[3] = alpha;
	trap_R_Text_Paint( x, y + SMALLCHAR_HEIGHT - 2, 5, SMALLCHAR_HEIGHT / 48.0f,
					   color, s, SMALLCHAR_WIDTH, 0, 0 );
}

/*
==================
CG_DrawSmallStringColor
==================
*/
void CG_DrawSmallStringColor( float x, float y, const char *s, const float *color ) {
	trap_R_Text_Paint( x, y + SMALLCHAR_HEIGHT - 2, 5, SMALLCHAR_HEIGHT / 48.0f,
					   color, s, SMALLCHAR_WIDTH, 0, 0 );
}

/*
=================
Q_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int Q_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( s[0] == '^' && s[1] && s[1] != '^' && s[1] >= '0' && s[1] <= '7' ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float s1, t1, s2, t2;
	s1 = x / 64.0f;
	t1 = y / 64.0f;
	s2 = ( x + w ) / 64.0f;
	t2 = ( y + h ) / 64.0f;
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}

/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int top, bottom, left, right;
	int w, h;

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( cg.refdef.x == 0 && cg.refdef.y == 0 &&
		 cg.refdef.width == w && cg.refdef.height == h ) {
		return;     // full screen rendering
	}

	top = cg.refdef.y;
	bottom = top + cg.refdef.height - 1;
	left = cg.refdef.x;
	right = left + cg.refdef.width - 1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );
}

/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t color;
	int t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * ( 1.0f / FADE_TIME );
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	color[3] *= cg_hudAlpha.value;          // NERVE - SMF - make this work like everything else

	return color;
}

/*
=================
CG_GetColorForHealth

armor is unused
=================
*/
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor ) {
	if ( health <= 0 ) {
		VectorClear( hcolor );  // black
		hcolor[3] = 1;
		return;
	}

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) * ( 1.0f / 33.0f );
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) * ( 1.0f / 30.0f );
	}
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForHealth( vec4_t hcolor ) {
	int health;

	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health <= 0 ) {
		VectorClear( hcolor );  // black
		hcolor[3] = 1;
		return;
	}

	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) * ( 1.0f / 33.0f );
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) * ( 1.0f / 30.0f );
	}
}

/*
================
CG_NorthDirectionChanged

The map's north yaw config string changed
================
*/
#define CS_NORTHYAW                 11

void CG_NorthDirectionChanged( void ) {
	const char  *str;

	str = CG_ConfigString( CS_NORTHYAW );
	cg.compassNorthYaw = atof( str );
}

/*
=================
CG_UpdateCompassOrientation

Spring the compass rose towards the player's yaw, in 5 msec steps
(keep: the statement order and the nested ifs match original codegen)
=================
*/
void CG_UpdateCompassOrientation( void ) {
	static int lastCompassTime;
	int msec, step;
	float playerAngle, delta, frametime, angle;

	playerAngle = AngleNormalize360( cg.refdefViewAngles[YAW] - cg.compassNorthYaw );

	if ( lastCompassTime <= cg.time ) {
		msec = cg.time - lastCompassTime;
		if ( !( (float)msec > 500.0f ) ) {
			lastCompassTime = cg.time;

			delta = AngleSubtract( cg.compassPlayerAngle, playerAngle );

			while ( msec > 0 ) {
				if ( msec > 5 ) {
					step = 5;
					msec -= 5;
				} else {
					step = msec;
					msec = 0;
				}
				frametime = step * 0.001f;

				if ( fabs( delta ) < 0.25 && fabs( cg.compassPlayerAngleVel ) < 1.0 ) {
					cg.compassPlayerAngleVel = 0;
					cg.compassPlayerAngle = playerAngle;
					return;
				}

				angle = AngleNormalize180( cg.compassPlayerAngleVel * frametime + delta );
				delta = angle;

				if ( delta > 0 ) {
					cg.compassPlayerAngleVel -= frametime * 1000.0f;
				} else if ( delta < 0 ) {
					cg.compassPlayerAngleVel += frametime * 1000.0f;
				}

				cg.compassPlayerAngleVel -= cg.compassPlayerAngleVel * frametime * 2;

				if ( cg.compassPlayerAngleVel > 0 ) {
					if ( delta > 0 ) {
						cg.compassPlayerAngleVel -= cg.compassPlayerAngleVel * frametime * 3.5f;
					}
					cg.compassPlayerAngleVel -= frametime;
					if ( cg.compassPlayerAngleVel < 0 ) {
						cg.compassPlayerAngleVel = 0;
						continue;
					}
				} else {
					if ( delta < 0 ) {
						cg.compassPlayerAngleVel -= cg.compassPlayerAngleVel * frametime * 3.5f;
					}
					cg.compassPlayerAngleVel += frametime;
					if ( cg.compassPlayerAngleVel > 0 ) {
						cg.compassPlayerAngleVel = 0;
						continue;
					}
				}

				if ( cg.compassPlayerAngleVel > 30000.0f ) {
					cg.compassPlayerAngleVel = 30000.0f;
				} else if ( cg.compassPlayerAngleVel < -30000.0f ) {
					cg.compassPlayerAngleVel = -30000.0f;
				}
			}

			cg.compassPlayerAngle = AngleNormalize360( delta + playerAngle );
			return;
		}
	}

	lastCompassTime = cg.time;
	cg.compassPlayerAngle = playerAngle;
	cg.compassPlayerAngleVel = 0;
}

/*
=================
CG_UpdateCompPointerOrientation

The same spring for the compass pointers, with its own constants and a
+-10 degree cap on the error it will chase
=================
*/
void CG_UpdateCompPointerOrientation( void ) {
	static int lastPointerTime;
	int msec, step;
	float angle, delta, frametime;
	rfloat vel;	// keep: rfloat matches original codegen

	if ( !cg_hudCompassSpringyPointers.integer ) {
		cg.compassPointerAngle = cg.refdefViewAngles[YAW];
		return;
	}

	if ( lastPointerTime == cg.time ) {
		return;
	}

	angle = cg.refdefViewAngles[YAW];

	if ( lastPointerTime <= cg.time ) {
		msec = cg.time - lastPointerTime;
		if ( !( (float)msec > 500.0f ) ) {
			lastPointerTime = cg.time;

			delta = AngleSubtract( cg.compassPointerAngle, angle );
			if ( fabs( delta ) > 10.0 ) {
				float sign = delta;	// keep: the sign bit test matches original codegen

				if ( *(int *)&sign < 0 ) {
					delta = -10.0f;
				} else {
					delta = 10.0f;
				}
			}

			while ( msec > 0 ) {
				if ( msec > 5 ) {
					step = 5;
					msec -= 5;
				} else {
					step = msec;
					msec = 0;
				}
				frametime = step * 0.001f;

				if ( fabs( delta ) < 0.5 && fabs( cg.compassPointerAngleVel ) < 2.0 ) {
					cg.compassPointerAngle = angle;
					cg.compassPointerAngleVel = 0;
					return;
				}

				delta = AngleNormalize180( cg.compassPointerAngleVel * frametime + delta );

				if ( delta > 0 ) {
					vel = cg.compassPointerAngleVel - frametime * 1500.0f;
				} else if ( delta < 0 ) {
					vel = frametime * 1500.0f + cg.compassPointerAngleVel;
				} else {
					vel = cg.compassPointerAngleVel;
				}

				vel -= frametime * vel * 3;

				if ( vel > 0 ) {
					if ( delta > 0 ) {
						vel -= frametime * vel * 5;
					}
					cg.compassPointerAngleVel = vel - frametime * 2;
					if ( cg.compassPointerAngleVel < 0 ) {
						cg.compassPointerAngleVel = 0;
						continue;
					}
				} else {
					if ( delta < 0 ) {
						vel -= frametime * vel * 5;
					}
					cg.compassPointerAngleVel = vel + frametime * 2;
					if ( cg.compassPointerAngleVel > 0 ) {
						cg.compassPointerAngleVel = 0;
						continue;
					}
				}

				if ( cg.compassPointerAngleVel > 2000.0f ) {
					cg.compassPointerAngleVel = 2000.0f;
				} else if ( cg.compassPointerAngleVel < -2000.0f ) {
					cg.compassPointerAngleVel = -2000.0f;
				}
			}

			cg.compassPointerAngle = AngleNormalize360( delta + angle );
			return;
		}
	}

	lastPointerTime = cg.time;
	cg.compassPointerAngle = angle;
	cg.compassPointerAngleVel = 0;
}

// the twelve edges of a box, as indices into its eight corners
static const int cg_debugBoxEdges[12][2] = {
	{0, 1}, {0, 2}, {0, 4}, {1, 3}, {1, 5}, {2, 3},
	{2, 6}, {3, 7}, {4, 5}, {4, 6}, {5, 7}, {6, 7}
};

/*
=================
CG_DebugBox

duration is ignored: the lines always pass 0
=================
*/
void CG_DebugBox( const vec3_t mins, const vec3_t maxs, const float *color, int depthTest, int duration ) {
	vec3_t points[8];
	int i, j;

	for ( i = 0; i < 8; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			points[i][j] = ( i & ( 1 << j ) ) ? maxs[j] : mins[j];
		}
	}

	for ( i = 0; i < sizeof( cg_debugBoxEdges ) / sizeof( cg_debugBoxEdges[0] ); i++ ) {
		trap_AddDebugLine( points[cg_debugBoxEdges[i][0]], points[cg_debugBoxEdges[i][1]],
						   color, depthTest, 0 );
	}
}

/*
=================
CG_DebugCircleEx

A 16-segment circle of radius about org, in the plane whose normal is dir
=================
*/
void CG_DebugCircleEx( const vec3_t dir, const vec3_t org, float radius, const float *color, int depthTest, int duration ) {
	vec3_t axis, vRight, vUp;
	vec3_t points[16];
	float angle, s, c;
	int i;

	VectorNormalize2( dir, axis );
	PerpendicularVector( vRight, axis );
	CrossProduct( axis, vRight, vUp );

	for ( i = 0; i < sizeof( points ) / sizeof( points[0] ); i++ ) {
		angle = i * 0.39269909f;    // 2*M_PI/16
		SinCos( angle, &s, &c );
		s *= radius;
		c *= radius;
		VectorScale( vUp, s, points[i] );
		VectorMA( points[i], c, vRight, points[i] );
		VectorAdd( points[i], org, points[i] );
	}

	for ( i = 0; i < sizeof( points ) / sizeof( points[0] ); i++ ) {
		trap_AddDebugLine( points[i], points[( i + 1 ) & 15], color, depthTest, duration );
	}
}

/*
=================
CG_DebugArc

15 segments of an arc in the XY plane at org[2], from startAngle to endAngle
=================
*/
void CG_DebugArc( const vec3_t org, float radius, float startAngle, float endAngle, const float *color, int depthTest, int duration ) {
	vec3_t points[16];
	float step, angle, s, c;
	int i;

	step = ( endAngle - startAngle ) * ( 1.0f / 15 );
	if ( step < 0 ) {
		startAngle -= 360.0f;
		step = ( endAngle - startAngle ) * ( 1.0f / 15 );
	}

	for ( i = 0; i < sizeof( points ) / sizeof( points[0] ); i++ ) {
		// keep: the (float) cast matches original codegen
		angle = (float)( ( ( i * step + startAngle ) * (float)M_PI ) * ( 1.0f / 180.0f ) );
		SinCos( angle, &s, &c );
		points[i][0] = c * radius + org[0];
		points[i][1] = s * radius + org[1];
		points[i][2] = org[2];
	}

	for ( i = 0; i < 15; i++ ) {
		trap_AddDebugLine( points[i], points[i + 1], color, depthTest, duration );
	}
}

/*
=================
CG_TeamColor
=================
*/
float *CG_TeamColor( int team ) {
	static vec4_t red = { 1, 0.2f, 0.2f, 1 };
	static vec4_t blue = { 0.2f, 0.2f, 1, 1 };
	static vec4_t other = { 1, 1, 1, 1 };
	static vec4_t spectator = { 0.7f, 0.7f, 0.7f, 1 };

	switch ( team ) {
	case 1:  return red;        // TEAM_AXIS
	case 2:  return blue;       // TEAM_ALLIES
	case 3:  return spectator;  // TEAM_SPECTATOR
	default: return other;
	}
}
