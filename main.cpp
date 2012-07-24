
#include <gdemu_emulator.h>
#include <X11/Xlib.h>

void setup();
void loop();

// int __stdcall WinMain(void *, void *, void *, int)
int main(int, char* [])
{
   XInitThreads();
	GDEMU::Emulator.run(setup, loop,
		GDEMU::EmulatorEnableKeyboard | GDEMU::EmulatorEnableJ1 //| GDEMU::EmulatorEnableAudio
		);
	return 0;
}
