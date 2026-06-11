#include "uiicons.h"

#include <ctype.h>

#include "gworld.h"
#include "main.h"

#define kIconCacheEntries 256

typedef struct IconCacheEntry
{
	char path[1024];
	SDL_Surface* surface;
} IconCacheEntry;

static IconCacheEntry s_iconCache[kIconCacheEntries];
static int s_iconCacheCount = 0;
static PromptInputSource s_lastPromptInputSource = kPromptInputKeyboardMouse;

static SDL_Surface* LoadCachedIconByPrefix(const char* prefix)
{
	char resolvedPath[1024];

	SDL_strlcpy(resolvedPath, QuickResourceName(prefix, 0, ".png"), sizeof(resolvedPath));
	if (!FileExists(resolvedPath))
	{
		return NULL;
	}

	for (int i = 0; i < s_iconCacheCount; i++)
	{
		if (SDL_strcmp(s_iconCache[i].path, resolvedPath) == 0)
		{
			return s_iconCache[i].surface;
		}
	}

	if (s_iconCacheCount >= kIconCacheEntries)
	{
		return NULL;
	}

	SDL_Surface* surface = LoadGraphicAsSurface(resolvedPath, 32);
	if (!surface)
	{
		return NULL;
	}

	SDL_strlcpy(s_iconCache[s_iconCacheCount].path, resolvedPath, sizeof(s_iconCache[s_iconCacheCount].path));
	s_iconCache[s_iconCacheCount].surface = surface;
	s_iconCacheCount++;
	return surface;
}

static const char* GetThemeFolder(Uint8 theme)
{
	switch (theme)
	{
		case kControllerIconThemeXbox360:		return "ui/controllericons/xbox360";
		case kControllerIconThemeXboxWireless:	return "ui/controllericons/xboxwireless";
		case kControllerIconThemePS3:			return "ui/controllericons/ps3";
		case kControllerIconThemePS4:			return "ui/controllericons/ps4";
		case kControllerIconThemePS5:			return "ui/controllericons/ps5";
		case kControllerIconThemeSwitch:		return "ui/controllericons/switch";
		default:								return NULL;
	}
}

static SDL_Surface* LoadThemeAsset(Uint8 theme, const char* const* assetNames, int assetCount)
{
	const char* folder = GetThemeFolder(theme);
	char prefix[512];

	if (!folder)
	{
		return NULL;
	}

	for (int i = 0; i < assetCount; i++)
	{
		if (!assetNames[i] || !assetNames[i][0])
		{
			continue;
		}

		SDL_snprintf(prefix, sizeof(prefix), "%s/%s", folder, assetNames[i]);
		SDL_Surface* surface = LoadCachedIconByPrefix(prefix);
		if (surface)
		{
			return surface;
		}
	}

	return NULL;
}

static SDL_Surface* LoadFaceButtonIcon(Uint8 theme, SDL_GamepadButtonLabel label)
{
	switch (label)
	{
		case SDL_GAMEPAD_BUTTON_LABEL_A:
		{
			const char* assets[] = { "A" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_B:
		{
			const char* assets[] = { "B" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_X:
		{
			const char* assets[] = { "X" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_Y:
		{
			const char* assets[] = { "Y" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
		{
			const char* assets[] = { "Cross", "PS4_Cross", "PS5_Cross" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_CIRCLE:
		{
			const char* assets[] = { "Circle", "PS4_Circle", "PS5_Circle" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_SQUARE:
		{
			const char* assets[] = { "Square", "PS4_Square", "PS5_Square" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE:
		{
			const char* assets[] = { "Triangle", "PS4_Triangle", "PS5_Triangle" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		default:
			return NULL;
	}
}

static SDL_GamepadType GetThemeGamepadType(Uint8 theme)
{
	switch (theme)
	{
		case kControllerIconThemeXbox360:		return SDL_GAMEPAD_TYPE_XBOX360;
		case kControllerIconThemeXboxWireless:	return SDL_GAMEPAD_TYPE_XBOXONE;
		case kControllerIconThemePS3:			return SDL_GAMEPAD_TYPE_PS3;
		case kControllerIconThemePS4:			return SDL_GAMEPAD_TYPE_PS4;
		case kControllerIconThemePS5:			return SDL_GAMEPAD_TYPE_PS5;
		case kControllerIconThemeSwitch:		return SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO;
		default:								return SDL_GAMEPAD_TYPE_STANDARD;
	}
}

static SDL_Surface* LoadGamepadButtonIcon(Uint8 theme, SDL_GamepadButton button)
{
	if (button >= SDL_GAMEPAD_BUTTON_SOUTH && button <= SDL_GAMEPAD_BUTTON_NORTH)
	{
		SDL_GamepadButtonLabel label = SDL_GetGamepadButtonLabelForType(GetThemeGamepadType(theme), button);
		return LoadFaceButtonIcon(theme, label);
	}

	switch (button)
	{
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
		{
			const char* assets[] = { "Dpad_lf", "dpad_lf", "PS4_Dpad_Left", "Switch_Left" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
		{
			const char* assets[] = { "Dpad_rt", "Dpad_right", "dpad_rt", "PS4_Dpad_Right", "Switch_Right" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_DPAD_UP:
		{
			const char* assets[] = { "Dpad_up", "dpad_up", "PS4_Dpad_Up", "Switch_Up" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
		{
			const char* assets[] = { "Dpad_dn", "dpad_dn", "PS4_Dpad_Down", "Switch_Down" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_START:
		{
			const char* assets[] = { "Start", "menu", "Options", "Plus" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_BACK:
		{
			const char* assets[] = { "Back", "view", "Select", "Share", "Minus" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
		{
			const char* assets[] = { "LB", "L1", "PS4_L1" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
		{
			const char* assets[] = { "RB", "R1", "PS4_R1" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_LEFT_STICK:
		{
			const char* assets[] = { "LS_Click", "LS_click", "L3_Click", "PS4_Left_Stick_Click" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
		{
			const char* assets[] = { "RS_Click", "RS_click", "R3_Click", "PS4_Right_Stick_Click" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_BUTTON_TOUCHPAD:
		{
			const char* assets[] = { "Touchpad", "PS5_Touch_Pad" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		default:
			return NULL;
	}
}

static SDL_Surface* LoadGamepadAxisIcon(Uint8 theme, Uint8 axis)
{
	switch (axis)
	{
		case SDL_GAMEPAD_AXIS_LEFTX:
		case SDL_GAMEPAD_AXIS_LEFTY:
		{
			const char* assets[] = { "LS", "L3", "PS4_Left_Stick" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_AXIS_RIGHTX:
		case SDL_GAMEPAD_AXIS_RIGHTY:
		{
			const char* assets[] = { "RS", "R3", "PS4_Right_Stick" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
		{
			const char* assets[] = { "LT", "L2", "PS4_L2" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
		{
			const char* assets[] = { "RT", "R2", "PS4_R2" };
			return LoadThemeAsset(theme, assets, arrsize(assets));
		}

		default:
			return NULL;
	}
}

static MBoolean BuildKeyboardIconToken(SDL_Keycode key, char* token, size_t tokenSize)
{
	switch (key)
	{
		case SDLK_RETURN:		SDL_strlcpy(token, "Enter", tokenSize); break;
		case SDLK_ESCAPE:		SDL_strlcpy(token, "Esc", tokenSize); break;
		case SDLK_SPACE:		SDL_strlcpy(token, "Space", tokenSize); break;
		case SDLK_BACKSPACE:	SDL_strlcpy(token, "Backspace", tokenSize); break;
		case SDLK_TAB:			SDL_strlcpy(token, "Tab", tokenSize); break;
		case SDLK_LEFT:			SDL_strlcpy(token, "Arrow_Left", tokenSize); break;
		case SDLK_RIGHT:		SDL_strlcpy(token, "Arrow_Right", tokenSize); break;
		case SDLK_UP:			SDL_strlcpy(token, "Arrow_Up", tokenSize); break;
		case SDLK_DOWN:			SDL_strlcpy(token, "Arrow_Down", tokenSize); break;
		case SDLK_LCTRL:
		case SDLK_RCTRL:		SDL_strlcpy(token, "Ctrl", tokenSize); break;
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:		SDL_strlcpy(token, "Shift", tokenSize); break;
		case SDLK_LALT:
		case SDLK_RALT:		SDL_strlcpy(token, "Alt", tokenSize); break;
		case SDLK_MINUS:		SDL_strlcpy(token, "Minus", tokenSize); break;
		case SDLK_EQUALS:		SDL_strlcpy(token, "Plus", tokenSize); break;
		case SDLK_LEFTBRACKET:	SDL_strlcpy(token, "Bracket_Left", tokenSize); break;
		case SDLK_RIGHTBRACKET:	SDL_strlcpy(token, "Bracket_Right", tokenSize); break;
		case SDLK_SEMICOLON:	SDL_strlcpy(token, "Semicolon", tokenSize); break;
		case SDLK_APOSTROPHE:	SDL_strlcpy(token, "Quote", tokenSize); break;
		case SDLK_SLASH:		SDL_strlcpy(token, "Slash", tokenSize); break;
		case SDLK_COMMA:		SDL_strlcpy(token, "Mark_Left", tokenSize); break;
		case SDLK_PERIOD:		SDL_strlcpy(token, "Mark_Right", tokenSize); break;
		default:
		{
			const char* keyName = SDL_GetKeyName(key);
			if (!keyName || !keyName[0] || keyName[1])
			{
				return false;
			}

			token[0] = (char) SDL_toupper((unsigned char) keyName[0]);
			token[1] = '\0';
			break;
		}
	}

	return true;
}

void UIIcons_RecordKeyboardMouseInput(void)
{
	s_lastPromptInputSource = kPromptInputKeyboardMouse;
}

void UIIcons_RecordControllerInput(void)
{
	s_lastPromptInputSource = kPromptInputController;
}

MBoolean UIIcons_ShouldUseControllerPrompts(int player)
{
	return s_lastPromptInputSource == kPromptInputController && IsPlayerControllerConnected(player);
}

SDL_Surface* UIIcons_LoadKeyboardKeyIcon(SDL_Keycode key, MBoolean lightVariant)
{
	char token[128];
	char prefix[512];

	if (!BuildKeyboardIconToken(key, token, sizeof(token)))
	{
		return NULL;
	}

	SDL_snprintf(prefix, sizeof(prefix),
		"ui/controllericons/kbmouse/%s/%s_Key_%s",
		lightVariant ? "Light" : "Dark",
		token,
		lightVariant ? "Light" : "Dark");
	return LoadCachedIconByPrefix(prefix);
}

SDL_Surface* UIIcons_LoadControllerBindingIcon(int player, const ControllerBinding* binding)
{
	Uint8 theme = GetResolvedPlayerControllerIconTheme(player);

	if (!binding)
	{
		return NULL;
	}

	switch (binding->kind)
	{
		case kControllerBindGamepadButton:
			return LoadGamepadButtonIcon(theme, (SDL_GamepadButton) binding->index);

		case kControllerBindGamepadAxis:
			return LoadGamepadAxisIcon(theme, binding->index);

		case kControllerBindJoystickHat:
		{
			if (binding->hatMask == SDL_HAT_LEFT)
			{
				return LoadGamepadButtonIcon(theme, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
			}
			if (binding->hatMask == SDL_HAT_RIGHT)
			{
				return LoadGamepadButtonIcon(theme, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
			}
			if (binding->hatMask == SDL_HAT_UP)
			{
				return LoadGamepadButtonIcon(theme, SDL_GAMEPAD_BUTTON_DPAD_UP);
			}
			if (binding->hatMask == SDL_HAT_DOWN)
			{
				return LoadGamepadButtonIcon(theme, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
			}
			return NULL;
		}

		case kControllerBindJoystickAxis:
		{
			if (binding->index <= 1)
			{
				return LoadGamepadAxisIcon(theme, SDL_GAMEPAD_AXIS_LEFTX);
			}
			if (binding->index <= 3)
			{
				return LoadGamepadAxisIcon(theme, SDL_GAMEPAD_AXIS_RIGHTX);
			}
			return NULL;
		}

		case kControllerBindJoystickButton:
			if (binding->index <= 3)
			{
				return LoadGamepadButtonIcon(theme, (SDL_GamepadButton) (SDL_GAMEPAD_BUTTON_SOUTH + binding->index));
			}
			return NULL;

		default:
			return NULL;
	}
}

SDL_Surface* UIIcons_LoadControllerFaceButtonIcon(int player, SDL_GamepadButton button)
{
	return LoadGamepadButtonIcon(GetResolvedPlayerControllerIconTheme(player), button);
}

void UIIcons_DrawSurfaceContain(SDL_Surface* source, SDL_Surface* dest, const SDL_Rect* destRect)
{
	SDL_Rect target = *destRect;
	float scaleX;
	float scaleY;
	float scale;

	if (!source || !dest || !destRect || destRect->w <= 0 || destRect->h <= 0)
	{
		return;
	}

	scaleX = destRect->w / (float) source->w;
	scaleY = destRect->h / (float) source->h;
	scale = scaleX < scaleY ? scaleX : scaleY;

	target.w = (int) (source->w * scale + 0.5f);
	target.h = (int) (source->h * scale + 0.5f);
	target.x = destRect->x + (destRect->w - target.w) / 2;
	target.y = destRect->y + (destRect->h - target.h) / 2;

	SDL_BlitSurfaceScaled(source, NULL, dest, &target, SDL_SCALEMODE_LINEAR);
}
