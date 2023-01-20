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

	Vec3 EulerFromQuat(Vec4 q);

	// Config settings

	inline float IPD = 0.0064f;
	inline float RATIO = 0.95416f;
	inline float leftmin = -0.1f;
	inline float leftmax = 1.0f;
	inline float rightmin = 0.1f;
	inline float rightmax = 1.1f;
	inline float VRHeight = 2.0f;
	inline float FOV = 91.0f;

	const inline double pi = 3.14159265358979323846;
}