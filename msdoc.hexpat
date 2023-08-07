#include <std/ptr.pat>

enum valueType: u16 {
  VT_EMPTY = 0x0000,
  VT_NULL = 0x0001,
  VT_I2 = 0x0002,
  VT_I4 = 0x0003,
  VT_R4 = 0x0004,
  VT_R8 = 0x0005,
  VT_CY = 0x0006,
  VT_DATE = 0x0007,
  VT_BSTR = 0x0008,
  VT_ERROR = 0x000A,
  VT_BOOL = 0x000B,
  VT_DECIMAL = 0x000E,
  VT_I1 = 0x0010,
  VT_UI1 = 0x0011,
  VT_UI2 = 0x0012,
  VT_UI4 = 0x0013,
  VT_I8 = 0x0014,
  VT_UI8 = 0x0015,
  VT_INT = 0x0016,
  VT_UINT = 0x0017,
  VT_LPSTR = 0x001E,
  VT_LPWSTR = 0x001F,
  VT_FILETIME = 0x0040,
  VT_BLOB = 0x0041,
  VT_STREAM = 0x0042,
  VT_STORAGE = 0x0043,
};

struct typedPropertyValue {
    valueType type;
    u16 Padding;
    if (type == valueType::VT_LPSTR) {
        u32 size;
        char string[size];
    }
};

struct propertyIdentifierAndOffset {
    u32 propertyIdentifier;
    u32 offset;
    typedPropertyValue @ 0x2e00 + parent.parent.offset0 + offset;
};

struct propertySet {
    u32 size;
    u32 numProperties;
    propertyIdentifierAndOffset pi[numProperties];
};

struct propertySetStream {
    u16 byteorder;
    u16 version;
    u32 systemIdentifier;
    u128 CLSID;
    u32 numPropertySets;
    u128 FTMID0;
    u32 offset0;
    if (numPropertySets == 2) {
        u128 FTMID1;
        u32 offset2;
    }
    propertySet ps;
};

propertySetStream p @ 0x2e00;