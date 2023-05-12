// InputService.h - Headers for the code that manages XInput input via
// ViGEm.
// Copyright(C) 2023 Ethan Porcaro

// This program is free software : you can redistribute itand /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

#include <Xinput.h>
#include <windows.h>

#include "../../third-party/ViGEm/Client.h"

namespace BF2VR {
	class InputService {
	public:

		static inline short thumbLX = 0;
		static inline short thumbLY = 0;
		static inline byte leftTrigger = 0;
		static inline byte rightTrigger = 0;
		static inline WORD buttons = 0;
		static inline bool useRight = false;

		static bool connect();
		static void update();
		static void disconnect();

	private:
		static inline PVIGEM_CLIENT pVigemClient;
		static inline PVIGEM_TARGET pVigemGamepad;
	};
}