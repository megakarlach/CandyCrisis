#ifndef __UIICONS__
#define __UIICONS__

#include "SDLU.h"
#include "keyselect.h"

typedef enum PromptInputSource
{
	kPromptInputKeyboardMouse = 0,
	kPromptInputController
} PromptInputSource;

void UIIcons_RecordKeyboardMouseInput(void);
void UIIcons_RecordControllerInput(void);
MBoolean UIIcons_ShouldUseControllerPrompts(int player);
SDL_Surface* UIIcons_LoadKeyboardKeyIcon(SDL_Keycode key, MBoolean lightVariant);
SDL_Surface* UIIcons_LoadControllerBindingIcon(int player, const ControllerBinding* binding);
SDL_Surface* UIIcons_LoadControllerFaceButtonIcon(int player, SDL_GamepadButton button);
void UIIcons_DrawSurfaceContain(SDL_Surface* source, SDL_Surface* dest, const SDL_Rect* destRect);

#endif
