// font.c

#include "SDLU.h"

#include "main.h"
#include "font.h"
#include "gworld.h"


#define kNumFonts (picBatsuFont-picFont+1)

static SkittlesFont s_font[kNumFonts];

static MBoolean IsWhitePixel(SDL_Surface* surface, int x, int y)
{
	SDL_Color pixel;
	SDLU_GetPixel(surface, x, y, &pixel);
	return pixel.r == 0xFF && pixel.g == 0xFF && pixel.b == 0xFF;
}

static MBoolean MeasureFontGlyphs(SDL_Surface* surface, SkittlesFontPtr font, unsigned char* letterMap)
{
	int width = surface->w;
	int y = surface->h - 1;
	int start;
	int across = 0;
	int skip;

	while (across < width && IsWhitePixel(surface, across, y))
	{
		across++;
	}
	skip = across;

	while (*letterMap)
	{
		while (across < width && !IsWhitePixel(surface, across, y))
		{
			across++;
		}
		if (across >= width)
		{
			return false;
		}

		start = across;
		font->across[*letterMap] = across + (skip / 2);

		while (across < width && IsWhitePixel(surface, across, y))
		{
			across++;
		}
		font->width[*letterMap] = across - start - skip;

		letterMap++;
	}

	return true;
}


static SkittlesFontPtr LoadFont( SkittlesFontPtr font, int pictID, unsigned char *letterMap )
{
	MBoolean       success = false;
	SDL_Surface*   temporarySurface;
	SDL_Rect       sdlRect;
	
	temporarySurface = LoadPICTAsSurface( pictID, 32 );
	
	if( temporarySurface )
	{
		success = MeasureFontGlyphs(temporarySurface, font, letterMap);

		sdlRect.x = 0;
		sdlRect.y = 0;
		sdlRect.h = temporarySurface->h;
		sdlRect.w = temporarySurface->w;

		if (success)
		{
			font->surface = SDLU_InitSurface( &sdlRect, 8 );
			SDL_SetSurfaceBlendMode(temporarySurface, SDL_BLENDMODE_NONE);
			SDLU_BlitSurface( temporarySurface, &sdlRect,
			                  font->surface,    &sdlRect );
		}

		SDL_DestroySurface( temporarySurface );
	}
	
	if( success )
	{
		return font;
	}
	else
	{
		char error[128];
		SDL_snprintf(error, sizeof(error), "LoadFont failed for PICT_%d", pictID);
		Error(error);
		return NULL;
	}
}


void InitFont( void ) 
{
	LoadFont( &s_font[0],  picFont, (unsigned char*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!:,.()*?0123456789'|-\x01\x02\x03\x04\x05 " );
	LoadFont( &s_font[1],  picHiScoreFont, (unsigned char*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*().,/-=_+<>?|'\":; " );
	LoadFont( &s_font[2],  picContinueFont, (unsigned char*) "0123456789" );
	LoadFont( &s_font[3],  picBalloonFont, (unsigned char*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-=_+;:,./<>? \x01\x02'\"" );
	LoadFont( &s_font[4],  picZapFont, (unsigned char*) "0123456789*PS" );
	LoadFont( &s_font[5],  picZapOutlineFont, (unsigned char*) "0123456789*" );
	LoadFont( &s_font[6],  picVictoryFont, (unsigned char*) "AB" );
	LoadFont( &s_font[7],  picBubbleFont, (unsigned char*) "*" );
	LoadFont( &s_font[8],  picTinyFont, (unsigned char*) "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789,.!`-=[];'/~_+{}:\"\\ " );
	LoadFont( &s_font[9],  picDashedLineFont, (unsigned char*) "." );
	LoadFont( &s_font[10], picBatsuFont, (unsigned char*) "X" );
}


SkittlesFontPtr GetFont( int pictID )
{
	int fontID = pictID - picFont;
	
	if( (fontID < 0) || (fontID >= kNumFonts) )
		Error( "GetFont: fontID" );
			
	return &s_font[fontID];
} 


int GetTextWidth( SkittlesFontPtr font, const char *text )
{
	int width = 0;
	while( *text )
	{
		width += font->width[(uint8_t) *text++];
	}
	
	return width;
}

