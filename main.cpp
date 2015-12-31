#include <string>
#include <windows.h>

HHOOK	kbdhook;	/* Keyboard hook handle */
bool	running;	/* Used in main loop */
bool shift = false;

unsigned int print;
std::string specialChars = "ÈË‡˘Á";
std::string capsCharsAssociated = "…»¿Ÿ«";

void downKey(int keyCode) {
    INPUT ip;

    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = keyCode; // virtual-key code
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));
}
void upKey(int keyCode) {
    // This structure will be used to create the keyboard
    // input event.
    INPUT ip;

    // Set up a generic keyboard event.
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    ip.ki.wVk = keyCode; // virtual-key code
    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
    SendInput(1, &ip, sizeof(INPUT));
}
void pressKey(int keyCode) {
    downKey(keyCode);
    upKey(keyCode);
}

void writeLetter(unsigned char letter) {
	pressKey(VK_BACK);

	INPUT inp[2];
	memset(inp,0,sizeof(INPUT)); 
	inp[0].type = INPUT_KEYBOARD; 
	inp[0].ki.dwFlags = KEYEVENTF_UNICODE; // to avoid shift, and so on 
	inp[1] = inp[0]; 
	inp[1].ki.dwFlags |= KEYEVENTF_KEYUP; 

	inp[0].ki.wScan = inp[1].ki.wScan = letter; 
	SendInput(2, inp, sizeof(INPUT));
}

DWORD WINAPI postWrite(LPVOID lpParameter) {
	writeLetter(capsCharsAssociated[print]);
	return 0;
}

/**
 * \brief Called by Windows automatically every time a key is pressed (regardless
 * of who has focus)
 */
__declspec(dllexport) LRESULT CALLBACK handlekeys(int code, WPARAM wp, LPARAM lp)
{
	bool toPrint = false;
	if (code == HC_ACTION) {
		bool down = (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN);
		bool up = (wp == WM_SYSKEYUP || wp == WM_KEYUP);
		if (down || up) {
			char tmp[0xFF] = {0};
			std::string str;
			DWORD msg = 1;
			KBDLLHOOKSTRUCT st_hook = *((KBDLLHOOKSTRUCT*)lp);

			/*
			 * Get key name as string
			 */
			msg += (st_hook.scanCode << 16);
			msg += (st_hook.flags << 24);
			GetKeyNameText(msg, tmp, 0xFF);
			str = std::string(tmp);
			
			if ((str.length() <= 1) && shift && down) {
				char letter = str[0];
				for (unsigned int i=0;i<specialChars.length();i++) {
					if (letter == specialChars[i]) {
						print = i;
						toPrint = true;
						break;
					}
				}
			}
			else if ((str == "SHIFT") || (str == "MAJ"))
					shift = down;
			else if (shift && (str == "ECHAP"))
				ExitProcess(0);
		}
	}

	LRESULT res = CallNextHookEx(kbdhook, code, wp, lp);
	if (toPrint) {
		int pasteCounter = 0;
		DWORD postID;
		CreateThread(0, 0, postWrite, &pasteCounter, 0, &postID);
	}
	return res;
}


/**
 * \brief Called by DispatchMessage() to handle messages
 * \param hwnd	Window handle
 * \param msg	Message to handle
 * \param wp
 * \param lp
 * \return 0 on success
 */
LRESULT CALLBACK windowprocedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_CLOSE: case WM_DESTROY:
			running = false;
			break;
		default:
			/* Call default message handler */
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE thisinstance, HINSTANCE previnstance,
		LPSTR cmdline, int ncmdshow)
{
	MSG		msg;

	/*
	 * Hook keyboard input so we get it too
	 */
	HINSTANCE modulehandle = GetModuleHandle(NULL);
	kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)handlekeys, modulehandle, NULL);

	running = true;

	/*
	 * Main loop
	 */
	while (running) {
		/*
		 * Get messages, dispatch to window procedure
		 */
		if (!GetMessage(&msg, NULL, 0, 0))
			running = false; /*
					  * This is not a "return" or
					  * "break" so the rest of the loop is
					  * done. This way, we never miss keys
					  * when destroyed but we still exit.
					  */
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}