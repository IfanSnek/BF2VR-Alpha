// Utils.h - Headers for various utilities like logging, config loading and saving,
// and a few math ones.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

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
	inline bool NOROT = false;

	const inline float PI = 3.14159265358979323846f;
}