// keyselect.c

#include "SDLU.h"

#include <ctype.h>

#include "main.h"
#include "players.h"
#include "gameticks.h"
#include "keyselect.h"

#define kNumControls 4
#define kControllerAxisDeadZone 16000

typedef struct ControllerSlot
{
	SDL_JoystickID instanceID;
	SDL_Gamepad* gamepad;
	SDL_Joystick* joystick;
	MBoolean isGamepad;
	char name[128];
} ControllerSlot;

static ControllerSlot s_controllerSlots[2];
static MTicks s_lastControllerScan = (MTicks) -1;

SDL_Keycode playerKeys[2][4] =
{
	{ SDLK_A, SDLK_D, SDLK_X, SDLK_S },
	{ SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN, SDLK_UP }
};

const SDL_Keycode defaultPlayerKeys[2][4] =
{
	{ SDLK_A, SDLK_D, SDLK_X, SDLK_S },
	{ SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN, SDLK_UP }
};

const ControllerBinding defaultPlayerControllerBindings[2][4] =
{
	{
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_WEST, 0, 0 },
	},
	{
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_WEST, 0, 0 },
	},
};

ControllerBinding playerControllerBindings[2][4] =
{
	{
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_WEST, 0, 0 },
	},
	{
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_DPAD_DOWN, 0, 0 },
		{ kControllerBindGamepadButton, SDL_GAMEPAD_BUTTON_WEST, 0, 0 },
	},
};

Uint8 playerControllerIconThemes[2] =
{
	kControllerIconThemeAuto,
	kControllerIconThemeAuto
};

typedef struct MenuControllerState
{
	MBoolean confirm;
	MBoolean cancel;
	MBoolean up;
	MBoolean down;
} MenuControllerState;

static MenuControllerState s_prevMenuControllerState[2];
static MenuControllerState s_menuControllerPressed[2];
static MTicks s_lastMenuControllerPoll = (MTicks) -1;

static MBoolean NameContainsInsensitive(const char* haystack, const char* needle)
{
	char haystackLower[128];
	char needleLower[64];
	size_t haystackLength;
	size_t needleLength;

	if (!haystack || !needle)
	{
		return false;
	}

	haystackLength = SDL_strlen(haystack);
	needleLength = SDL_strlen(needle);
	if (needleLength == 0)
	{
		return true;
	}
	if (haystackLength >= sizeof(haystackLower))
	{
		haystackLength = sizeof(haystackLower) - 1;
	}
	if (needleLength >= sizeof(needleLower))
	{
		needleLength = sizeof(needleLower) - 1;
	}

	for (size_t i = 0; i < haystackLength; i++)
	{
		haystackLower[i] = (char) tolower((unsigned char) haystack[i]);
	}
	haystackLower[haystackLength] = '\0';

	for (size_t i = 0; i < needleLength; i++)
	{
		needleLower[i] = (char) tolower((unsigned char) needle[i]);
	}
	needleLower[needleLength] = '\0';

	return SDL_strstr(haystackLower, needleLower) != NULL;
}

static void CloseControllerSlot(ControllerSlot* slot)
{
	if (slot->gamepad)
	{
		SDL_CloseGamepad(slot->gamepad);
		slot->gamepad = NULL;
	}
	if (slot->joystick)
	{
		SDL_CloseJoystick(slot->joystick);
		slot->joystick = NULL;
	}

	slot->instanceID = 0;
	slot->isGamepad = false;
	SDL_strlcpy(slot->name, "No controller", sizeof(slot->name));
}

static void AssignControllerSlot(ControllerSlot* slot, SDL_JoystickID instanceID, MBoolean isGamepad)
{
	CloseControllerSlot(slot);

	slot->instanceID = instanceID;
	slot->isGamepad = isGamepad;

	if (isGamepad)
	{
		slot->gamepad = SDL_OpenGamepad(instanceID);
		if (!slot->gamepad)
		{
			CloseControllerSlot(slot);
			return;
		}

		const char* name = SDL_GetGamepadName(slot->gamepad);
		SDL_strlcpy(slot->name, name ? name : "Gamepad", sizeof(slot->name));
	}
	else
	{
		slot->joystick = SDL_OpenJoystick(instanceID);
		if (!slot->joystick)
		{
			CloseControllerSlot(slot);
			return;
		}

		const char* name = SDL_GetJoystickName(slot->joystick);
		SDL_strlcpy(slot->name, name ? name : "Joystick", sizeof(slot->name));
	}
}

static void EnsureControllerSlotsUpToDate(void)
{
	MTicks now = MTickCount();
	if (s_lastControllerScan == now)
	{
		return;
	}
	s_lastControllerScan = now;

	SDL_JoystickID assignedIDs[2] = { 0, 0 };
	MBoolean assignedIsGamepad[2] = { false, false };
	int assignedCount = 0;

	int numGamepads = 0;
	SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
	for (int i = 0; i < numGamepads && assignedCount < arrsize(assignedIDs); i++)
	{
		assignedIDs[assignedCount] = gamepads[i];
		assignedIsGamepad[assignedCount] = true;
		assignedCount++;
	}
	SDL_free(gamepads);

	if (assignedCount < arrsize(assignedIDs))
	{
		int numJoysticks = 0;
		SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
		for (int i = 0; i < numJoysticks && assignedCount < arrsize(assignedIDs); i++)
		{
			if (SDL_IsGamepad(joysticks[i]))
			{
				continue;
			}

			assignedIDs[assignedCount] = joysticks[i];
			assignedIsGamepad[assignedCount] = false;
			assignedCount++;
		}
		SDL_free(joysticks);
	}

	for (int i = 0; i < arrsize(s_controllerSlots); i++)
	{
		if (i >= assignedCount)
		{
			CloseControllerSlot(&s_controllerSlots[i]);
			continue;
		}

		if (s_controllerSlots[i].instanceID != assignedIDs[i]
			|| s_controllerSlots[i].isGamepad != assignedIsGamepad[i]
			|| (assignedIsGamepad[i] && !SDL_GamepadConnected(s_controllerSlots[i].gamepad))
			|| (!assignedIsGamepad[i] && (!s_controllerSlots[i].joystick || !SDL_JoystickConnected(s_controllerSlots[i].joystick))))
		{
			AssignControllerSlot(&s_controllerSlots[i], assignedIDs[i], assignedIsGamepad[i]);
		}
	}
}

static Uint8 ResolveThemeForGamepad(const ControllerSlot* slot)
{
	SDL_GamepadType type;
	Uint16 vendor;
	const char* name;

	if (!slot || !slot->gamepad)
	{
		return kControllerIconThemeGeneric1;
	}

	type = SDL_GetRealGamepadType(slot->gamepad);
	if (type == SDL_GAMEPAD_TYPE_STANDARD)
	{
		type = SDL_GetGamepadType(slot->gamepad);
	}

	switch (type)
	{
		case SDL_GAMEPAD_TYPE_XBOX360:
			return kControllerIconThemeXbox360;

		case SDL_GAMEPAD_TYPE_XBOXONE:
			return kControllerIconThemeXboxWireless;

		case SDL_GAMEPAD_TYPE_PS3:
			return kControllerIconThemePS3;

		case SDL_GAMEPAD_TYPE_PS4:
			return kControllerIconThemePS4;

		case SDL_GAMEPAD_TYPE_PS5:
			return kControllerIconThemePS5;

		case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
			return kControllerIconThemeSwitch;

		default:
			break;
	}

	vendor = SDL_GetGamepadVendor(slot->gamepad);
	switch (vendor)
	{
		case 0x045E:
			return kControllerIconThemeXboxWireless;

		case 0x054C:
		{
			Uint16 product = SDL_GetGamepadProduct(slot->gamepad);
			if (product == 0x0268)
			{
				return kControllerIconThemePS3;
			}
			if (product == 0x09CC || product == 0x05C4)
			{
				return kControllerIconThemePS4;
			}
			if (product == 0x0CE6)
			{
				return kControllerIconThemePS5;
			}
			break;
		}

		case 0x057E:
			return kControllerIconThemeSwitch;
	}

	name = slot->name;
	if (NameContainsInsensitive(name, "xbox 360"))
	{
		return kControllerIconThemeXbox360;
	}
	if (NameContainsInsensitive(name, "xbox"))
	{
		return kControllerIconThemeXboxWireless;
	}
	if (NameContainsInsensitive(name, "dualsense") || NameContainsInsensitive(name, "ps5"))
	{
		return kControllerIconThemePS5;
	}
	if (NameContainsInsensitive(name, "dualshock 4") || NameContainsInsensitive(name, "ps4"))
	{
		return kControllerIconThemePS4;
	}
	if (NameContainsInsensitive(name, "dualshock 3") || NameContainsInsensitive(name, "sixaxis") || NameContainsInsensitive(name, "ps3"))
	{
		return kControllerIconThemePS3;
	}
	if (NameContainsInsensitive(name, "switch") || NameContainsInsensitive(name, "joy-con") || NameContainsInsensitive(name, "pro controller"))
	{
		return kControllerIconThemeSwitch;
	}

	return kControllerIconThemeGeneric1;
}

static Uint8 ResolveThemeForJoystick(const ControllerSlot* slot)
{
	const char* name = slot ? slot->name : NULL;

	if (NameContainsInsensitive(name, "xbox 360"))
	{
		return kControllerIconThemeXbox360;
	}
	if (NameContainsInsensitive(name, "xbox"))
	{
		return kControllerIconThemeXboxWireless;
	}
	if (NameContainsInsensitive(name, "dualsense") || NameContainsInsensitive(name, "ps5"))
	{
		return kControllerIconThemePS5;
	}
	if (NameContainsInsensitive(name, "dualshock 4") || NameContainsInsensitive(name, "ps4"))
	{
		return kControllerIconThemePS4;
	}
	if (NameContainsInsensitive(name, "dualshock 3") || NameContainsInsensitive(name, "sixaxis") || NameContainsInsensitive(name, "ps3"))
	{
		return kControllerIconThemePS3;
	}
	if (NameContainsInsensitive(name, "switch") || NameContainsInsensitive(name, "joy-con") || NameContainsInsensitive(name, "pro controller"))
	{
		return kControllerIconThemeSwitch;
	}

	return kControllerIconThemeGeneric1;
}

static Uint8 ResolveControllerIconTheme(int player)
{
	const ControllerSlot* slot;

	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots))
	{
		return kControllerIconThemeXbox360;
	}

	slot = &s_controllerSlots[player];
	if ((slot->isGamepad && (!slot->gamepad || !SDL_GamepadConnected(slot->gamepad)))
		|| (!slot->isGamepad && (!slot->joystick || !SDL_JoystickConnected(slot->joystick))))
	{
		return kControllerIconThemeXbox360;
	}

	if (slot->isGamepad)
	{
		return ResolveThemeForGamepad(slot);
	}
	return ResolveThemeForJoystick(slot);
}

int GetControllerIconThemeCount(void)
{
	return kControllerIconThemeCount;
}

const char* GetControllerIconThemeName(Uint8 theme)
{
	switch (theme)
	{
		case kControllerIconThemeAuto:			return "Auto detect";
		case kControllerIconThemeXbox360:		return "Xbox 360";
		case kControllerIconThemeXboxWireless:	return "Xbox One/Series";
		case kControllerIconThemePS3:			return "PS3";
		case kControllerIconThemePS4:			return "PS4";
		case kControllerIconThemePS5:			return "PS5";
		case kControllerIconThemeSwitch:		return "Nintendo";
		case kControllerIconThemeGeneric1:		return "Generic 1";
		case kControllerIconThemeGeneric2:		return "Generic 2";
		case kControllerIconThemeGeneric3:		return "Generic 3";
		case kControllerIconThemeGeneric4:		return "Generic 4";
		default:								return "Auto detect";
	}
}

Uint8 GetPlayerControllerIconTheme(int player)
{
	if (player < 0 || player >= arrsize(playerControllerIconThemes))
	{
		return kControllerIconThemeAuto;
	}
	return playerControllerIconThemes[player];
}

void SetPlayerControllerIconTheme(int player, Uint8 theme)
{
	if (player < 0 || player >= arrsize(playerControllerIconThemes))
	{
		return;
	}

	if (theme >= kControllerIconThemeCount)
	{
		theme = kControllerIconThemeAuto;
	}

	playerControllerIconThemes[player] = theme;
}

Uint8 GetResolvedPlayerControllerIconTheme(int player)
{
	Uint8 selectedTheme = GetPlayerControllerIconTheme(player);
	if (selectedTheme == kControllerIconThemeAuto)
	{
		return ResolveControllerIconTheme(player);
	}
	return selectedTheme;
}

static MBoolean IsBindingPressedOnSlot(const ControllerSlot* slot, const ControllerBinding* binding)
{
	if (slot->isGamepad)
	{
		if (!slot->gamepad || !SDL_GamepadConnected(slot->gamepad))
		{
			return false;
		}

		switch (binding->kind)
		{
			case kControllerBindGamepadButton:
				return SDL_GetGamepadButton(slot->gamepad, (SDL_GamepadButton) binding->index);

			case kControllerBindGamepadAxis:
			{
				Sint16 value = SDL_GetGamepadAxis(slot->gamepad, (SDL_GamepadAxis) binding->index);
				return binding->direction < 0
					? value <= -kControllerAxisDeadZone
					: value >= +kControllerAxisDeadZone;
			}

			default:
				return false;
		}
	}

	if (!slot->joystick || !SDL_JoystickConnected(slot->joystick))
	{
		return false;
	}

	switch (binding->kind)
	{
		case kControllerBindJoystickButton:
			return SDL_GetJoystickButton(slot->joystick, binding->index);

		case kControllerBindJoystickAxis:
		{
			Sint16 value = SDL_GetJoystickAxis(slot->joystick, binding->index);
			return binding->direction < 0
				? value <= -kControllerAxisDeadZone
				: value >= +kControllerAxisDeadZone;
		}

		case kControllerBindJoystickHat:
			return (SDL_GetJoystickHat(slot->joystick, binding->index) & binding->hatMask) != 0;

		default:
			return false;
	}
}

static const char* GetGamepadAxisName(Uint8 axis, Sint8 direction)
{
	switch (axis)
	{
		case SDL_GAMEPAD_AXIS_LEFTX:			return direction < 0 ? "LS Left"  : "LS Right";
		case SDL_GAMEPAD_AXIS_LEFTY:			return direction < 0 ? "LS Up"    : "LS Down";
		case SDL_GAMEPAD_AXIS_RIGHTX:			return direction < 0 ? "RS Left"  : "RS Right";
		case SDL_GAMEPAD_AXIS_RIGHTY:			return direction < 0 ? "RS Up"    : "RS Down";
		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:		return "LT";
		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:	return "RT";
		default:								return "Axis";
	}
}

static const char* GetJoystickAxisName(Uint8 axis, Sint8 direction)
{
	static char name[32];
	SDL_snprintf(name, sizeof(name), "Axis %d%s", axis, direction < 0 ? "-" : "+");
	return name;
}

static const char* GetJoystickHatName(Uint8 hatMask)
{
	switch (hatMask)
	{
		case SDL_HAT_UP:		return "Hat Up";
		case SDL_HAT_DOWN:		return "Hat Down";
		case SDL_HAT_LEFT:		return "Hat Left";
		case SDL_HAT_RIGHT:		return "Hat Right";
		default:				return "Hat";
	}
}

static void DescribeControllerBinding(const ControllerBinding* binding, char* buf, size_t bufSize)
{
	switch (binding->kind)
	{
		case kControllerBindNone:
			SDL_strlcpy(buf, "None", bufSize);
			break;

		case kControllerBindGamepadButton:
		{
			const char* name = SDL_GetGamepadStringForButton((SDL_GamepadButton) binding->index);
			SDL_strlcpy(buf, name ? name : "Pad", bufSize);
			break;
		}

		case kControllerBindGamepadAxis:
			SDL_strlcpy(buf, GetGamepadAxisName(binding->index, binding->direction), bufSize);
			break;

		case kControllerBindJoystickButton:
			SDL_snprintf(buf, bufSize, "Btn %d", binding->index + 1);
			break;

		case kControllerBindJoystickAxis:
			SDL_strlcpy(buf, GetJoystickAxisName(binding->index, binding->direction), bufSize);
			break;

		case kControllerBindJoystickHat:
			SDL_strlcpy(buf, GetJoystickHatName(binding->hatMask), bufSize);
			break;

		default:
			SDL_strlcpy(buf, "???", bufSize);
			break;
	}
}

static MBoolean CaptureBindingFromSlot(const ControllerSlot* slot, ControllerBinding* outBinding)
{
	if (slot->isGamepad)
	{
		if (!slot->gamepad || !SDL_GamepadConnected(slot->gamepad))
		{
			return false;
		}

		for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
		{
			if (SDL_GetGamepadButton(slot->gamepad, (SDL_GamepadButton) button))
			{
				outBinding->kind = kControllerBindGamepadButton;
				outBinding->index = button;
				outBinding->direction = 0;
				outBinding->hatMask = 0;
				return true;
			}
		}

		for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
		{
			Sint16 value = SDL_GetGamepadAxis(slot->gamepad, (SDL_GamepadAxis) axis);
			if (value <= -kControllerAxisDeadZone || value >= +kControllerAxisDeadZone)
			{
				outBinding->kind = kControllerBindGamepadAxis;
				outBinding->index = axis;
				outBinding->direction = value < 0 ? -1 : 1;
				outBinding->hatMask = 0;
				return true;
			}
		}

		return false;
	}

	if (!slot->joystick || !SDL_JoystickConnected(slot->joystick))
	{
		return false;
	}

	for (int button = 0; button < SDL_GetNumJoystickButtons(slot->joystick); button++)
	{
		if (SDL_GetJoystickButton(slot->joystick, button))
		{
			outBinding->kind = kControllerBindJoystickButton;
			outBinding->index = button;
			outBinding->direction = 0;
			outBinding->hatMask = 0;
			return true;
		}
	}

	for (int hat = 0; hat < SDL_GetNumJoystickHats(slot->joystick); hat++)
	{
		Uint8 hatValue = SDL_GetJoystickHat(slot->joystick, hat);
		static const Uint8 maskOrder[] = { SDL_HAT_UP, SDL_HAT_DOWN, SDL_HAT_LEFT, SDL_HAT_RIGHT };
		for (int i = 0; i < arrsize(maskOrder); i++)
		{
			if (hatValue & maskOrder[i])
			{
				outBinding->kind = kControllerBindJoystickHat;
				outBinding->index = hat;
				outBinding->direction = 0;
				outBinding->hatMask = maskOrder[i];
				return true;
			}
		}
	}

	for (int axis = 0; axis < SDL_GetNumJoystickAxes(slot->joystick); axis++)
	{
		Sint16 value = SDL_GetJoystickAxis(slot->joystick, axis);
		if (value <= -kControllerAxisDeadZone || value >= +kControllerAxisDeadZone)
		{
			outBinding->kind = kControllerBindJoystickAxis;
			outBinding->index = axis;
			outBinding->direction = value < 0 ? -1 : 1;
			outBinding->hatMask = 0;
			return true;
		}
	}

	return false;
}

MBoolean IsPlayerControllerConnected(int player)
{
	EnsureControllerSlotsUpToDate();
	return player >= 0
		&& player < arrsize(s_controllerSlots)
		&& ((s_controllerSlots[player].gamepad && SDL_GamepadConnected(s_controllerSlots[player].gamepad))
			|| (s_controllerSlots[player].joystick && SDL_JoystickConnected(s_controllerSlots[player].joystick)));
}

MBoolean IsAnyPlayerControllerInputPressed(int player)
{
	ControllerBinding binding;
	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots))
	{
		return false;
	}

	return CaptureBindingFromSlot(&s_controllerSlots[player], &binding);
}

MBoolean CapturePlayerControllerBinding(int player, ControllerBinding* outBinding)
{
	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots) || !outBinding)
	{
		return false;
	}

	return CaptureBindingFromSlot(&s_controllerSlots[player], outBinding);
}

const char* GetPlayerControllerName(int player)
{
	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots))
	{
		return "No controller";
	}

	return s_controllerSlots[player].name;
}

void GetControllerBindingName(int player, int action, char* buf, size_t bufSize)
{
	if (!buf || bufSize == 0)
	{
		return;
	}

	if (player < 0 || player >= arrsize(playerControllerBindings) || action < 0 || action >= kNumControls)
	{
		SDL_strlcpy(buf, "???", bufSize);
		return;
	}

	DescribeControllerBinding(&playerControllerBindings[player][action], buf, bufSize);
}

static MBoolean IsControllerActionPressed(int player, int action)
{
	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots) || action < 0 || action >= kNumControls)
	{
		return false;
	}

	return IsBindingPressedOnSlot(&s_controllerSlots[player], &playerControllerBindings[player][action]);
}

static MenuControllerState GetMenuControllerState(int player)
{
	MenuControllerState state = { false, false, false, false };
	const ControllerSlot* slot;

	EnsureControllerSlotsUpToDate();
	if (player < 0 || player >= arrsize(s_controllerSlots))
	{
		return state;
	}

	slot = &s_controllerSlots[player];
	if (slot->isGamepad)
	{
		if (!slot->gamepad || !SDL_GamepadConnected(slot->gamepad))
		{
			return state;
		}

		state.confirm =
			SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_SOUTH)
			|| SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_START);
		state.cancel =
			SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_EAST)
			|| SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_BACK);
		state.up =
			SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)
			|| SDL_GetGamepadAxis(slot->gamepad, SDL_GAMEPAD_AXIS_LEFTY) <= -kControllerAxisDeadZone;
		state.down =
			SDL_GetGamepadButton(slot->gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)
			|| SDL_GetGamepadAxis(slot->gamepad, SDL_GAMEPAD_AXIS_LEFTY) >= kControllerAxisDeadZone;
		return state;
	}

	if (!slot->joystick || !SDL_JoystickConnected(slot->joystick))
	{
		return state;
	}

	state.confirm = SDL_GetJoystickButton(slot->joystick, 0) || SDL_GetJoystickButton(slot->joystick, 7);
	state.cancel = SDL_GetJoystickButton(slot->joystick, 1) || SDL_GetJoystickButton(slot->joystick, 6);
	state.up =
		(SDL_GetNumJoystickHats(slot->joystick) > 0 && (SDL_GetJoystickHat(slot->joystick, 0) & SDL_HAT_UP))
		|| (SDL_GetNumJoystickAxes(slot->joystick) > 1 && SDL_GetJoystickAxis(slot->joystick, 1) <= -kControllerAxisDeadZone);
	state.down =
		(SDL_GetNumJoystickHats(slot->joystick) > 0 && (SDL_GetJoystickHat(slot->joystick, 0) & SDL_HAT_DOWN))
		|| (SDL_GetNumJoystickAxes(slot->joystick) > 1 && SDL_GetJoystickAxis(slot->joystick, 1) >= kControllerAxisDeadZone);
	return state;
}

typedef enum MenuPressKind
{
	kMenuPressConfirm,
	kMenuPressCancel,
	kMenuPressUp,
	kMenuPressDown
} MenuPressKind;

static void UpdateMenuControllerPresses(void)
{
	MTicks now = MTickCount();

	if (s_lastMenuControllerPoll == now)
	{
		return;
	}

	s_lastMenuControllerPoll = now;

	for (int player = 0; player < arrsize(s_controllerSlots); player++)
	{
		MenuControllerState state = GetMenuControllerState(player);

		s_menuControllerPressed[player].confirm = state.confirm && !s_prevMenuControllerState[player].confirm;
		s_menuControllerPressed[player].cancel = state.cancel && !s_prevMenuControllerState[player].cancel;
		s_menuControllerPressed[player].up = state.up && !s_prevMenuControllerState[player].up;
		s_menuControllerPressed[player].down = state.down && !s_prevMenuControllerState[player].down;
		s_prevMenuControllerState[player] = state;
	}
}

static MBoolean WasMenuStateJustPressed(MenuPressKind kind)
{
	UpdateMenuControllerPresses();

	for (int player = 0; player < arrsize(s_controllerSlots); player++)
	{
		MBoolean currentField = false;

		switch (kind)
		{
			case kMenuPressConfirm:
				currentField = s_menuControllerPressed[player].confirm;
				break;

			case kMenuPressCancel:
				currentField = s_menuControllerPressed[player].cancel;
				break;

			case kMenuPressUp:
				currentField = s_menuControllerPressed[player].up;
				break;

			case kMenuPressDown:
				currentField = s_menuControllerPressed[player].down;
				break;
		}
		if (currentField)
		{
			return true;
		}
	}

	return false;
}

MBoolean AnyMenuControllerConfirmPressed(void)
{
	return WasMenuStateJustPressed(kMenuPressConfirm);
}

MBoolean AnyMenuControllerCancelPressed(void)
{
	return WasMenuStateJustPressed(kMenuPressCancel);
}

MBoolean AnyMenuControllerUpPressed(void)
{
	return WasMenuStateJustPressed(kMenuPressUp);
}

MBoolean AnyMenuControllerDownPressed(void)
{
	return WasMenuStateJustPressed(kMenuPressDown);
}

void CheckKeys()
{
	int player;
	int arraySize;
	const bool* pressedKeys;

	SDLU_PumpEvents();
	pressedKeys = SDL_GetKeyboardState(&arraySize);

	for (player = 0; player < 2; player++)
	{
		if (pressedKeys[SDL_GetScancodeFromKey(playerKeys[player][0], NULL)] || IsControllerActionPressed(player, 0))
			hitKey[player].left++;
		else
			hitKey[player].left = 0;

		if (pressedKeys[SDL_GetScancodeFromKey(playerKeys[player][1], NULL)] || IsControllerActionPressed(player, 1))
			hitKey[player].right++;
		else
			hitKey[player].right = 0;

		if (pressedKeys[SDL_GetScancodeFromKey(playerKeys[player][2], NULL)] || IsControllerActionPressed(player, 2))
			hitKey[player].drop++;
		else
			hitKey[player].drop = 0;

		if (pressedKeys[SDL_GetScancodeFromKey(playerKeys[player][3], NULL)] || IsControllerActionPressed(player, 3))
			hitKey[player].rotate++;
		else
			hitKey[player].rotate = 0;
	}

	pauseKey = pressedKeys[SDL_SCANCODE_ESCAPE]
		|| (s_controllerSlots[0].gamepad && SDL_GetGamepadButton(s_controllerSlots[0].gamepad, SDL_GAMEPAD_BUTTON_START));
}
