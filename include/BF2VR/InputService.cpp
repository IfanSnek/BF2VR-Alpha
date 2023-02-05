#include "Utils.h"
#include "InputService.h"

#include <Xinput.h>

namespace BF2VR {
	bool InputService::connect() {

		pVigemClient = vigem_alloc();

		if (pVigemClient == nullptr)
		{
			error("Could not allocate memory for ViGEm.");
			return false;
		}

		const auto retval = vigem_connect(pVigemClient);
		if (!VIGEM_SUCCESS(retval))
		{
			error("Could not conntect to ViGEm. Error: " + std::to_string(retval));
			return false;
		}

		// Create a controller

		pVigemGamepad = vigem_target_x360_alloc();
		const auto pir = vigem_target_add(pVigemClient, pVigemGamepad);

		if (!VIGEM_SUCCESS(pir))
		{
			error("Could not plug in ViGEm gamepad. Error: " + std::to_string(pir));
			return false;
		}

		return true;
	}

	void InputService::update() {

		XINPUT_GAMEPAD gamepad{};
		if (useRight)
		{
			gamepad.sThumbRX = thumbLX;
			gamepad.sThumbRY = thumbLY;
		}
		else
		{
			gamepad.sThumbLX = thumbLX;
			gamepad.sThumbLY = thumbLY;
		}

		gamepad.bRightTrigger = rightTrigger;
		gamepad.bLeftTrigger = leftTrigger;
		gamepad.wButtons = buttons;

		VIGEM_ERROR err = vigem_target_x360_update(pVigemClient, pVigemGamepad, *reinterpret_cast<XUSB_REPORT*>(&gamepad));
		if (!VIGEM_SUCCESS(err))
		{
			warn("Could not update the virtual gamepad. Error: " + std::to_string(err));
		}
	}

	void InputService::disconnect() {
		vigem_disconnect(pVigemClient);
		vigem_free(pVigemClient);
	}
}