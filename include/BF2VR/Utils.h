#pragma once
#include <string>
#include <vector>
#include "Types.h"
#include "../../third-party/minhook/MinHook.h"
#include <fstream>
#include <iostream>

namespace BF2VR {

	inline HMODULE ownModule;
	inline HWND ownWindow;
	

	static std::ofstream logFile("logs.txt", std::ios_base::app);

	void log(std::string message);
	void error(std::string message);

	static inline int warnCount = 0;
	static inline std::string lastWarn = "";
	void warn(std::string message);

	void success(std::string message);
	void info(std::string message);
	void deb(std::string message);

	void shutdown();

	void shutdownNoHooks();

	void loadConfig();

	void saveConfig();

	Vec3 eulerFromQuat(Vec4 q);
	Vec4 quatFromEuler(Vec3 e);


	// Config settings

	inline float RATIO = 1;
	inline float FOV = 90.0f;
	inline bool doReconfig = false;
	inline bool NOFOV = false;
	inline bool HEADAIM = false;

	const inline float PI = 3.14159265358979323846f;
}