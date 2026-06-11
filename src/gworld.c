// gworld.c

#include "SDLU.h"

#include "main.h"
#include "gworld.h"
#include "blitter.h"
#include "graphics.h"

#ifdef _WIN32
#include <objbase.h>
#include <wincodec.h>
#endif

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "support/stb_image.h"

SDL_Surface* blobSurface;
SDL_Surface* maskSurface;
SDL_Surface* charMaskSurface;
SDL_Surface* boardSurface[2];
SDL_Surface* blastSurface;
SDL_Surface* blastMaskSurface;
SDL_Surface* playerSurface[2];
SDL_Surface* playerSpriteSurface[2];

static SDL_Surface* CreateARGBSurface(int width, int height)
{
	SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
	if (!surface)
	{
		Error("CreateARGBSurface failed");
	}
	return surface;
}

static SDL_Surface* ConvertSurfaceToMask(SDL_Surface* source)
{
	SDL_Rect rect;
	SDL_Surface* maskSurface;
	Uint8* destPixels;
	Uint8* srcPixels;
	MBoolean usesExplicitAlpha = false;

	rect.x = 0;
	rect.y = 0;
	rect.w = source->w;
	rect.h = source->h;

	maskSurface = SDLU_InitSurface(&rect, MASK_DEPTH);

	SDL_LockSurface(source);
	srcPixels = (Uint8*) source->pixels;
	for (int y = 0; y < source->h && !usesExplicitAlpha; y++)
	{
		Uint8* row = srcPixels + y * source->pitch;
		for (int x = 0; x < source->w; x++)
		{
			if (row[x * 4 + 3] != 255)
			{
				usesExplicitAlpha = true;
				break;
			}
		}
	}

	SDL_LockSurface(maskSurface);
	srcPixels = (Uint8*) source->pixels;
	destPixels = (Uint8*) maskSurface->pixels;
	for (int y = 0; y < source->h; y++)
	{
		Uint8* srcRow = srcPixels + y * source->pitch;
		Uint8* dstRow = destPixels + y * maskSurface->pitch;
		for (int x = 0; x < source->w; x++)
		{
			Uint8 blue = srcRow[x * 4 + 0];
			Uint8 green = srcRow[x * 4 + 1];
			Uint8 red = srcRow[x * 4 + 2];
			Uint8 alpha = srcRow[x * 4 + 3];
			Uint8 luminance = MaxInt(red, MaxInt(green, blue));
			dstRow[x] = usesExplicitAlpha ? alpha : (Uint8) (255 - luminance);
		}
	}
	SDL_UnlockSurface(maskSurface);
	SDL_UnlockSurface(source);

	return maskSurface;
}

#ifdef _WIN32
static SDL_Surface* LoadDDSAsSurface(const char* filename)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	MBoolean mustUninitialize = SUCCEEDED(hr);
	SDL_Surface* surface = NULL;
	wchar_t wideFilename[1024];
	IWICImagingFactory* factory = NULL;
	IWICBitmapDecoder* decoder = NULL;
	IWICBitmapFrameDecode* frame = NULL;
	IWICFormatConverter* converter = NULL;
	UINT width = 0, height = 0;

	if (!MultiByteToWideChar(CP_UTF8, 0, filename, -1, wideFilename, arrsize(wideFilename)))
	{
		goto cleanup;
	}

	hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (LPVOID*) &factory);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	hr = factory->lpVtbl->CreateDecoderFromFilename(factory, wideFilename, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	hr = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	hr = factory->lpVtbl->CreateFormatConverter(factory, &converter);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	hr = converter->lpVtbl->Initialize(
		converter,
		(IWICBitmapSource*) frame,
		&GUID_WICPixelFormat32bppBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0f,
		WICBitmapPaletteTypeCustom);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	hr = frame->lpVtbl->GetSize(frame, &width, &height);
	if (FAILED(hr))
	{
		goto cleanup;
	}

	surface = CreateARGBSurface((int) width, (int) height);
	SDL_LockSurface(surface);

	hr = converter->lpVtbl->CopyPixels(converter, NULL, surface->pitch, surface->pitch * surface->h, surface->pixels);
	SDL_UnlockSurface(surface);

	if (FAILED(hr))
	{
		SDL_DestroySurface(surface);
		surface = NULL;
	}
	else
	{
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	}

cleanup:
	if (converter) converter->lpVtbl->Release(converter);
	if (frame) frame->lpVtbl->Release(frame);
	if (decoder) decoder->lpVtbl->Release(decoder);
	if (factory) factory->lpVtbl->Release(factory);
	if (mustUninitialize) CoUninitialize();

	return surface;
}
#endif

static SDL_Surface* LoadRasterSurface(const char* filename)
{
	SDL_Surface* surface;
	SDL_Rect rect = { 0 };
	uint8_t* pixels = stbi_load(filename, &rect.w, &rect.h, NULL, 4);
	if (!pixels)
	{
		return NULL;
	}

	surface = CreateARGBSurface(rect.w, rect.h);
	SDL_LockSurface(surface);

	uint8_t* srcPixels = pixels;
	uint8_t* destPixels = (uint8_t*) surface->pixels;
	for (int i = 0; i < rect.w * rect.h; i++)
	{
		destPixels[0] = srcPixels[2];
		destPixels[1] = srcPixels[1];
		destPixels[2] = srcPixels[0];
		destPixels[3] = srcPixels[3];
		destPixels += 4;
		srcPixels += 4;
	}

	SDL_UnlockSurface(surface);
	stbi_image_free(pixels);
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	return surface;
}

static SDL_Surface* LoadMissingSurface(void)
{
	const char* missingFilename = QuickResourceName("missing", 0, ".png");
	if (!FileExists(missingFilename))
	{
		return NULL;
	}

	return LoadRasterSurface(missingFilename);
}

SDL_Surface* LoadGraphicAsSurface(const char* filename, int depth)
{
	SDL_Surface* surface = NULL;
	const char* extension = filename ? SDL_strrchr(filename, '.') : NULL;

	if (filename && FileExists(filename))
	{
#ifdef _WIN32
		if (extension && SDL_strcasecmp(extension, ".dds") == 0)
		{
			surface = LoadDDSAsSurface(filename);
		}
		else
#endif
		{
			surface = LoadRasterSurface(filename);
		}
	}

	if (!surface)
	{
		const char* missingFilename = QuickResourceName("missing", 0, ".png");
		if (!filename || SDL_strcasecmp(filename, missingFilename) != 0)
		{
			surface = LoadMissingSurface();
		}
	}

	if (!surface)
	{
		return NULL;
	}

	if (depth == MASK_DEPTH)
	{
		SDL_Surface* maskSurface = ConvertSurfaceToMask(surface);
		SDL_DestroySurface(surface);
		surface = maskSurface;
	}
	else if (depth != 0 && depth != 32)
	{
		SDLU_ChangeSurfaceDepth(&surface, depth);
	}

	return surface;
}

void GetBlobGraphics()
{
	MRect myRect;
	
	// Get board
	
	myRect.top = myRect.left = 0;
	myRect.right = kBlobHorizSize * kGridAcross;
	myRect.bottom = kBlobVertSize * (kGridDown-1);
	
	boardSurface[0] = LoadPICTAsSurface( picBoard, 32 );
	
	boardSurface[1] = LoadPICTAsSurface( picBoardRight, 32 );
	if( boardSurface[1] == NULL )
		boardSurface[1] = LoadPICTAsSurface( picBoard, 32 );

	// Get blob worlds
	blobSurface = LoadPICTAsSurface( picBlob, 32 );
	maskSurface = LoadPICTAsSurface( picBlobMask, MASK_DEPTH );
	charMaskSurface = LoadPICTAsSurface( picCharMask, MASK_DEPTH );

	// Get blast worlds
	
	blastSurface = LoadPICTAsSurface( picBlast, 32 );
	blastMaskSurface = LoadPICTAsSurface( picBlastMask, 32 );
}


void InitPlayerWorlds()
{
	MRect     myRect;
	SDL_Rect  sdlRect;
	int       count;
	
	myRect.top = myRect.left = 0;
	myRect.right = kGridAcross * kBlobHorizSize;
	myRect.bottom = kGridDown * kBlobVertSize;
	
	SDLU_MRectToSDLRect( &myRect, &sdlRect );
	
	for( count=0; count<=1; count++ )
	{
		playerSurface[count]       = SDLU_InitSurface( &sdlRect, 32 );
		playerSpriteSurface[count] = SDLU_InitSurface( &sdlRect, 32 );
	}
}


void SurfaceDrawBoard( int player, const MRect *myRect )
{
	MRect    srcRect, offsetRect;
	SDL_Rect srcSDLRect, offsetSDLRect;
	
	srcRect = *myRect;
	if( srcRect.bottom <= kBlobVertSize ) return;
	if( srcRect.top < kBlobVertSize ) srcRect.top = kBlobVertSize;

	offsetRect = srcRect;
	OffsetMRect( &offsetRect, 0, -kBlobVertSize );

	SDLU_BlitSurface( boardSurface[player],     SDLU_MRectToSDLRect( &offsetRect, &offsetSDLRect ),
	                  SDLU_GetCurrentSurface(), SDLU_MRectToSDLRect( &srcRect, &srcSDLRect )         );
}


void SurfaceDrawBlob( int player, const MRect *myRect, int blob, int state, int charred )
{
	SurfaceDrawBoard( player, myRect );
	SurfaceDrawSprite( myRect, blob, state );

	if( charred & 0x0F )
	{
		MRect blobRect, charRect, alphaRect;
		
		CalcBlobRect( (charred & 0x0F), kBombTop-1, &charRect );
		CalcBlobRect( (charred & 0x0F), kBombBottom-1, &alphaRect );
		CalcBlobRect( state, blob-1, &blobRect );

		SurfaceBlitWeightedDualAlpha( SDLU_GetCurrentSurface(),  blobSurface,  charMaskSurface,  blobSurface,  SDLU_GetCurrentSurface(),
                                       myRect,                   &charRect,    &blobRect,        &alphaRect,    myRect, 
                                      (charred & 0xF0) );
	}
}

void SurfaceDrawShadow( const MRect *myRect, int blob, int state )
{
	int x;
	const MPoint offset[4] = { {-2, 0}, {0, -2}, {2, 0}, {0, 2} };
	
	if( blob > kEmpty )
	{
		MRect blobRect, destRect;

		for( x=0; x<4; x++ )
		{
			destRect = *myRect;
			OffsetMRect( &destRect, offset[x].h, offset[x].v );
			
			CalcBlobRect( state, blob-1, &blobRect );
			SurfaceBlitColor( maskSurface,  SDLU_GetCurrentSurface(),
			                  &blobRect,    &destRect, 
			                   0, 0, 0, _5TO8(3) );
		}
	}
}

void SurfaceDrawColor( const MRect *myRect, int blob, int state, int r, int g, int b, int w )
{
	MRect blobRect;
	if( blob > kEmpty )
	{
		CalcBlobRect( state, blob-1, &blobRect );
		SurfaceBlitColor( charMaskSurface,  SDLU_GetCurrentSurface(),
						  &blobRect,         myRect, 
						   r, g, b, w );
	}
}

void SurfaceDrawAlpha( const MRect *myRect, int blob, int mask, int state )
{
	if( blob > kEmpty )
	{
		MRect blobRect, alphaRect;

		CalcBlobRect( state, blob-1, &blobRect );
		CalcBlobRect( state, mask-1, &alphaRect );
		
		SurfaceBlitAlpha( SDLU_GetCurrentSurface(),  blobSurface,  blobSurface, SDLU_GetCurrentSurface(),
		                  myRect,                   &blobRect,    &alphaRect,   myRect );
	}
}

void SurfaceDrawSprite( const MRect *myRect, int blob, int state )
{
	MRect blobRect;
	if( blob > kEmpty )
	{
		CalcBlobRect( state, blob-1, &blobRect );
		SurfaceBlitBlob( &blobRect, myRect );
	}
}


MBoolean PICTExists( int pictID )
{
#ifdef _WIN32
	if( FileExists( QuickResourceName( "PICT", pictID, ".dds" ) ) )
		return true;

	if( FileExists( QuickResourceName( "PICT", pictID, ".DDS" ) ) )
		return true;
#endif

	if( FileExists( QuickResourceName( "PICT", pictID, ".jpg" ) ) )
		return true;

	if( FileExists( QuickResourceName( "PICT", pictID, ".png" ) ) )
		return true;
	
	return false;
}


SDL_Surface* LoadPICTAsSurface( int pictID, int depth )
{
	const char*  filename;
	SDL_Surface* surface;

	filename = QuickResourceName( "PICT", pictID, ".dds" );
	if( !FileExists( filename ) )
	{
		filename = QuickResourceName( "PICT", pictID, ".DDS" );
	}
    if( !FileExists( filename ) )
    {
		filename = QuickResourceName( "PICT", pictID, ".png" );
    }
	if( !FileExists( filename ) )
	{
		filename = QuickResourceName( "PICT", pictID, ".jpg" );
	}
	if( !FileExists( filename ) )
	{
		surface = LoadGraphicAsSurface(QuickResourceName("missing", 0, ".png"), depth);
	}
	else
	{
		surface = LoadGraphicAsSurface(filename, depth);
	}

	return surface;
}

void DrawPICTInSurface( SDL_Surface* surface, int pictID )
{
	SDL_Surface* image;
	
	image = LoadPICTAsSurface( pictID, 0 );
	if( image != NULL )
	{
		SDLU_BlitSurface1to1( image, surface );
		SDL_DestroySurface( image );
	}
}
