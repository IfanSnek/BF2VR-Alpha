#pragma once
#include <string>
#include "HookHelper.h"
#include "SDK.h"
#include <fstream>

namespace BF2VR {

	inline HMODULE OwnModule;
	inline HWND OwnWindow;
	

	static std::ofstream Log("logs.txt", std::ios_base::app);

	void log(std::string message);
	
	bool IsValidPtr(PVOID p);

	void Shutdown();

	void ShutdownNoHooks();

	void LoadConfig();

	void SaveConfig();

	Vec3 EulerFromQuat(Vec4 q);

	// Config settings

	inline float RATIO = 1;
	inline float FOV = 90.0f;
	inline bool Reconfig = false;

	const inline float pi = 3.14159265358979323846f;
}