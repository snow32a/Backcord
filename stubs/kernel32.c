#include <windows.h>

BOOL WINAPI IsProcessorFeaturePresent(DWORD ProcessorFeature)
{
	return FALSE;
}

FARPROC WINAPI DelayLoadFailureHook(_In_ LPCSTR pszDllName,
                                    _In_ LPCSTR pszProcName) {
	return (void*)(IsProcessorFeaturePresent);
}