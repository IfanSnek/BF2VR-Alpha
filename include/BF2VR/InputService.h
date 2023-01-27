#include <Xinput.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <Windows.h>

#include "../../third-party/minhook/MinHook.h"

namespace BF2VR {
	class InputService {
	public:
		static inline XINPUT_STATE* state = nullptr;

		static inline int lX = 0;
		static inline int lY = 0;

		static inline unsigned int packet = 0;

		static bool HookXInput();

		static void UnhookXInput();

	private:
		static DWORD detourXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);

		typedef DWORD(*XINPUTGETSTATE)(DWORD, XINPUT_STATE*);
		static inline XINPUTGETSTATE hookedXInputGetState = nullptr;
	};
}