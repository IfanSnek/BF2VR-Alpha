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