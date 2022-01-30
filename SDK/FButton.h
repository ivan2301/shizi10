template <int key>
class FButton
{
private:
	uint8_t state;
	uint16_t lastState;

public:
	 void Update() {
		auto keyState = (uint16_t)FC2(win32u, NtUserGetAsyncKeyState, key);
		state = keyState ? (!lastState ? 1 : 2) : 0;
		lastState = keyState;
	}

	 bool Click() {
		return state == 1;
	}

	 bool Press() {
		return state == 2;
	}

	 bool isDown(bool update) {
		if (update)
			Update();
		return state;
	}
};