#include <stdint.h>
#include <stddef.h>
#include <windows.h>
#include <stdio.h>
#include <GL/GL.h>
#define GL_VERSION_1_0
#include "glcorearb.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint32_t b32;
typedef size_t usz;

#define Assert(x) if(!(x)) { *(int*)(0) = 0; }
#define ArrLen(x) sizeof(x)/sizeof(x[0])

typedef struct
{
    usz Sz;
    u8* At;
} gfx_buf;

typedef struct
{
    usz Sz;
    i8* At;
} gfx_str;

typedef struct
{
    u32 Cols;
    u32 Rows;
    u32 Jump;
    u32 Size;
    u8* Data;
} gfx_font;

static void gfxError(const char* Format, ...)
{
    char String[1024];

    va_list VaList;
    va_start(VaList, Format);
    vsnprintf(String, sizeof(String), Format, VaList);
    va_end(VaList);

    OutputDebugStringA(String);
    Assert(0);
}

#define GL_DEBUG_TYPE_ERROR               0x824C

static void gfxGlCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    gfxError( "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

static void gfxFree(void* Data)
{
    VirtualFree(Data, 0, MEM_DECOMMIT|MEM_RELEASE);
}

static void* gfxLoadFile(const char* Name, usz* Size)
{
    void* Result = 0;

    HANDLE Handle = CreateFileA(Name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(Handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER LargeInteger;
        if(GetFileSizeEx(Handle, &LargeInteger))
        {
            if(!LargeInteger.HighPart)
            {
                DWORD Count = LargeInteger.LowPart;
                void* Data = VirtualAlloc(0, Count+1, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                if(Data)
                {
                    DWORD Length = 0;
                    if(ReadFile(Handle, Data, Count, &Length, 0) && Length == Count)
                    {
                        *Size = Count;

                        Result = Data;
                    }
                    else
                    {
                        VirtualFree(Data, 0, MEM_DECOMMIT|MEM_RELEASE);
                    }
                }
            }
        }

        CloseHandle(Handle);
    }

    return Result;
}

static b32 gfxHexToDec(char C, u8* V)
{
    if(C >= 'a' && C <= 'f')
    {
        *V = C - 'a' + 10;
    }
    else if(C >= 'A' && C <= 'F')
    {
        *V = C - 'A' + 10;
    }
    else if(C >= '0' && C <= '9')
    {
        *V = C - '0';
    }
    else
    {
        return 0;
    }

    return 1;
}

static b32 gfxStrEqu(gfx_str* Str, const char* Cmp)
{
    for(usz Idx = 0; Idx < Str->Sz; Idx++)
    {
        if(Cmp[Idx] != Str->At[Idx])
        {
            return 0;
        }
    }

    return (Cmp[Str->Sz] == 0);
}

static b32 gfxStrToU32(gfx_str* Str, u32* Val)
{
    if(!Str->Sz)
    {
        // TODO: Logging
        return 0;
    }

    u32 Tmp = 0;

    do
    {
        char C = *Str->At;

        if(C < '0' && C > '9')
        {
            // TODO: Logging
            return 0;
        }

        u32 N = Tmp * 10 + (C - '0');
        if(N < Tmp)
        {
            // TODO: Logging
            return 0;
        }

        Tmp = N;
        Str->Sz--;
        Str->At++;
    }
    while(Str->Sz);

    *Val = Tmp;

    return 1;
}

static b32 gfxGetLine(gfx_str* Str, gfx_str* Line)
{
    b32 Result = 0;

    if(Str->Sz)
    {
        Line->At = Str->At;

        while(Str->Sz)
        {
            char C = *(Str->At++);
            Str->Sz--;
            if(C == '\n')
            {
                break;
            }
        }

        Line->Sz = Str->At - Line->At - 1;

        Result = 1;
    }

    return Result;
}

static b32 gfxTokenize(gfx_str* Str, gfx_str* Token)
{
    b32 Result = 0;

    if(Str->Sz)
    {
        while(Str->Sz && *Str->At == ' ')
        {
            Str->Sz--;
            Str->At++;
        }

        Token->At = Str->At;

        while(Str->Sz && *Str->At != ' ')
        {
            Str->At++;
            Str->Sz--;
        }

        Token->Sz = Str->At - Token->At;

        Result = 1;
    }

    return Result;
}

static b32 gfxParseLine(gfx_str* Str, gfx_str* Tokens, usz Count)
{
    b32 Result = 0;

    gfx_str Line;
    if(gfxGetLine(Str, &Line))
    {
        usz Idx = 0;
        while(Idx < Count && gfxTokenize(&Line, &Tokens[Idx]))
        {
            Idx++;
        }

        Result = 1;
    }

    return Result;
}

static b32 gfxReadFont(gfx_font* Font, gfx_str* Str)
{
    gfx_str Tokens[8] = {0};
    if(gfxParseLine(Str, Tokens, ArrLen(Tokens)) &&
       gfxStrEqu(&Tokens[0], "STARTFONT"))
    {
        while(1) // TODO: Break it
        {
            if(!gfxParseLine(Str, Tokens, ArrLen(Tokens)))
            {
                // TODO: Logging
                return 0;
            }

            if(gfxStrEqu(Tokens+0, "FONTBOUNDINGBOX"))
            {
                if(!gfxStrToU32(Tokens+1, &Font->Cols) ||
                   !gfxStrToU32(Tokens+2, &Font->Rows))
                {
                    // TODO: Logging
                    return 0;
                }
            }
            else if(gfxStrEqu(Tokens+0, "CHARS"))
            {
                if(Font->Data)
                {
                    VirtualFree(Font->Data, 0, MEM_DECOMMIT|MEM_RELEASE);
                }

                Font->Jump = Font->Cols * Font->Rows;
                Font->Size = Font->Jump * 256;
                Font->Data = VirtualAlloc(0, Font->Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                if(!Font->Data)
                {
                    // TODO: Logging
                    return 0;
                }
            }
            else if(gfxStrEqu(Tokens+0, "STARTCHAR"))
            {
                u32 Encoding = 0;
                while(1) // TODO: Break it
                {
                    if(!gfxParseLine(Str, Tokens, ArrLen(Tokens)))
                    {
                        // TODO: Logging
                        return 0;
                    }

                    if(gfxStrEqu(Tokens+0, "ENCODING"))
                    {
                        if(!gfxStrToU32(Tokens+1, &Encoding))
                        {
                            // TODO: Logging
                            return 0;
                        }

                        if(Encoding >= 256)
                        {
                            // TODO: Handle more chars
                            break;
                        }
                    }
                    else if(gfxStrEqu(Tokens+0, "BITMAP"))
                    {
                        u32 Bytes = ((Font->Cols + 7) / 8);
                        u8* ByteAt = Font->Data + Encoding * Font->Jump;
                        for(u32 Row = 0; Row < Font->Rows; Row++)
                        {
                            gfx_str Line;
                            if(!gfxGetLine(Str, &Line))
                            {
                                // TODO: Logging
                                return 0;
                            }

                            if(Line.Sz < 2*Bytes)
                            {
                                // TODO: Logging
                                return 0;
                            }

                            for(u32 Idx = 0; Idx < Bytes; Idx++)
                            {
                                u8 Hi = 0, Lo = 0;
                                if(!gfxHexToDec(Line.At[2*Idx+0], &Hi) ||
                                   !gfxHexToDec(Line.At[2*Idx+1], &Lo))
                                {
                                    // TODO: Logging
                                    return 0;
                                }

                                *(ByteAt++) = (Hi << 4) | Lo;
                            }
                        }
                    }
                    else if(gfxStrEqu(Tokens+0, "ENDCHAR"))
                    {
                        break;
                    }
                }
            }
            else if(gfxStrEqu(Tokens+0, "ENDFONT"))
            {
                break;
            }
        }
    }

    return 1;
}

static b32 gfxLoadFont(gfx_font* Font, const char* Name)
{
    b32 Result = 0;

    usz Size = 0;
    i8* Data = gfxLoadFile(Name, &Size);
    if(Data)
    {
        Data[Size] = 0;

        gfx_str Buf = {0};
        Buf.Sz = Size;
        Buf.At = Data;
        if(gfxReadFont(Font, &Buf))
        {
            Result = 1;
        }
        else
        {
            if(Font->Data)
            {
                VirtualFree(Font->Data, 0, MEM_DECOMMIT|MEM_RELEASE);
            }
        }

        gfxFree(Data);
    }

    return Result;
}

gfx_font Font;

#define GL_DEBUG_OUTPUT                   0x92E0
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC callback, const void *userParam);

static b32 gfxInit(void)
{
    b32 Result = 0;



    glEnable              ( GL_DEBUG_OUTPUT );
    // glDebugMessageCallback( gfxGlCallback, 0 );

    Result = gfxLoadFont(&Font, "spleen-5x8.bdf");

    return Result;
}
