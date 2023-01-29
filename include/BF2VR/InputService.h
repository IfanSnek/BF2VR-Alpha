#include <Xinput.h>
#define WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../third-party/ViGEm/Client.h"

namespace BF2VR {
	class InputService {
	public:

		static inline int lX = 0;
		static inline int lY = 0;
		static inline byte bL = 0;
		static inline byte bR = 0;
		static inline WORD buttons = 0;

		static bool Connect();
		static void Update();
		static void Disconnect();

	private:
		static inline PVIGEM_CLIENT client;
		static inline PVIGEM_TARGET pad;
	};
}