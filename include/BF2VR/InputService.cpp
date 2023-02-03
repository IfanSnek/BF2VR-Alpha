#include "Utils.h"
#include "InputService.h"

#include <Xinput.h>

namespace BF2VR {
	bool InputService::Connect() {

		client = vigem_alloc();

		if (client == nullptr)
		{
			error("Could not allocate memory for ViGEm.");
			return false;
		}

		const auto retval = vigem_connect(client);
		if (!VIGEM_SUCCESS(retval))
		{
			error("Could not conntect to ViGEm. Error: " + std::to_string(retval));
			return false;
		}

		// Create a controller

		pad = vigem_target_x360_alloc();
		const auto pir = vigem_target_add(client, pad);

		if (!VIGEM_SUCCESS(pir))
		{
			error("Could not plug in ViGEm gamepad. Error: " + std::to_string(pir));
			return false;
		}

		return true;
	}

	void InputService::Update() {

		XINPUT_GAMEPAD gamepad{};
		gamepad.sThumbLX = lX;
		gamepad.sThumbLY = lY;
		gamepad.bRightTrigger = bR;
		gamepad.bLeftTrigger = bL;
		gamepad.wButtons = buttons;

		VIGEM_ERROR err = vigem_target_x360_update(client, pad, *reinterpret_cast<XUSB_REPORT*>(&gamepad));
		if (!VIGEM_SUCCESS(err))
		{
			warn("Could not update the virtual gamepad. Error: " + std::to_string(err));
		}
	}

	void InputService::Disconnect() {
		vigem_disconnect(client);
		vigem_free(client);
	}
}