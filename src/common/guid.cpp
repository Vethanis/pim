#include "common/guid.h"

struct HexToNibble
{
    char values[128];

    static constexpr HexToNibble Create()
    {
        HexToNibble table = {};
        table.values['0'] = 0;
        table.values['1'] = 1;
        table.values['2'] = 2;
        table.values['3'] = 3;
        table.values['4'] = 4;
        table.values['5'] = 5;
        table.values['6'] = 6;
        table.values['7'] = 7;
        table.values['8'] = 8;
        table.values['9'] = 9;
        table.values['A'] = 10;
        table.values['B'] = 11;
        table.values['C'] = 12;
        table.values['D'] = 13;
        table.values['E'] = 14;
        table.values['F'] = 15;
        return table;
    }
};

struct NibbleToHex
{
    char values[16];

    static constexpr NibbleToHex Create()
    {
        NibbleToHex table =
        {
            '0',
            '1',
            '2',
            '3',
            '4',
            '5',
            '6',
            '7',
            '8',
            '9',
            'A',
            'B',
            'C',
            'D',
            'E',
            'F'
        };
        return table;
    }
};

static constexpr NibbleToHex N2H = NibbleToHex::Create();
static constexpr HexToNibble H2N = HexToNibble::Create();

Guid ToGuid(GuidStr str)
{
    cstr src = str.value;
    ASSERT(src[0] == '0');
    ASSERT(src[1] == 'x');
    src += 2;

    Guid guid = {};

    u8* dst = (u8*)&guid;
    for (i32 i = 0; i < 16; ++i)
    {
        u8 lo = *src++;
        u8 hi = *src++;

        lo = H2N.values[lo];
        hi = N2H.values[hi];

        dst[i] = (hi << 4) | lo;
    }

    return guid;
}

GuidStr ToString(Guid guid)
{
    GuidStr guidStr = {};
    u8* dst = (u8*)&guidStr;
    const u8* src = (const u8*)&guid;
    *dst++ = '0';
    *dst++ = 'x';
    for (i32 i = 0; i < 16; ++i)
    {
        u8 c = src[i];
        u8 lo = 0xf & c;
        u8 hi = 0xf & (c >> 4);
        lo = N2H.values[lo];
        hi = N2H.values[hi];
        *dst++ = lo;
        *dst++ = hi;
    }
    *dst++ = 0;
    return guidStr;
}