#include "gfx.c"

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
    WindowClassEx.hCursor = 0;
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

    MSG Msg;
    while(GetMessage(&Msg, 0, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        RECT ClientRect;
        Assert(GetClientRect(Window, &ClientRect));
        int Cols = ClientRect.right - ClientRect.left;
        int Rows = ClientRect.bottom - ClientRect.top;
        glViewport(0, 0, Cols, Rows);

        glBegin(GL_TRIANGLES);
        {
            glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
            glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(+0.5f, -0.5f);
            glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(-0.5f, +0.5f);

            glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(+0.5f, +0.5f);
            glColor3f(0.0f, 1.0f, 0.0f); glVertex2f(+0.5f, -0.5f);
            glColor3f(0.0f, 0.0f, 1.0f); glVertex2f(-0.5f, +0.5f);
        }
        glEnd();

        Assert(SwapBuffers(DC));
    }

    return 0;
}