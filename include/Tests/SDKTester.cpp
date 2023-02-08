#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <format>
#include <sstream>   

#include "../BF2VR/SDK.h"
#include "../BF2VR/BFSDK.h"

#include "../../third-party/minhook/MinHook.h"

using namespace std;

bool IsValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr && !IsBadReadPtr(p, sizeof(PVOID)); }

///////////////////////////
/// This file is a WIP
///////////////////////////

struct typeInfoMemberResult {
	void* pVTable;
	const char* name;
	DWORD offset;
};

std::vector<typeInfoMemberResult> typeInfoMemberResults;

template <class T = void*>
T GetClassFromName(void* addr, const char* name, SIZE_T classSize = 0x2000, bool rescan = false) {
	if (!rescan) {
		for (typeInfoMemberResult& result : typeInfoMemberResults) {
			if (result.pVTable == addr) {
				if (result.name == name) {
					return *(T*)((DWORD64)addr + result.offset);
				}
			}
		}
	}

	const BYTE INSTR_LEA = 0x48;
	const BYTE INSTR_RET = 0xc3;
	const BYTE INSTR_JMP = 0xe9;
	const DWORD64 BASE_ADDRESS = 0x140000000;
	const DWORD64 MAX_ADDRESS = 0x14fffffff;

	DWORD offset = 0;
	DWORD lastOffset = 0;
	while (offset < classSize) {
		offset += 8;
		if (!IsValidPtr((void*)((DWORD64)addr + offset))) continue;
		void* czech = *(void**)((DWORD64)addr + offset);
		if (!IsValidPtr(czech)) continue;
		if (!IsValidPtr(*(void**)czech)) continue; // vtable
		if (!IsValidPtr(**(void***)czech)) continue; // virtual 1;
		void* pGetType = **(DWORD64***)czech;
		if ((DWORD64)pGetType < BASE_ADDRESS || (DWORD64)pGetType > MAX_ADDRESS) continue;

		if (*(BYTE*)pGetType == INSTR_JMP || *(BYTE*)pGetType == INSTR_LEA) {
			void* pTypeInfo = nullptr;
			if (*(BYTE*)pGetType == INSTR_JMP) {
				pGetType = (void*)(*(int32_t*)((DWORD64)pGetType + 1) + (DWORD64)pGetType + 5);
			}
			if (*(BYTE*)pGetType == INSTR_LEA) {
				if (*(BYTE*)((DWORD64)pGetType + 7) != INSTR_RET) continue;
				pTypeInfo = (void*)(*(int32_t*)((DWORD64)pGetType + 3) + (DWORD64)pGetType + 7);
			}
			else continue;
			if (!IsValidPtr(pTypeInfo)) continue;
			void* pMemberInfo = *(void**)pTypeInfo;
			if (!IsValidPtr(pMemberInfo)) continue;
			char* m_name = *(char**)pMemberInfo;
			if (!IsValidPtr(m_name)) continue;
			if ((DWORD64)pTypeInfo > BASE_ADDRESS && (DWORD64)pTypeInfo < MAX_ADDRESS) {
				cout << m_name << endl;
				cout << format("{:#8x}", (DWORD64)offset) << endl;
				cout << endl;
				if (strcmp(m_name, name) == 0) {
					typeInfoMemberResult result;
					result.name = name;
					result.offset = offset;
					result.pVTable = addr;
					typeInfoMemberResults.push_back(result);
					return *(T*)((DWORD64)addr + offset);
				}

				lastOffset = offset;
			}
		}
	}
	return nullptr;
}


static std::ofstream sdk("SDK.txt", std::ios_base::app);

template <class T = void*>
T RecursiveFind(void* addr, const char* name, int level, const char* parentName) {

	const SIZE_T classSize = 0x2000;
	const BYTE INSTR_LEA = 0x48;
	const BYTE INSTR_RET = 0xc3;
	const BYTE INSTR_JMP = 0xe9;
	const DWORD64 BASE_ADDRESS = 0x140000000;
	const DWORD64 MAX_ADDRESS = 0x14fffffff;

	DWORD offset = 0;
	DWORD lastOffset = 0;
	int findIndex = 0;
	while (offset < classSize) {
		offset += 8;
		if (!IsValidPtr((void*)((DWORD64)addr + offset))) continue;
		void* czech = *(void**)((DWORD64)addr + offset);
		if (!IsValidPtr(czech)) continue;
		if (!IsValidPtr(*(void**)czech)) continue; // vtable
		if (!IsValidPtr(**(void***)czech)) continue; // virtual 1;
		void* pGetType = **(DWORD64***)czech;
		if ((DWORD64)pGetType < BASE_ADDRESS || (DWORD64)pGetType > MAX_ADDRESS) continue;

		if (*(BYTE*)pGetType == INSTR_JMP || *(BYTE*)pGetType == INSTR_LEA) {
			void* pTypeInfo = nullptr;
			if (*(BYTE*)pGetType == INSTR_JMP) {
				pGetType = (void*)(*(int32_t*)((DWORD64)pGetType + 1) + (DWORD64)pGetType + 5);
			}
			if (*(BYTE*)pGetType == INSTR_LEA) {
				if (*(BYTE*)((DWORD64)pGetType + 7) != INSTR_RET) continue;
				pTypeInfo = (void*)(*(int32_t*)((DWORD64)pGetType + 3) + (DWORD64)pGetType + 7);
			}
			else continue;
			if (!IsValidPtr(pTypeInfo)) continue;
			void* pMemberInfo = *(void**)pTypeInfo;
			if (!IsValidPtr(pMemberInfo)) continue;
			char* m_name = *(char**)pMemberInfo;
			if (!IsValidPtr(m_name)) continue;
			if ((DWORD64)pTypeInfo > BASE_ADDRESS && (DWORD64)pTypeInfo < MAX_ADDRESS) {

				if (strcmp(m_name, parentName) == 0 || strcmp(m_name, "EntityBus") == 0 || strcmp(m_name, "ShaderBlockDepotResource") == 0 || strcmp(m_name, "Dx11RenderBuffer") == 0 || strcmp(m_name, "MeshSet") == 0)
				{
					continue;
				}

				string n = m_name;
				if (n.substr(n.length() - 6, n.length()) == "Entity")
				{
					continue;
				}

				for (int i = 0; i <= level; i++)
				{
					cout << " ";
					sdk << " ";
				}

				cout << m_name << endl;
				sdk << m_name << endl;


				for (int i = 0; i <= level; i++)
				{
					cout << " ";
					sdk << " ";
				}

				cout << format("{:#8x}", (DWORD64)offset) << endl << endl;
				sdk << format("{:#8x}", (DWORD64)offset) << endl << endl;

				T result = *(T*)((DWORD64)addr + offset);

				if (strcmp(m_name, name) == 0 ) {

					return result;
				}
				else {
					if (GetAsyncKeyState(VK_RETURN))
					{
						break;
					}
					
					if (level < 10)
					{

						T output = RecursiveFind<T>(result, name, level + 1, m_name);

						if (output != nullptr)
						{
							return output;
						}
					}
				}

				lastOffset = offset;
			}
		}
	}
	return nullptr;
}

PVOID(*origPlayerPoses)() = nullptr;

PVOID updatePlayerPoses()
{
	sdk << "hi" << endl;
	return nullptr;
}

DWORD __stdcall mainThread(HMODULE module)
{

	AllocConsole();
	freopen("CONOUT$", "w", stdout);

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

	Sleep(200);

	DWORD64 addr = 0x14625454F;

	
	MH_Initialize();
	//cout << MH_CreateHook((LPVOID)addr, &updatePlayerPoses, reinterpret_cast<LPVOID*>(&origPlayerPoses)) << endl;



	auto* base = GameContext::GetInstance()->playerManager->LocalPlayer->controlledControllable;

	fbClientSoldierWeaponsComponent* find = GetClassFromName<fbClientSoldierWeaponsComponent*>((void*)base, "ClientBoneCollisionComponent");

	
	// Created with ReClass.NET 1.2 by KN4CK3R


	class QuatTransform
	{
	public:
		Vec4 Scale; //0x0000
		Vec4 Quat; //0x0010
		Vec4 Translation; //0x0020
	}; //Size: 0x0030

	class quatTransformArray
	{
	public:
		class QuatTransform transform[98]; //0x0000
	}; //Size: 0x1260

	class UpdatePoseResultData
	{
	public:
		class quatTransformArray* N00000FC3; //0x0000
	}; //Size: 0x0008

	class ClientBoneCollisionComponent
	{
	public:
		char pad_0000[72]; //0x0000
		class AnimationSkeleton* skeleton; //0x0048
		class UpdatePoseResultData pose; //0x0050
		char pad_0058[8]; //0x0058
	}; //Size: 0x0060

	class AnimationSkeleton
	{
	public:
		char pad_0000[8]; //0x0000
		class SkeletonAsset* asset; //0x0008
		int32_t boneCount; //0x0010
	}; //Size: 0x0014

	class SkeletonAsset
	{
	public:
		char pad_0000[144]; //0x0000
		char* boneNames[98]; //0x0090
	}; //Size: 0x03A0

	ClientBoneCollisionComponent* bones = (ClientBoneCollisionComponent*)find;


	cout << format("{:#8x}", (DWORD64)find) << endl;

	for (;;)

	{
		bones->pose.N00000FC3->transform[49].Translation.y = 1;
		if (GetAsyncKeyState(VK_F2))
		{
			MH_RemoveHook(MH_ALL_HOOKS);
			MH_Uninitialize();
			break;
		}
	}

	cout << "Done" << endl;

	FreeConsole();
	FreeLibraryAndExitThread(module, 0);
	
	return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)mainThread, hModule, NULL, NULL);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}