#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#include <sstream>
#include <shlobj.h>
#include "resource.h"
#pragma comment(lib, "Shlwapi.lib")

using namespace std;


int inject()
{
	// Get the DLL file path
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	std::string path = std::string(buffer).substr(0, pos);
	path += "\\BF2VR.dll";
	if (!PathFileExists(std::wstring(path.begin(), path.end()).c_str()))
	{
		cerr << "DLL not in the directory." << endl;
		return 1;
	}
	LPCSTR DllPath = const_cast<char*>(path.c_str()); // The Path to our DLL

	// Get running procceses
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 structprocsnapshot = { 0 };
	structprocsnapshot.dwSize = sizeof(PROCESSENTRY32);
	if (snapshot == INVALID_HANDLE_VALUE)return 0;
	if (Process32First(snapshot, &structprocsnapshot) == FALSE)return 0;

	// Iterate through processes
	DWORD pid = -1;
	DWORD pid2 = -1;

	while (Process32Next(snapshot, &structprocsnapshot))
	{
		// Convert WCHAR* into string
		WCHAR* procexe = structprocsnapshot.szExeFile;
		wstring ws_procexe(procexe);
		std::string s_procexe(ws_procexe.begin(), ws_procexe.end());

		// Check for game process
		if (!strcmp(s_procexe.c_str(), "vrserver.exe"))
		{
			pid2 = structprocsnapshot.th32ProcessID;
		}
		if (!strcmp(s_procexe.c_str(), "OVRServiceLauncher.exe"))
		{
			pid2 = structprocsnapshot.th32ProcessID;
		}

		// Check for game process
		if (!strcmp(s_procexe.c_str(), "starwarsbattlefrontii.exe"))
		{
			pid = structprocsnapshot.th32ProcessID;
		}
	}

	// Never found
	if (pid2 == -1)
	{
		cerr << "Oculus or SteamVR is not open. Press enter within 5 seconds to continue anyway." << endl;
		Sleep(500);
		bool pressed = false;
		for (int i = 0; i < 250; i++)
		{
			Sleep(1);
			if (GetAsyncKeyState(VK_RETURN)) { // This causes a delay
				pressed = true;
				break;
			}
		}
		if (!pressed)
		{
			return 1;
		}
	}

	// Never found
	if (pid == -1)
	{
		cerr << "Game not open or found." << endl;
		return 1;
	}

	CloseHandle(snapshot);

	// Inject
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); // Opening the Process with All Access
	// Allocate memory for the dllpath in the target process, length of the path string + null terminator
	LPVOID pDllPath = VirtualAllocEx(handle, 0, strlen(DllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
	// Write the path to the address of the memory we just allocated in the target process
	WriteProcessMemory(handle, pDllPath, (LPVOID)DllPath, strlen(DllPath) + 1, 0);
	// Create a Remote Thread in the target process which calls LoadLibraryA as our dllpath as an argument -> program loads our dll
	HANDLE hLoadThread = CreateRemoteThread(handle, 0, 0,
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "LoadLibraryA"), pDllPath, 0, 0);
	WaitForSingleObject(hLoadThread, INFINITE); // Wait for the execution of our loader thread to finish

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
	cout << "Dll path allocated at: " << hex << pDllPath << endl;

	Sleep(1000);

	VirtualFreeEx(handle, pDllPath, strlen(DllPath) + 1, MEM_RELEASE); // Free the memory allocated for our dll path
	return 0;
}


void reinstall()
{
	cout << "Reinstalling. You will need to find the folder where Battlefront is installed. Press enter to continue." << endl;

	for (;;)
	{
		if (GetAsyncKeyState(VK_RETURN)) {
			break;
		}
	}

	TCHAR szTitle[MAX_PATH];
	BROWSEINFO bi = { 0 };
	bi.lpszTitle = L"Select a folder containing starwarsbattlefrontii.exe";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	bi.lpfn = NULL;
	bi.lParam = 0;
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL)
	{
		if (SHGetPathFromIDList(pidl, szTitle))
		{
			char szString[MAX_PATH];
			size_t nNumCharConverted;
			wcstombs_s(&nNumCharConverted, szString, MAX_PATH,
				szTitle, MAX_PATH);

			// Copy openxrruntime

			// Get the DLL file path
			char buffer[MAX_PATH];
			GetModuleFileNameA(NULL, buffer, MAX_PATH);
			std::string::size_type pos = std::string(buffer).find_last_of("\\/");
			std::string path = std::string(buffer).substr(0, pos);
			path += "\\openxr_loader.dll";
			if (!PathFileExists(std::wstring(path.begin(), path.end()).c_str()))
			{
				Sleep(500);
				cerr << "openxr_loader.dll is not in the directory. Press enter to close" << endl;
				for (;;)
				{
					if (GetAsyncKeyState(VK_RETURN)) {
						return;
					}
				}
			}

			// Get the new file path
			std::string newpath = std::string(szString);
			newpath += "\\openxr_loader.dll";

			bool result = CopyFile(std::wstring(path.begin(), path.end()).c_str(), std::wstring(newpath.begin(), newpath.end()).c_str(), FALSE);
			if (!result)
			{
				Sleep(500);
				cerr << "openxr_loader.dll copy failed. Press enter to close" << endl;
				for (;;)
				{
					if (GetAsyncKeyState(VK_RETURN)) {
						return;
					}
				}
			}

			// Rename the config
			string newpath2 = std::string(szString);
			string newpath3 = newpath2;
			newpath2 += "\\config.txt";
			newpath3 += "\\config.txt.bak";

			result = MoveFile(std::wstring(newpath2.begin(), newpath2.end()).c_str(), std::wstring(newpath3.begin(), newpath3.end()).c_str());

			cout << "Installed. Press enter to launch." << endl;
			Sleep(500);
			for (;;)
			{
				if (GetAsyncKeyState(VK_RETURN)) {
					inject();
					return;
				}
			}
		}
		else {
			cout << "You did not select a window. Press enter to retry or ESC to close." << endl;
			for (;;)
			{
				if (GetAsyncKeyState(VK_RETURN)) {
					reinstall();
					return;
				}
				if (GetAsyncKeyState(VK_ESCAPE)) {
					return;
				}
			}
			return;
		}
	}


}


int main(const char* argv[]) {
	// Print fancy thing


	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole,
		FOREGROUND_BLUE | FOREGROUND_INTENSITY);

	cout << R""""(
$$$$$$$\  $$$$$$$$\  $$$$$$\  $$\    $$\ $$$$$$$\  
$$  __$$\ $$  _____|$$  __$$\ $$ |   $$ |$$  __$$\ 
$$ |  $$ |$$ |      \__/  $$ |$$ |   $$ |$$ |  $$ |
$$$$$$$\ |$$$$$\     $$$$$$  |\$$\  $$  |$$$$$$$  |
$$  __$$\ $$  __|   $$  ____/  \$$\$$  / $$  __$$< 
$$ |  $$ |$$ |      $$ |        \$$$  /  $$ |  $$ |
$$$$$$$  |$$ |      $$$$$$$$\    \$  /   $$ |  $$ |
\_______/ \__|      \________|    \_/    \__|  \__|
)"""";

	SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
	cout << "Press enter to launch or F10 to install/reinstall" << endl;

	for (;;)
	{
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);

		if (GetAsyncKeyState(VK_F10)) {
			SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
			Sleep(300);
			reinstall();
			break;
		}

		if (GetAsyncKeyState(VK_RETURN)) {
			Sleep(300);
			if (inject() == 0)
			{
				cout << "Injected! If you need to reinject, press F1 then Enter within 10 seconds of injection" << endl;
				Sleep(300);
				bool pressed = false;
				for (int i = 0; i < 500; i++)
				{
					Sleep(1);
					if (GetAsyncKeyState(VK_F1)) { // This delays for some reason
						pressed = true;
						break;
					}
				}
				if (!pressed)
				{
					return 0;
				}
			}
			else {
				SetConsoleTextAttribute(hConsole, FOREGROUND_INTENSITY);
				cout << "Press enter to try again or escape to close" << endl;
			}
		}

		if (GetAsyncKeyState(VK_ESCAPE)) {
			break;
		}
	}
	return 0;
}