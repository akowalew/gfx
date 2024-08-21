#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>

#define PI_F32   3.14159265358979323846264338327950288f

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
#define Max(x, y) ((x) > (y) ? (x) : (y))

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
                                    f32 V = (Val & 0x80) ? 1.0f : 0.0f;
                                    *(PixelAt++) = V;
                                    *(PixelAt++) = V;
                                    *(PixelAt++) = V;
                                    *(PixelAt++) = V;
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
    u32 Texture;
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
                    glGenTextures(1, &Img->Texture);
                    glBindTexture(GL_TEXTURE_2D, Img->Texture);
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
typedef float v2f[2];
typedef float v3f[3];

static void gfxIdentity(m4f M)
{
    _mm_store_ps(&M[4*0], _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f));
    _mm_store_ps(&M[4*1], _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f));
    _mm_store_ps(&M[4*2], _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f));
    _mm_store_ps(&M[4*3], _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));
}

static void gfxTranslateX(m4f M, f32 X)
{
    M[4*3+0] += X;
}

static void gfxTranslateY(m4f M, f32 Y)
{
    M[4*3+1] += Y;
}

static void gfxTranslateZ(m4f M, f32 Z)
{
    M[4*3+2] += Z;
}

static void gfxTranslate(m4f M, v3f V)
{
    M[4*3+0] += V[0];
    M[4*3+1] += V[1];
    M[4*3+2] += V[2];
}

static void gfxScaleX(m4f M, f32 X)
{
    M[4*0+0] *= X;
}

static void gfxScaleY(m4f M, f32 Y)
{
    M[4*1+1] *= Y;
}

static void gfxScaleZ(m4f M, f32 Z)
{
    M[4*2+2] *= Z;
}

static void gfxScale(m4f M, v3f V)
{
    M[4*0+0] *= V[0];
    M[4*1+1] *= V[1];
    M[4*2+2] *= V[2];
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

static gfx_fnt GfxFnt;
static f32 GfxSep = 5.f;
static v2f GfxPos = {0.0f, 0.0f};
static v2f GfxCur = {0.0f, 0.0f};
static b32 GfxBtn = 0;
static f32 GfxCols;
static f32 GfxRows;
static const void* GfxHot = 0;
static u8 GfxKeyLeft;
static u8 GfxKeyRight;
static u8 GfxKeyUp;
static u8 GfxKeyDown;

static void gfxText(const char* Text, usz Size)
{
    glBindTexture(GL_TEXTURE_2D, GfxFnt.Texture);

    f32 X = GfxPos[0];
    f32 Y = GfxPos[1];

    glBegin(GL_TRIANGLES);

    for(usz Idx = 0; Idx < Size; Idx++)
    {
        char C = Text[Idx];

        f32 rat = (C + 0) / 256.0f;
        f32 bat = (C + 1) / 256.0f;

        glTexCoord2f(0.0f, rat); glVertex2f(X, Y);
        glTexCoord2f(1.0f, rat); glVertex2f(X+GfxFnt.Cols, Y);
        glTexCoord2f(0.0f, bat); glVertex2f(X, Y+GfxFnt.Rows);

        glTexCoord2f(1.0f, bat); glVertex2f(X+GfxFnt.Cols, Y+GfxFnt.Rows);
        glTexCoord2f(1.0f, rat); glVertex2f(X+GfxFnt.Cols, Y);
        glTexCoord2f(0.0f, bat); glVertex2f(X, Y+GfxFnt.Rows);

        X += GfxFnt.Cols;
    }

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    GfxPos[1] += GfxFnt.Rows + GfxSep;
}

static void gfxString(const char* String)
{
    usz Length = strlen(String);
    gfxText(String, Length);
}

static void gfxImageAt(v2f Pos, gfx_img* Img)
{
    f32 X = Pos[0];
    f32 Y = Pos[1];

    glBindTexture(GL_TEXTURE_2D, Img->Texture);

    glBegin(GL_TRIANGLES);

    glTexCoord2f(0.0f, 1.0f); glVertex2f(X, Y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(X+Img->Cols, Y);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(X, Y+Img->Rows);

    glTexCoord2f(1.0f, 0.0f); glVertex2f(X+Img->Cols, Y+Img->Rows);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(X+Img->Cols, Y);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(X, Y+Img->Rows);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void gfxPolygon(f32 CX, f32 CY, f32 R, u32 N)
{
    glBegin(GL_POLYGON);
    for (u32 Idx = 0; Idx < N; Idx++)
    {
        f32 Theta = 2.0f * PI_F32 * Idx / N;
        f32 X = R * cosf(Theta);
        f32 Y = R * sinf(Theta);

        glVertex2f(X + CX, Y + CY);
    }
    glEnd();
}

static void gfxColorRGB8(u8 R, u8 G, u8 B)
{
    glColor3f(R / 255.F, G / 255.F, B / 255.F);
}

static b32 gfxPointInRect(v2f Pt, v2f TL, v2f BR)
{
    if(Pt[0] >= TL[0] && Pt[0] <= BR[0] &&
       Pt[1] >= TL[1] && Pt[1] <= BR[1])
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void gfxRect(f32 X1, f32 Y1, f32 X2, f32 Y2)
{
    glBegin(GL_TRIANGLES);

    glVertex2f(X1, Y1);
    glVertex2f(X2, Y1);
    glVertex2f(X1, Y2);

    glVertex2f(X2, Y2);
    glVertex2f(X2, Y1);
    glVertex2f(X1, Y2);

    glEnd();
}

typedef enum
{
    GFX_ITEM_IDLE,
    GFX_ITEM_HOVER,
    GFX_ITEM_ACTIVE,
    GFX_ITEM_RELEASE,
} gfx_its;

static int gfxProcessItem(const void* Item, v2f TL, v2f BR)
{
    if(gfxPointInRect(GfxCur, TL, BR))
    {
        if(GfxBtn)
        {
            if(GfxHot == 0 || GfxHot == Item)
            {
                GfxHot = Item;

                return GFX_ITEM_ACTIVE;
            }
            else
            {
                return GFX_ITEM_IDLE;
            }
        }
        else
        {
            if(GfxHot == Item)
            {
                return GFX_ITEM_RELEASE;
            }
            else
            {
                return GFX_ITEM_HOVER;
            }
        }
    }
    else
    {
        return GFX_ITEM_IDLE;
    }
}

static b32 gfxButton(const char* Text)
{
    b32 Result = 0;

    usz Len = strlen(Text);

    v2f TL, BR;
    TL[0] = GfxPos[0];
    TL[1] = GfxPos[1];
    BR[0] = GfxPos[0] + (Len + 1) * GfxFnt.Cols;
    BR[1] = GfxPos[1] + GfxFnt.Rows;

    switch(gfxProcessItem(Text, TL, BR))
    {
        case GFX_ITEM_IDLE:   gfxColorRGB8(44, 74, 114);  break;
        case GFX_ITEM_ACTIVE: gfxColorRGB8(15, 135, 250); break;
        case GFX_ITEM_RELEASE:  Result = 1; // fallthrough
        case GFX_ITEM_HOVER:  gfxColorRGB8(66, 150, 250); break;
    }

    gfxRect(TL[0], TL[1], BR[0], BR[1]);

    GfxPos[0] += GfxFnt.Cols/2;

    glColor3f(1.0f, 1.0f, 1.0f);
    gfxString(Text);

    GfxPos[0] -= GfxFnt.Cols/2;

    return Result;
}

static b32 gfxRadioButton(const char* Text, i32* Value, i32 Target)
{
    b32 Result = 0;

    usz Len = strlen(Text);

    v2f TL, BR;
    TL[0] = GfxPos[0];
    TL[1] = GfxPos[1];
    BR[0] = GfxPos[0] + GfxFnt.Rows + (Len + 0.5f) * GfxFnt.Cols;
    BR[1] = GfxPos[1] + GfxFnt.Rows;

    f32 R = GfxFnt.Rows * 0.5f;

    switch(gfxProcessItem(Text, TL, BR))
    {
        case GFX_ITEM_IDLE:   gfxColorRGB8(33, 51, 77);  break;
        case GFX_ITEM_ACTIVE: gfxColorRGB8(51, 107, 174); break;
        case GFX_ITEM_RELEASE:  *Value = Target; Result = 1; // fallthrough
        case GFX_ITEM_HOVER:  gfxColorRGB8(40, 74, 114); break;
    }

    gfxPolygon(GfxPos[0] + R, GfxPos[1] + R, R, 10);

    if(*Value == Target)
    {
        gfxColorRGB8(66, 150, 250);

        gfxPolygon(GfxPos[0] + R, GfxPos[1] + R, R*0.6f, 10);
    }

    GfxPos[0] += GfxFnt.Rows + GfxFnt.Cols/2;

    glColor3f(1.0f, 1.0f, 1.0f);

    gfxString(Text);

    GfxPos[0] -= GfxFnt.Rows + GfxFnt.Cols/2;

    return Result;
}

static void gfxCheckmark(f32 X, f32 Y, f32 M)
{
    // TODO: SIMD
    f32 Ax = X + 0.15f * M, Ay = Y + 0.40f * M;
    f32 Bx = X + 0.45f * M, By = Y + 0.80f * M;
    f32 Cx = X + 0.80f * M, Cy = Y + 0.20f * M;
    f32 Dx = X + 0.40f * M, Dy = Y + 0.55f * M;

    glBegin(GL_TRIANGLES);

    glVertex2f(Ax, Ay);
    glVertex2f(Bx, By);
    glVertex2f(Dx, Dy);

    glVertex2f(Dx, Dy);
    glVertex2f(Bx, By);
    glVertex2f(Cx, Cy);

    glEnd();
}

static b32 gfxCheckBox(const char* Text, b32* Value)
{
    b32 Result = 0;

    usz Len = strlen(Text);

    v2f TL, BR, MR;

    TL[0] = GfxPos[0];
    TL[1] = GfxPos[1];

    MR[0] = GfxPos[0] + GfxFnt.Rows;
    MR[1] = GfxPos[1] + GfxFnt.Rows;

    BR[0] = MR[0] + (Len + 0.5f) * GfxFnt.Cols;
    BR[1] = MR[1];

    switch(gfxProcessItem(Text, TL, BR))
    {
        case GFX_ITEM_IDLE:   gfxColorRGB8(33, 51, 77);  break;
        case GFX_ITEM_ACTIVE: gfxColorRGB8(51, 107, 174); break;
        case GFX_ITEM_RELEASE:  *Value = !*Value; Result = 1; // fallthrough
        case GFX_ITEM_HOVER:  gfxColorRGB8(40, 74, 114); break;
    }

    gfxRect(TL[0], TL[1], MR[0], MR[1]);

    if(*Value)
    {
        gfxColorRGB8(66, 150, 250);

#if 0
        gfxRect(TL[0] + 0.20f * GfxFnt.Rows, TL[1] + 0.20f * GfxFnt.Rows,
                TL[0] + 0.80f * GfxFnt.Rows, TL[1] + 0.80f * GfxFnt.Rows);
#else
        gfxCheckmark(TL[0], TL[1], (f32)GfxFnt.Rows);
#endif
    }

    GfxPos[0] += GfxFnt.Rows + GfxFnt.Cols/2;

    glColor3f(1.0f, 1.0f, 1.0f);

    gfxString(Text);

    GfxPos[0] -= GfxFnt.Rows + GfxFnt.Cols/2;

    return Result;
}

static usz gfxFormatV(char* Buffer, usz Length, const char* Format, va_list Args)
{
    int Count = vsnprintf(Buffer, Length, Format, Args);
    if(Count >= Length)
    {
        Count = (int)Length-1;
    }

    Buffer[Count] = 0;
    return (usz)Count;
}

#define Clamp(A, B, V) ((V) > (A) ? ((V) < (B) ? (V) : (B)) : (A))

static b32 gfxSliderFloat(f32 A, f32 B, f32* V, const char* Text, ...)
{
    b32 Result = 0;

    v2f STL, SBR;
    STL[0] = GfxPos[0];
    STL[1] = GfxPos[1];
    SBR[0] = STL[0] + 300;
    SBR[1] = STL[1] + GfxFnt.Rows;

    gfxColorRGB8(64, 68, 71);
    gfxRect(STL[0], STL[1], SBR[0], SBR[1]);

    va_list Args;
    char Buffer[128];
    va_start(Args, Text);
    usz Length = gfxFormatV(Buffer, sizeof(Buffer), Text, Args);
    va_end(Args);

    GfxPos[0] = (SBR[0] + STL[0] - Length * GfxFnt.Cols) * 0.5f;
    glColor3f(1.0f, 1.0f, 1.0f);
    gfxText(Buffer, Length);
    GfxPos[0] = STL[0];

    f32 Width = 25.0f;
    f32 Offset = (SBR[0] - STL[0] - Width) * (*V - A) / (B - A);

    v2f BTL, BBR;
    BTL[0] = STL[0] + Offset;
    BTL[1] = STL[1];
    BBR[0] = BTL[0] + Width;
    BBR[1] = SBR[1];

    switch(gfxProcessItem(V, BTL, BBR))
    {
        case GFX_ITEM_IDLE:    glColor4f(0.5f, 0.5f, 0.5f, 0.5f); break;
        case GFX_ITEM_ACTIVE:  glColor4f(0.8f, 0.8f, 0.8f, 0.8f); break;
        case GFX_ITEM_RELEASE: // fallthrough
        case GFX_ITEM_HOVER:   glColor4f(0.7f, 0.7f, 0.7f, 0.7f); break;
    }

    if(GfxHot == V)
    {
        glColor4f(0.8f, 0.8f, 0.8f, 0.8f);

        f32 Value = A + (GfxCur[0] - STL[0]) * (B - A) / (SBR[0] - STL[0]);

        *V = Clamp(A, B, Value);

        Result = 1;
    }

    gfxRect(BTL[0], BTL[1], BBR[0], BBR[1]);

    return Result;
}

static void gfxBegin(void)
{
    GfxPos[0] = GfxSep;
    GfxPos[1] = GfxSep;
}

static void gfxEnd(void)
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

static b32 gfxInit(void)
{
    b32 Result = 1;

    Assert(glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) gfxGlGetProcAddress("glDebugMessageCallback"));

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDebugMessageCallback(gfxGlCallback, 0);

    Assert(gfxLoadBdf(&GfxFnt, "spleen-32x64.bdf"));
    GfxFnt.Cols/=2;
    GfxFnt.Rows/=2;

    return Result;
}
