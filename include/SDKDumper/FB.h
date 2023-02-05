#include <cstdint>
#include <Windows.h>

using namespace std;

enum BasicTypesEnum
{
	kTypeCode_Void = 0x0,
	kTypeCode_DbObject = 0x20,
	kTypeCode_ValueType = 0x40,
	kTypeCode_Class = 0x60,
	kTypeCode_Array = 0x80,
	kTypeCode_FixedArray = 0xA0,
	kTypeCode_String = 0xC0,
	kTypeCode_CString = 0xE0,
	kTypeCode_Enum = 0x100,
	kTypeCode_FileRef = 0x120,
	kTypeCode_Boolean = 0x140,
	kTypeCode_Int8 = 0x160,
	kTypeCode_Uint8 = 0x180,
	kTypeCode_Int16 = 0x1A0,
	kTypeCode_Uint16 = 0x1C0,
	kTypeCode_Int32 = 0x1E0,
	kTypeCode_Uint32 = 0x200,
	kTypeCode_Int64 = 0x220,
	kTypeCode_Uint64 = 0x240,
	kTypeCode_Float32 = 0x260,
	kTypeCode_Float64 = 0x280,
	kTypeCode_Guid = 0x2A0,
	kTypeCode_SHA1 = 0x2C0,
	kTypeCode_ResourceRef = 0x2E0,
	kTypeCode_BasicTypeCount = 0x300,
	kTypeCode_TypeRef = 0x320,
	kTypeCode_BoxedValueRef = 0x340
};


class TestList
{
public:
    void* m_Head; //0x0000
    void* m_Tail; //0x0008
}; //Size = 0x0010


class MemberInfoFlags
{
public:
    uint16_t m_FlagBits; //0x0000 
}; //Size = 0x0002

class ModuleInfo
{
public:
    char* m_ModuleName; //0x0000 char*
    ModuleInfo* m_NextModule; //0x0008 ModuleInfo*
    TestList* m_TestList; //0x0010 TestList*
}; //Size = 0x0018


class MemberInfoData
{
public:
    char* m_Name; //0x0000 char*
    uint16_t m_Flags; //0x0008

    BasicTypesEnum GetNewEntryType()
    {
        int type = m_Flags & 0x03E0;
        BasicTypesEnum Type = BasicTypesEnum::kTypeCode_Void;

        switch (type)
        {
        case 0x0000:
            Type = BasicTypesEnum::kTypeCode_Void;
            break;
        case 0x0020:
            Type = BasicTypesEnum::kTypeCode_DbObject;
            break;
        case 0x0040:
            Type = BasicTypesEnum::kTypeCode_ValueType;
            break;
        case 0x0060:
            Type = BasicTypesEnum::kTypeCode_Class;
            break;
        case 0x0080:
            Type = BasicTypesEnum::kTypeCode_Array;
            break;
        case 0x00A0:
            Type = BasicTypesEnum::kTypeCode_FixedArray;
            break;
        case 0x00C0:
            Type = BasicTypesEnum::kTypeCode_String;
            break;
        case 0x00E0:
            Type = BasicTypesEnum::kTypeCode_CString;
            break;
        case 0x0100:
            Type = BasicTypesEnum::kTypeCode_Enum;
            break;
        case 0x0120:
            Type = BasicTypesEnum::kTypeCode_FileRef;
            break;
        case 0x0140:
            Type = BasicTypesEnum::kTypeCode_Boolean;
            break;
        case 0x0160:
            Type = BasicTypesEnum::kTypeCode_Int8;
            break;
        case 0x0180:
            Type = BasicTypesEnum::kTypeCode_Uint8;
            break;
        case 0x01A0:
            Type = BasicTypesEnum::kTypeCode_Int16;
            break;
        case 0x01C0:
            Type = BasicTypesEnum::kTypeCode_Uint16;
            break;
        case 0x01E0:
            Type = BasicTypesEnum::kTypeCode_Int32;
            break;
        case 0x0200:
            Type = BasicTypesEnum::kTypeCode_Uint32;
            break;
        case 0x0220:
            Type = BasicTypesEnum::kTypeCode_Int64;
            break;
        case 0x0240:
            Type = BasicTypesEnum::kTypeCode_Uint64;
            break;
        case 0x0260:
            Type = BasicTypesEnum::kTypeCode_Float32;
            break;
        case 0x0280:
            Type = BasicTypesEnum::kTypeCode_Float64;
            break;
        case 0x02A0:
            Type = BasicTypesEnum::kTypeCode_Guid;
            break;
        case 0x02C0:
            Type = BasicTypesEnum::kTypeCode_SHA1;
            break;
        case 0x02E0:
            Type = BasicTypesEnum::kTypeCode_ResourceRef;
            break;
        case 0x0300:
            Type = BasicTypesEnum::kTypeCode_BasicTypeCount;
            break;
        case 0x0320:
            Type = BasicTypesEnum::kTypeCode_TypeRef;
            break;
        case 0x0340:
            Type = BasicTypesEnum::kTypeCode_BoxedValueRef;
            break;
        default:
            Type = BasicTypesEnum::kTypeCode_Void;
            break;
        }

        return Type;
    }

}; //Size = 0x000A


class MemberInfo
{
public:
    MemberInfoData* m_InfoData; //0x0000 MemberInfoData*
};


class TypeInfo : public MemberInfo
{
public:
    TypeInfo* m_Next; //0x0008 TypeInfo*
    uint16_t m_RuntimeId; //0x0010
    uint16_t m_Flags; //0x0012
    uint32_t pad_0x0014;
}; //Size = 0x0018


class TypeInfoData : public MemberInfoData
{
public:
    uint16_t m_TotalSize; //0x000A 
    uint32_t pad_0x000C;
    ModuleInfo* m_Module; //0x0010 ModuleInfo*
    TypeInfo* m_pArrayTypeInfo; //0x0018 TypeInfo*
    uint16_t m_Alignment; //0x0020 
    uint16_t m_FieldCount; //0x0022 
    uint32_t pad_0x0024;
}; //Size = 0x0028


class FieldInfoData : public MemberInfoData
{
public:
    uint16_t m_FieldOffset; //0x000A
    uint32_t pad_0x0014;
    TypeInfo* m_FieldTypePtr; //0x0010 TypeInfo*
}; //Size = 0x0018


class FieldInfo : public MemberInfo
{
public:
    TypeInfo* m_DeclaringType; //0x0010 TypeInfo*
}; //Size = 0x0018

class ClassInfo : public TypeInfo
{
public:
    uint64_t pad_0018;
    uint64_t pad_0020;
    uint64_t pad_0028;
    uint64_t pad_0030;
    void* m_Super; //0x0038 ClassInfo*
    void* m_DefaultInstance; //0x0040 void*
    uint16_t m_ClassId; //0x0048
    uint16_t m_LastSubClassId; //0x004A
    uint32_t pad_0x004C;
    uint64_t pad_0050;
    uint64_t pad_0058;
    uint64_t pad_0060;
    uint64_t pad_0068;
    uint64_t pad_0070;
    uint64_t pad_0078;
    uint64_t pad_0080;
    uint64_t m_EntityFLink; // 0X0088 pointer to first entity in list (soldiers, vehicles, grenades etc)
    uint64_t pad_0090;
    uint64_t pad_0098;
    uint64_t pad_00A0;
    uint64_t pad_00A8;// 0X00A8 other pointer to first entity in list
    uint64_t pad_00B0;
};

class ClassInfoData : public TypeInfoData
{
public:
    ClassInfo* m_SuperClass; //0x0028 ClassInfo*
    FieldInfoData* m_Fields; //0x0030 FieldInfoData*
}; //Size = 0x0038

class ArrayTypeInfoData : public TypeInfoData
{
public:
    void* m_ElementType; //0x0028 TypeInfo*
};

class ArrayTypeInfo : public TypeInfo
{
public:
};

class EnumFieldInfoData : public TypeInfoData
{
public:
    void* m_Fields; //0x0028   FieldInfoData*   
};

class EnumFieldInfo : public TypeInfo
{
public:
};

class ValueTypeInfoData : public TypeInfoData
{
public:
    uint64_t pad_0028;
    uint64_t pad_0030;
    uint64_t pad_0038;
    uint64_t pad_0040;
    uint64_t pad_0048;
    void* m_Fields; //0x0050 FieldInfoData*
};

class ValueTypeInfo : public TypeInfo
{
public:
};