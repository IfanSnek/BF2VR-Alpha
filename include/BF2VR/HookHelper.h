#pragma once
#include "../../third-party/minhook/MinHook.h"

class HookHelper
{
public:
	// create a standard hook
	static void CreateHook(DWORD64 address, void* pHook, void* original) {
		// if this is the first hook, initialize minhook
		static bool firstTime = true;
		if (firstTime) {
			MH_Initialize();
		}
		// then create the hook, and enable it
		MH_CreateHook((LPVOID)address, pHook, (LPVOID*)original);
		MH_EnableHook((LPVOID)address);

	}
	template <typename T>
	static MH_STATUS APIHook(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, T** ppOriginal)
	{
		return MH_CreateHookApi(pszModule, pszProcName, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
	}
	static void DestroyHook(DWORD64 address) {
		MH_DisableHook((LPVOID)address);
		MH_RemoveHook((LPVOID)address);
	}
};

