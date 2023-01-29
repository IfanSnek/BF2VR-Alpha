#include "Utils.h"
#include "InputService.h"

#include <Xinput.h>

namespace BF2VR {
	bool InputService::Connect() {

		client = vigem_alloc();

		if (client == nullptr)
		{
			log("Could not allocate memory for ViGEm");
			return false;
		}

		const auto retval = vigem_connect(client);
		if (!VIGEM_SUCCESS(retval))
		{
			log("Could not conntect to ViGEm");
			return false;
		}

		// Create a controller

		pad = vigem_target_x360_alloc();
		const auto pir = vigem_target_add(client, pad);

		if (!VIGEM_SUCCESS(pir))
		{
			log("Could not plug in ViGEm gamepad");
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
			log(std::to_string(err));
		}
	}

	void InputService::Disconnect() {
		vigem_disconnect(client);
		vigem_free(client);
	}
}