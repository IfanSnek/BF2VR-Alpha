#pragma once
#include <string>
#include <vector>
#include "HookHelper.h"
#include "Types.h"
#include <fstream>

namespace BF2VR {

	inline HMODULE OwnModule;
	inline HWND OwnWindow;
	

	static std::ofstream Log("logs.txt", std::ios_base::app);

	void log(std::string message);
	void error(std::string message);

	static inline int warnCount = 0;
	static inline std::string lastWarn = "";
	void warn(std::string message);

	void success(std::string message);
	void info(std::string message);
	void deb(std::string message);
	
	bool IsValidPtr(PVOID p);

	void Shutdown();

	void ShutdownNoHooks();

	void LoadConfig();

	void SaveConfig();

	Vec3 EulerFromQuat(Vec4 q);

    // https://github.com/onra2/swbf2-internal/blob/master/swbf2%20onra2/Classes.h

	struct typeInfoMemberResult {
		void* pVTable;
		const char* name;
		DWORD offset;
	};

	inline std::vector<typeInfoMemberResult> typeInfoMemberResults;

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

        const byte INSTR_LEA = 0x48;
        const byte INSTR_RET = 0xc3;
        const byte INSTR_JMP = 0xe9;
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

			if (*(byte*)pGetType == INSTR_JMP || *(byte*)pGetType == INSTR_LEA) {
				void* pTypeInfo = nullptr;
				if (*(byte*)pGetType == INSTR_JMP) {
					//std::cout << std::hex << "rel:\t" << *(int32_t*)((DWORD64)pGetType + 1) << "\tRIP:\t" << (DWORD64)pGetType + 5 << std::endl;
					pGetType = (void*)(*(int32_t*)((DWORD64)pGetType + 1) + (DWORD64)pGetType + 5);
				}
				if (*(byte*)pGetType == INSTR_LEA) {
					if (*(byte*)((DWORD64)pGetType + 7) != INSTR_RET) continue;
					pTypeInfo = (void*)(*(int32_t*)((DWORD64)pGetType + 3) + (DWORD64)pGetType + 7);
				}
				else continue;
				if (!IsValidPtr(pTypeInfo)) continue;
				void* pMemberInfo = *(void**)pTypeInfo;
				if (!IsValidPtr(pMemberInfo)) continue;
				char* m_name = *(char**)pMemberInfo;
				if (!IsValidPtr(m_name)) continue;
				if ((DWORD64)pTypeInfo > BASE_ADDRESS && (DWORD64)pTypeInfo < MAX_ADDRESS) {
					if (strcmp(m_name, name) == 0) {
						typeInfoMemberResult result;
						result.name = name;
						result.offset = offset;
						result.pVTable = addr;
						typeInfoMemberResults.push_back(result);
                        success("Found it.");
                        return *(T*)((DWORD64)addr + offset);
                    }

                    lastOffset = offset;
                }
            }
        }
        return nullptr;
    }

	// Config settings

	inline float RATIO = 1;
	inline float FOV = 90.0f;
	inline bool Reconfig = false;
	inline bool NOFOV = false;
	inline bool HEADAIM = false;

	const inline float pi = 3.14159265358979323846f;
}