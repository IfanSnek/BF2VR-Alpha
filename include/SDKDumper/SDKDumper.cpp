#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <format>

#include "FB.h"

using namespace std;

bool isValidPtr(PVOID p) { return (p >= (PVOID)0x10000) && (p < ((PVOID)0x000F000000000000)) && p != nullptr && !IsBadReadPtr(p, sizeof(PVOID)); }

std::ofstream SDK("SDK.txt", std::ios_base::app);

///////////////////////////
/// This file is a WIP
///////////////////////////


string typeFromInt(int type)
{
	string Type = "void";

	switch (type)
	{
	case 0x0000:
		Type = "void";
		break;
	case 0x0040:
		Type = "value";
		break;
	case 0x0060:
		Type = "class";
		break;
	case 0x0080:
		Type = "array";
		break;
	case 0x00A0:
		Type = "fixedarray";
		break;
	case 0x00C0:
		Type = "string";
		break;
	case 0x00E0:
		Type = "const char*";
		break;
	case 0x0100:
		Type = "enum";
		break;
	case 0x0140:
		Type = "bool";
		break;
	case 0x0160:
		Type = "INT8";
		break;
	case 0x0180:
		Type = "UINT8";
		break;
	case 0x01A0:
		Type = "INT16";
		break;
	case 0x01C0:
		Type = "UINT16";
		break;
	case 0x01E0:
		Type = "INT32";
		break;
	case 0x0200:
		Type = "UINT32";
		break;
	case 0x0220:
		Type = "INT64";
		break;
	case 0x0240:
		Type = "UINT64";
		break;
	case 0x0260:
		Type = "float";
		break;
	case 0x0280:
		Type = "float";
		break;
	default:
		Type = "void";
		break;
	}
	return Type;
}

string GetCppType(BasicTypesEnum Type, string Name)
{
	string cppType = "";

	switch (Type)
	{
	case BasicTypesEnum::kTypeCode_Void:
		cppType = "void";
		break;
	case BasicTypesEnum::kTypeCode_DbObject:
		cppType = "DbObject";
		break;
	case BasicTypesEnum::kTypeCode_ValueType:
		cppType = Name;
		break;
	case BasicTypesEnum::kTypeCode_Class:
	{
		cppType = Name;
	}
	break;
	case BasicTypesEnum::kTypeCode_Array:
	{
		cppType = Name;
	}
	break;
	case BasicTypesEnum::kTypeCode_FixedArray:
		cppType = "FixedArray";
		break;
	case BasicTypesEnum::kTypeCode_String:
		cppType = "string";
		break;
	case BasicTypesEnum::kTypeCode_CString:
		cppType = "char*";
		break;
	case BasicTypesEnum::kTypeCode_Enum:
		cppType = Name;
		break;
	case BasicTypesEnum::kTypeCode_FileRef:
		cppType = "FileRef";
		break;
	case BasicTypesEnum::kTypeCode_Boolean:
		cppType = "bool";
		break;
	case BasicTypesEnum::kTypeCode_Int8:
		cppType = "int8_t";
		break;
	case BasicTypesEnum::kTypeCode_Uint8:
		cppType = "uint8_t";
		break;
	case BasicTypesEnum::kTypeCode_Int16:
		cppType = "int16_t";
		break;
	case BasicTypesEnum::kTypeCode_Uint16:
		cppType = "uint16_t";
		break;
	case BasicTypesEnum::kTypeCode_Int32:
		cppType = "int32_t";
		break;
	case BasicTypesEnum::kTypeCode_Uint32:
		cppType = "uint32_t";
		break;
	case BasicTypesEnum::kTypeCode_Int64:
		cppType = "int64_t";
		break;
	case BasicTypesEnum::kTypeCode_Uint64:
		cppType = "int64_t";
		break;
	case BasicTypesEnum::kTypeCode_Float32:
		cppType = "float";
		break;
	case BasicTypesEnum::kTypeCode_Float64:
		cppType = "double";
		break;
	case BasicTypesEnum::kTypeCode_Guid:
		cppType = "Guid";
		break;
	case BasicTypesEnum::kTypeCode_SHA1:
		cppType = "SHA1";
		break;
	case BasicTypesEnum::kTypeCode_ResourceRef:
		cppType = "ResourceRef";
		break;
	case BasicTypesEnum::kTypeCode_BasicTypeCount:
		cppType = "BasicTypeCount";
		break;
	default:
		cppType = "unknown_type";
		break;
	}

	return cppType;
}

string FixTypeName(string name)
{
	if(name=="Int8")
		name = "int8_t";
	if(name=="Uint8")
		name = "uint8_t";
	if(name=="Int16")
		name = "int16_t";
	if(name=="Uint16")
		name = "uint16_t";
	if(name=="Int32")
		name = "int32_t";
	if(name=="Uint32")
		name = "uint32_t";
	if(name=="Int64")
		name = "int64_t";
	if(name=="Uint64")
		name = "uint64_t";
	if(name=="Float32")
		name = "float";
	if(name=="Float64")
		name = "double";
	if(name=="Boolean")
		name = "int16_t";
	if(name=="CString")
		name = "char*";
	
	return name;
}

struct typeInfoMemberResult {
	string dataType;
	string memberName;
	int arraysize;
	string hexLocation;
};

std::vector<typeInfoMemberResult> typeInfoMemberResults;

bool GetClassFromAddress(void* addr) {

	const BYTE INSTR_LEA = 0x48;
	const BYTE INSTR_RET = 0xc3;
	const BYTE INSTR_JMP = 0xe9;
	const DWORD64 BASE_ADDRESS = 0x140000000;
	const DWORD64 MAX_ADDRESS = 0x14fffffff;
	const DWORD64 MAX_SIZE = 0x2000;

	DWORD offset = 0;
	while (offset < MAX_SIZE) {
		offset += 8;
		if (!isValidPtr((void*)((DWORD64)addr + offset))) continue;
		void* czech = *(void**)((DWORD64)addr + offset);
		if (!isValidPtr(czech)) continue;
		if (!isValidPtr(*(void**)czech)) continue; // vtable
		if (!isValidPtr(**(void***)czech)) continue; // virtual 1;
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
			if (!isValidPtr(pTypeInfo)) continue;

			if ((DWORD64)pTypeInfo > BASE_ADDRESS && (DWORD64)pTypeInfo < MAX_ADDRESS) {

				MemberInfoData* pMemberInfoData = *(MemberInfoData**)pTypeInfo;
				if (!isValidPtr(pMemberInfoData)) continue;

				char* m_Name = pMemberInfoData->m_Name;
				if (!isValidPtr(m_Name)) continue;

				BasicTypesEnum dataType = pMemberInfoData->GetNewEntryType();

				string name = m_Name;

				if (name.find("(") != string::npos)
				{
					// some functions apparently defined, skip for now investigate later
					dataType = BasicTypesEnum::kTypeCode_Void;
				}
				if ((name.find("Float32") != string::npos) || (name.find("char") != string::npos))
				{
					// uneeded class
					dataType = BasicTypesEnum::kTypeCode_Void;
				}

				switch (dataType)
				{
				case BasicTypesEnum::kTypeCode_Enum:
				{
					cout << to_string((DWORD64)pTypeInfo) << endl;
					SDK << to_string((DWORD64)pTypeInfo) << endl;
					cout << "sup" << endl;

					EnumFieldInfoData* typeInfoData = *(EnumFieldInfoData**)pTypeInfo;
					if (!isValidPtr(typeInfoData))break;

					cout << "enum " << name << endl;
					cout << "{" << endl;

					SDK << "enum " << name << endl;
					SDK << "{" << endl;

					int FieldCount = typeInfoData->m_FieldCount;
					if (FieldCount > 0)
					{
						for (int i = 0; i < FieldCount; i++)
						{
							FieldInfoData* fieldInfoData = *((FieldInfoData**)((&typeInfoData->m_Fields) + (i * 0x18)));

							cout << "    " << fieldInfoData->m_Name << ", //" << format("0x{:#04x}", i) << endl;
							SDK << "    " << fieldInfoData->m_Name << ", //" << format("0x{:#04x}", i) << endl;
						}
					}
					cout << "};" << endl;
					cout << endl;

					SDK << "};" << endl;
					SDK << endl;

				}
				break;

				case BasicTypesEnum::kTypeCode_Class:
				{
					if (name.find("::") != string::npos)
					{
						break;
					}

					ClassInfoData* typeInfoData = *(ClassInfoData**)pTypeInfo;
					if (!isValidPtr(typeInfoData))break;

					cout << "class " << typeInfoData->m_Name;
					SDK << "class " << typeInfoData->m_Name;
					
					ClassInfo* parent = typeInfoData->m_SuperClass;
					if (isValidPtr(parent))
					{
						MemberInfoData* parentInfoData = parent->m_InfoData;
						if (isValidPtr(parentInfoData))
						{
							if (strcmp(parentInfoData->m_Name, "") != 0)
							{
								cout << " : public " << parentInfoData->m_Name;
							}
						}
					}

					cout << endl;
					cout << "{" << endl;

					SDK << endl;
					SDK << "{" << endl;

					int FieldCount = typeInfoData->m_FieldCount;
					if (FieldCount > 0)
					{
						for (int i = 0; i < FieldCount; i++)
						{
							FieldInfoData* fieldInfoData = *((FieldInfoData**)( (&typeInfoData->m_Fields) + (i * 0x18)));

							if (!isValidPtr(fieldInfoData))
							{
								continue;
							}

							char* namePtr = fieldInfoData->m_Name;
							if (!isValidPtr(namePtr))
							{
								continue;
							}

							string name = namePtr;

							if (find_if(name.begin(), name.end(), [](char c) { return !(isalnum(c)); }) != name.end() || name.size() == 0)
							{
								continue;
							}

							BasicTypesEnum fieldTypeEnum = fieldInfoData->GetNewEntryType();


							string fieldType;

							// fix the odd field type with flags as 0x0000 or 0x2000
							if ((fieldInfoData->m_Flags == 0) || (fieldInfoData->m_Flags == 0x2000))
							{
								fieldType = FixTypeName(name);
							}
							else
							{
								fieldType = GetCppType(fieldTypeEnum, name);
							}

							if (fieldType.find("-Array") != string::npos)
							{
								fieldType = fieldType.substr(0, fieldType.length() - 6);
							}

							fieldType = FixTypeName(fieldType);


							cout << "    " << fieldType;
							SDK << "    " << fieldType;

							// Check for pointer
							if (fieldTypeEnum == kTypeCode_Class && fieldType != "float")
							{
								cout << "*";
								SDK << "*";
							}

							cout << " " << name;
							SDK << " " << name;

							// Check for array

							if (fieldTypeEnum == kTypeCode_Array)
							{
								cout << "[] ";
								SDK << "[] ";
							}

							cout << ";  //" << format("0x{:#04x}", i) << endl;
							SDK << ";  //" << format("0x{:#04x}", i) << endl;

						}
					}

					cout << "}; //" << format("0x{:#04x}", typeInfoData->m_TotalSize) << endl;
					cout << endl;

					SDK << "}; //" << format("0x{:#04x}", typeInfoData->m_TotalSize) << endl;
					SDK << endl;
				}
				break;

				default:
					break;
				}

				return true;
			}
		}
	}
	return false;
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
	cout << "Dumping SDK" << endl;


	const DWORD64 BASE_ADDRESS = 0x140000000;
	const DWORD64 MAX_ADDRESS = 0x14E45B5EC;

	for (DWORD64 addr = BASE_ADDRESS; addr < MAX_ADDRESS; addr++)
	{
		void* ptr = *((void**)addr);
		if (isValidPtr(ptr))
		{
			GetClassFromAddress(ptr);
		}

		if (addr % 1000 == 0)
		{
			if (GetAsyncKeyState(VK_F1))
			{
				break;
			}
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