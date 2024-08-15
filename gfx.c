#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

typedef char GLchar;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uint32_t b32;
typedef size_t usz;

#define Assert(x) if(!(x)) { *(int*)(0) = 0; }
#define ArrLen(x) sizeof(x)/sizeof(x[0])
#define Min(x, y) ((x) < (y) ? (x) : (y))

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
    u32 Skip;
    u32 Jump;
    u32 Size;
    u8* Data;
    u32 Texture;
} gfx_fnt;

#if defined(BUILD_WIN32)

//
// WIN32
//

#include <windows.h>
#include <GL/gl.h>

#define gfxGlGetProcAddress(Name) wglGetProcAddress(Name)

static void* gfxVirtualAlloc(usz Size)
{
    return VirtualAlloc(0, Size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

static void gfxVirtualFree(void* Data)
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

static void gfxDebugPrint(const char* String)
{
    OutputDebugStringA(String);
}

#elif defined(BUILD_LINUX)

//
// LINUX
//

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <GL/glx.h>

#define gfxGlGetProcAddress(Name) glXGetProcAddress((const GLubyte*) (Name))

static void* gfxVirtualAlloc(usz Size)
{
    return malloc(Size);
}

static void gfxVirtualFree(void* Data)
{
    free(Data);
}

static void* gfxLoadFile(const char* Name, usz* Size)
{
    void* Result = 0;

    int Fd = open(Name, O_RDONLY);
    if(Fd != -1)
    {
        struct stat Stat;
        if(fstat(Fd, &Stat) == 0)
        {
            char* Data = gfxVirtualAlloc(Stat.st_size + 1);
            if(Data)
            {
                ssize_t Count = read(Fd, Data, Stat.st_size);
                if(Count == Stat.st_size)
                {
                    Data[Count] = 0;
                    *Size = Count;
                    Result = Data;
                }
                else
                {
                    // TODO: Logging
                    gfxVirtualFree(Data);
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }

        close(Fd);
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

static void gfxDebugPrint(const char* String)
{
    fputs(String, stdout);
}

#endif

static void gfxError(const char* Format, ...)
{
    char String[1024];

    va_list VaList;
    va_start(VaList, Format);
    vsnprintf(String, sizeof(String), Format, VaList);
    va_end(VaList);

    gfxDebugPrint(String);

    Assert(0);
}

#define GL_DEBUG_TYPE_ERROR               0x824C

static void gfxGlCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    gfxError( "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

#define GL_DEBUG_OUTPUT                   0x92E0

typedef void (*GLDEBUGPROC) (GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

typedef void (*PFNGLDEBUGMESSAGECALLBACKPROC) (GLDEBUGPROC callback, const void *userParam);

static void (*glDebugMessageCallback) (GLDEBUGPROC callback, const void *userParam);


static gfx_buf gfxLoadBuf(const char* Name)
{
    gfx_buf Result = {0};

    Result.At = gfxLoadFile(Name, &Result.Sz);

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

static b32 gfxReadFnt(gfx_fnt* Fnt, gfx_str* Str)
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
                if(!gfxStrToU32(Tokens+1, &Fnt->Cols) ||
                   !gfxStrToU32(Tokens+2, &Fnt->Rows))
                {
                    // TODO: Logging
                    return 0;
                }
            }
            else if(gfxStrEqu(Tokens+0, "CHARS"))
            {
                if(Fnt->Data)
                {
                    gfxVirtualFree(Fnt->Data);
                }

                Fnt->Skip = Fnt->Cols * 4 * sizeof(f32);
                Fnt->Jump = Fnt->Skip * Fnt->Rows;
                Fnt->Size = Fnt->Jump * 256;
                Fnt->Data = gfxVirtualAlloc(Fnt->Size);
                if(!Fnt->Data)
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
                        u32 Bytes = ((Fnt->Cols + 7) / 8);
                        f32* PixelAt = (f32*) (Fnt->Data + Encoding * Fnt->Jump);
                        for(u32 Row = 0; Row < Fnt->Rows; Row++)
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

                                u8 Val = (Hi << 4) | Lo;
                                u32 Left = Fnt->Cols - Idx * 8;
                                u32 Iters = (Left > 8) ? 8 : Left;
                                for(u32 Jdx = 0; Jdx < Iters; Jdx++)
                                {
                                    *(PixelAt++) = (Val & 0x80) ? 1.0f : 0.0f;
                                    *(PixelAt++) = 0;
                                    *(PixelAt++) = 0;
                                    *(PixelAt++) = 0;
                                    Val <<= 1;
                                }
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

static b32 gfxLoadBdf(gfx_fnt* Fnt, const char* Name)
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
        if(gfxReadFnt(Fnt, &Buf))
        {
            GLuint Texture;
            glGenTextures(1, &Texture);
            glBindTexture(GL_TEXTURE_2D, Texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Fnt->Cols * 1, Fnt->Rows * 256, 0, GL_RGBA, GL_FLOAT, Fnt->Data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glBindTexture(GL_TEXTURE_2D, 0);
            Fnt->Texture = Texture;
            Result = 1;
        }
        else
        {
            if(Fnt->Data)
            {
                gfxVirtualFree(Fnt->Data);
            }
        }

        gfxVirtualFree(Data);
    }

    return Result;
}

typedef struct
{
    u32 Cols;
    u32 Rows;
    u64 Jump;
    u64 Size;
    u8* Data;
    u32 Text;
} gfx_img;

#pragma pack(push, 1)
typedef struct
{
    char Magic[2];
    u32 FileSize;
    u32 Reserved;
    u32 DataOffset;
} gfx_bmp_file_header;

typedef struct
{
    u32 HeaderSize;
    u32 BitmapWidth;
    u32 BitmapHeight;
    u16 ColorPlanes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 ImageSize;
    u32 HorizontalRes;
    u32 VerticalRes;
    u32 PaletteColors;
    u32 ColorsImportant;
} gfx_bmp_info_header;

typedef struct
{
    gfx_bmp_file_header FileHeader;
    gfx_bmp_info_header InfoHeader;
    // u8 Pixels[];
} gfx_bmp;
#pragma pack(pop)

static b32 gfxLoadBmp(gfx_img* Img, const char* Path)
{
    b32 Result = 0;

    gfx_buf Buf = gfxLoadBuf(Path);
    if(Buf.At)
    {
        gfx_bmp* Bmp;
        if(Buf.Sz > sizeof(*Bmp))
        {
            Bmp = (gfx_bmp*) Buf.At;
            if(Bmp->FileHeader.Magic[0] == 'B' &&
               Bmp->FileHeader.Magic[1] == 'M' &&
               Bmp->InfoHeader.HeaderSize == sizeof(Bmp->InfoHeader))
            {
                Img->Cols = Bmp->InfoHeader.BitmapWidth;
                Img->Rows = Bmp->InfoHeader.BitmapHeight;
                Img->Jump = (((Img->Cols * 3) + 3) / 4) * 4;
                Img->Size = Img->Jump * Img->Rows;
                if(Buf.Sz >= Bmp->FileHeader.DataOffset + Img->Size)
                {
                    Img->Data = Buf.At + Bmp->FileHeader.DataOffset;
                    glGenTextures(1, &Img->Text);
                    glBindTexture(GL_TEXTURE_2D, Img->Text);
                    glPixelStorei(GL_PACK_ROW_LENGTH, Img->Cols);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Img->Cols, Img->Rows, 0, GL_BGR, GL_UNSIGNED_BYTE, Img->Data);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    glBindTexture(GL_TEXTURE_2D, 0);
                    Result = 1;
                }
                else
                {
                    // TODO: Logging
                }
            }
            else
            {
                // TODO: Logging
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        // TODO: Logging
    }

    return Result;
}

#include <immintrin.h>

typedef float m4f[16];
typedef float v4f[4];

static void gfxIdentity(m4f M)
{
    _mm_store_ps(&M[4*0], _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f));
    _mm_store_ps(&M[4*1], _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    _mm_store_ps(&M[4*2], _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_store_ps(&M[4*3], _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));
}

static void gfxOrtho(f32* M, f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
{
    M[0]  = 2.0f / (Right - Left);
    M[1]  = 0.0f;
    M[2]  = 0.0f;
    M[3]  = 0.0f;

    M[4]  = 0.0f;
    M[5]  = 2.0f / (Top - Bottom);
    M[6]  = 0.0f;
    M[7]  = 0.0f;

    M[8]  = 0.0f;
    M[9]  = 0.0f;
    M[10] = -2.0f / (Far - Near);
    M[11] = 0.0f;

    M[12] = -(Right + Left) / (Right - Left);
    M[13] = -(Top + Bottom) / (Top - Bottom);
    M[14] = -(Far + Near) / (Far - Near);
    M[15] = 1.0f;
}

gfx_fnt Fnt;

static void gfxTextAt(float X, float Y, const char* String)
{
    glBegin(GL_TRIANGLES);

    char C;
    while(C = *(String++))
    {
        f32 rat = C / 256.0f;
        f32 bat = rat + 1/256.0f;

        glTexCoord2f(0.0f, bat); glVertex2f(X-Fnt.Cols*0.5f, Y-Fnt.Rows*0.5f);
        glTexCoord2f(1.0f, bat); glVertex2f(X+Fnt.Cols*0.5f, Y-Fnt.Rows*0.5f);
        glTexCoord2f(0.0f, rat); glVertex2f(X-Fnt.Cols*0.5f, Y+Fnt.Rows*0.5f);

        glTexCoord2f(1.0f, rat); glVertex2f(X+Fnt.Cols*0.5f, Y+Fnt.Rows*0.5f);
        glTexCoord2f(1.0f, bat); glVertex2f(X+Fnt.Cols*0.5f, Y-Fnt.Rows*0.5f);
        glTexCoord2f(0.0f, rat); glVertex2f(X-Fnt.Cols*0.5f, Y+Fnt.Rows*0.5f);

        X += Fnt.Cols;
    }

    glEnd();
}

gfx_img Img;

static b32 gfxInit(void)
{
    b32 Result = 1;

    Assert(glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) gfxGlGetProcAddress("glDebugMessageCallback"));

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gfxGlCallback, 0);

    Assert(gfxLoadBdf(&Fnt, "spleen-32x64.bdf"));
    Assert(gfxLoadBmp(&Img, "test.bmp"));

    return Result;
}
