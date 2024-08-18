#include "gfx.c"

u32 num = 'a';

static LRESULT WindowProc(HWND Window, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Msg)
    {
        case WM_KEYDOWN:
        {
            switch(WParam)
            {
                case VK_ESCAPE:
                {
                    PostQuitMessage(0);
                } break;

                case VK_LEFT:
                {
                    num = (num - 1) & 255;
                } break;

                case VK_RIGHT:
                {
                    num = (num + 1) & 255;
                } break;
            }
        } break;

        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;

        default:
        {
            Result = DefWindowProc(Window, Msg, WParam, LParam);
        } break;
    }

    return Result;
}

static HWND Window;

int APIENTRY WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow)
{
    WNDCLASSEX WindowClassEx = {0};
    WindowClassEx.cbSize = sizeof(WindowClassEx);
    WindowClassEx.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClassEx.lpfnWndProc = WindowProc;
    WindowClassEx.cbClsExtra = 0;
    WindowClassEx.cbWndExtra = 0;
    WindowClassEx.hInstance = Instance;
    WindowClassEx.hIcon = 0;
    WindowClassEx.hCursor = LoadCursorA(0, IDC_ARROW);
    WindowClassEx.hbrBackground = 0;
    WindowClassEx.lpszMenuName = 0;
    WindowClassEx.lpszClassName = "ViewWindowClass";
    WindowClassEx.hIconSm = 0;
    Assert(RegisterClassEx(&WindowClassEx));

    DWORD Style = WS_OVERLAPPEDWINDOW|WS_VISIBLE;
    DWORD ExStyle = WS_EX_OVERLAPPEDWINDOW;
    Assert(Window = CreateWindowExA(ExStyle, "ViewWindowClass", "View", Style,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             0, 0, Instance, 0));

    HDC DC;
    Assert(DC = GetDC(Window));

    int PixelFormat;
    PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {0};
    PixelFormatDescriptor.nSize = sizeof(PixelFormatDescriptor);
    PixelFormatDescriptor.nVersion = 1;
    PixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    PixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
    PixelFormatDescriptor.cColorBits = 24;
    PixelFormatDescriptor.cAlphaBits = 8;
    PixelFormatDescriptor.cAccumBits = 0;
    PixelFormatDescriptor.cDepthBits = 0;
    PixelFormatDescriptor.cStencilBits = 0;
    PixelFormatDescriptor.cAuxBuffers = 0;
    PixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;
    Assert(PixelFormat = ChoosePixelFormat(DC, &PixelFormatDescriptor));
    Assert(SetPixelFormat(DC, PixelFormat, &PixelFormatDescriptor));

    HGLRC GLRC;
    Assert(GLRC = wglCreateContext(DC));
    Assert(wglMakeCurrent(DC, GLRC));
    Assert(gfxInit());

    gfx_img Img;
    Assert(gfxLoadBmp(&Img, "test.bmp"));

    MSG Msg;
    b32 OneMoreTime = 0;
    while(1)
    {
        // TODO: Ideally we want to have some smooth animation?
        if(!OneMoreTime)
        {
            if(GetMessage(&Msg, 0, 0, 0))
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);

                OneMoreTime = 1;
            }
            else
            {
                break;
            }
        }
        else
        {
            OneMoreTime = 0;
        }

        RECT ClientRect;
        Assert(GetClientRect(Window, &ClientRect));
        f32 Cols = (f32)(ClientRect.right - ClientRect.left);
        f32 Rows = (f32)(ClientRect.bottom - ClientRect.top);
        glViewport(0, 0, (i32)Cols, (i32)Rows);

        POINT CursorPos;
        Assert(GetCursorPos(&CursorPos));
        Assert(ScreenToClient(Window, &CursorPos));

        GfxCur[0] = (f32) (CursorPos.x);
        GfxCur[1] = (f32) (CursorPos.y);

        GfxBtn = GetKeyState(VK_LBUTTON) >> 15;

        glClearColor(0.0f, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        m4f ProjectionMatrix;
        gfxIdentity(ProjectionMatrix);
        gfxOrtho(ProjectionMatrix, 0, Cols, 0, Rows, -1.0f, 1.0f);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(ProjectionMatrix);

        m4f ModelViewMatrix;
        gfxIdentity(ModelViewMatrix);
        gfxTranslateY(ModelViewMatrix, Rows);
        gfxScaleY(ModelViewMatrix, -1.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(ModelViewMatrix);

        m4f TextureMatrix;
        gfxIdentity(TextureMatrix);
        glMatrixMode(GL_TEXTURE);
        glLoadMatrixf(TextureMatrix);

        gfxBegin();
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            gfxText("Hello world!");
            gfxText("Welcome to Windows.");

            if(gfxButton("Push me"))
            {
                gfxDebugPrint("Push me\n");
            }

            if(gfxButton("I am not kidding"))
            {
                gfxDebugPrint("I am not kidding\n");
            }

            if(gfxButton("C'mon man!"))
            {
                gfxDebugPrint("C'mon man!\n");
            }

            static int RadioValue = 0;
            gfxRadioButton("Radio button 0", &RadioValue, 0);
            gfxRadioButton("Radio button 1", &RadioValue, 1);
            gfxRadioButton("Radio button 2", &RadioValue, 2);

            glColor3f(1.0f, 0.0f, 0.0f);
            gfxTextAt(GfxCur, "I am moving");
        }
        gfxEnd();

        Assert(SwapBuffers(DC));

        if(!GfxBtn)
        {
            GfxHot = 0;
        }
    }

    return 0;
}