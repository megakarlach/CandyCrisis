#include "webmshim.h"

MBoolean WebMPlaybackIsEnabled(void)
{
#if CANDY_ENABLE_WEBM
	return true;
#else
	return false;
#endif
}

MBoolean WebMPlaybackIsRuntimeAvailable(void)
{
	return false;
}

const char* WebMPlaybackStatus(void)
{
#if CANDY_ENABLE_WEBM
	return "WebM build flag enabled; runtime decoder pending";
#else
	return "WebM backend not linked";
#endif
}
