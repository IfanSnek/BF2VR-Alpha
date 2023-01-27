#include "Utils.h"
#include "InputService.h"

namespace BF2VR {

	DWORD InputService::detourXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
	{
		DWORD toReturn = hookedXInputGetState(dwUserIndex, pState);

		packet++;

		pState->Gamepad.sThumbLX = lX;
		pState->Gamepad.sThumbLY = lY;
		pState->Gamepad.wButtons = 0x0040; // Sprint
		pState->dwPacketNumber = packet;

		return toReturn;
	}

	bool InputService::HookXInput() {

		MH_STATUS status = MH_CreateHookApi(L"XINPUT1_4", "XInputGetState", reinterpret_cast<LPVOID>(&detourXInputGetState), reinterpret_cast<LPVOID*>(&hookedXInputGetState));
		if (status != MH_OK)
		{
			log("Could not create hook for xinput" + std::to_string(status));
			return false;
		}
		status = MH_EnableHook(MH_ALL_HOOKS);
		if (status != MH_OK)
		{
			log("Could not enable hook for xinput");
			return false;
		}
		return true;
	}

	void InputService::UnhookXInput() {
		MH_DisableHook(&detourXInputGetState);
		MH_RemoveHook(&detourXInputGetState);
	}
}