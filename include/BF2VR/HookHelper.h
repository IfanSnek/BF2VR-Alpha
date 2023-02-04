#pragma once
#include "../../third-party/minhook/MinHook.h"

class HookHelper
{
public:
	// create a standard hook
	static void createHook(DWORD64 address, void* pHook, void* pOriginal) {

		// if this is the first hook, initialize minhook
		static bool firstTimeHooking = true;
		if (firstTimeHooking) {
			MH_Initialize();
		}

		MH_CreateHook((LPVOID)address, pHook, (LPVOID*)pOriginal);
		MH_EnableHook((LPVOID)address);

	}
	template <typename T>
	static MH_STATUS APIHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHookApi(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
	}
	static void destroyHook(DWORD64 address) {
		MH_DisableHook((LPVOID)address);
		MH_RemoveHook((LPVOID)address);
	}
};

