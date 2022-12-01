#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

int main()
{
	// Get the DLL file path
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	std::string path = std::string(buffer).substr(0, pos);
	path += "\\BF2VR.dll";
	cout << path << endl;
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
	while (Process32Next(snapshot, &structprocsnapshot))
	{
		// Convert WCHAR* into string
		WCHAR* procexe = structprocsnapshot.szExeFile;
		wstring ws_procexe(procexe);
		std::string s_procexe(ws_procexe.begin(), ws_procexe.end());

		// Check for game process
		if (!strcmp(s_procexe.c_str(), "starwarsbattlefrontii.exe"))
		{
			cout << "Game found with id: " << structprocsnapshot.th32ProcessID << endl;
			pid = structprocsnapshot.th32ProcessID;
			CloseHandle(snapshot);
			break;
		}
	}
	
	// Never found
	if (pid == -1)
	{
		cerr << "Game not open or found" << endl;
		return 1;
	}

	cout << "ID: " << toascii(pid) << endl;

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

	cout << "Dll path allocated at: " << hex << pDllPath << endl;

	VirtualFreeEx(handle, pDllPath, strlen(DllPath) + 1, MEM_RELEASE); // Free the memory allocated for our dll path
	return 0;
}