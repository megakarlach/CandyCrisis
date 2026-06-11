// keyselect.h

#ifndef __KEYSELECT_H__
#define __KEYSELECT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ControllerBindingKind
{
	kControllerBindNone = 0,
	kControllerBindGamepadButton,
	kControllerBindGamepadAxis,
	kControllerBindJoystickButton,
	kControllerBindJoystickAxis,
	kControllerBindJoystickHat
} ControllerBindingKind;

typedef struct ControllerBinding
{
	Uint8 kind;
	Uint8 index;
	Sint8 direction;
	Uint8 hatMask;
} ControllerBinding;

typedef enum ControllerIconTheme
{
	kControllerIconThemeAuto = 0,
	kControllerIconThemeXbox360,
	kControllerIconThemeXboxWireless,
	kControllerIconThemePS3,
	kControllerIconThemePS4,
	kControllerIconThemePS5,
	kControllerIconThemeSwitch,
	kControllerIconThemeGeneric1,
	kControllerIconThemeGeneric2,
	kControllerIconThemeGeneric3,
	kControllerIconThemeGeneric4,
	kControllerIconThemeCount
} ControllerIconTheme;

void CheckKeys();
MBoolean IsPlayerControllerConnected(int player);
MBoolean IsAnyPlayerControllerInputPressed(int player);
MBoolean CapturePlayerControllerBinding(int player, ControllerBinding* outBinding);
const char* GetPlayerControllerName(int player);
void GetControllerBindingName(int player, int action, char* buf, size_t bufSize);
int GetControllerIconThemeCount(void);
const char* GetControllerIconThemeName(Uint8 theme);
Uint8 GetPlayerControllerIconTheme(int player);
void SetPlayerControllerIconTheme(int player, Uint8 theme);
Uint8 GetResolvedPlayerControllerIconTheme(int player);
MBoolean AnyMenuControllerConfirmPressed(void);
MBoolean AnyMenuControllerCancelPressed(void);
MBoolean AnyMenuControllerUpPressed(void);
MBoolean AnyMenuControllerDownPressed(void);

extern SDL_Keycode playerKeys[2][4];
extern const SDL_Keycode defaultPlayerKeys[2][4];
extern ControllerBinding playerControllerBindings[2][4];
extern const ControllerBinding defaultPlayerControllerBindings[2][4];
extern Uint8 playerControllerIconThemes[2];

#ifdef __cplusplus
}
#endif

#endif
