#include "cefshim.h"

MBoolean CefIntegrationIsEnabled(void)
{
#if CANDY_ENABLE_CEF
	return true;
#else
	return false;
#endif
}

MBoolean CefIntegrationIsRuntimeAvailable(void)
{
	return false;
}

const char* CefIntegrationStatus(void)
{
#if CANDY_ENABLE_CEF
	return "CEF build flag enabled; runtime bootstrap pending";
#else
	return "CEF SDK not linked";
#endif
}
