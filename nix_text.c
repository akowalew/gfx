#include "gfx.c"
#include "text.c"

int main(int Argc, char** Argv)
{
    setenv("DISPLAY", ":0", 1); // TODO: Fix that
    Display* X11Display = XOpenDisplay(0);
    if(!X11Display)
    {
        perror("Failed to open display");
        return 1;
    }

    Window X11Root = DefaultRootWindow(X11Display);
    GLint GlAttributes[] =
    {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        GLX_SAMPLE_BUFFERS, 1,
        GLX_SAMPLES, 4,
        None
    };

    XVisualInfo* X11VisualInfo = glXChooseVisual(X11Display, 0, GlAttributes);
    if(!X11VisualInfo)
    {
        perror("No appropriate visual found for display");
        return 1;
    }

    Colormap X11Colormap = XCreateColormap(X11Display, X11Root, X11VisualInfo->visual, AllocNone);

    XSetWindowAttributes X11WindowAttributes = {0};
    X11WindowAttributes.colormap = X11Colormap;
    X11WindowAttributes.event_mask = ExposureMask|KeyPressMask|PointerMotionMask|ButtonPressMask|ButtonReleaseMask;
    Window X11Window = XCreateWindow(X11Display, X11Root, 0, 0, 800, 600, 0, X11VisualInfo->depth,
                                     InputOutput, X11VisualInfo->visual, CWColormap|CWEventMask, &X11WindowAttributes);
    XStoreName(X11Display, X11Window, "Text view");
    XMapWindow(X11Display, X11Window);

    GLXContext GlContext = glXCreateContext(X11Display, X11VisualInfo, 0, GL_TRUE);
    if(!GlContext)
    {
        perror("Failed to create gl context");
        return 1;
    }

    glXMakeCurrent(X11Display, X11Window, GlContext);

    Atom WM_DELETE_WINDOW = XInternAtom(X11Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(X11Display, X11Window, &WM_DELETE_WINDOW, 1);

    Assert(gfxInit());

    glEnable(GL_MULTISAMPLE);

    u32 ShouldExit = 0;
    while(ShouldExit == 0)
    {
        do
        {
            XEvent X11Event;
            XNextEvent(X11Display, &X11Event);
            if(X11Event.type == KeyPress)
            {
                XKeyEvent *X11KeyEvent = (XKeyEvent *)&X11Event;
                if(X11KeyEvent->state & ShiftMask)
                {
                    GfxKeyShift = 1;
                }

                KeySym X11Key = XLookupKeysym(&X11Event.xkey, 0);
                switch(X11Key)
                {
                    case XK_Escape:
                    {
                        ShouldExit = 1;
                    } break;

                    case XK_Left:
                    {
                        GfxKeyLeft++;
                    } break;

                    case XK_Right:
                    {
                        GfxKeyRight++;
                    } break;

                    case XK_Up:
                    {
                        GfxKeyUp++;
                    } break;

                    case XK_Down:
                    {
                        GfxKeyDown++;
                    } break;
                }
            }
            else if(X11Event.type == ClientMessage)
            {
                if(X11Event.xclient.data.l[0] == (int)WM_DELETE_WINDOW)
                {
                    ShouldExit = 1;
                }
            }
        } while(XPending(X11Display));

        XWindowAttributes X11Attributes;
        XGetWindowAttributes(X11Display, X11Window, &X11Attributes);
        GfxCols = X11Attributes.width;
        GfxRows = X11Attributes.height;

        unsigned BtnsMask;
        int WinCurX, WinCurY;
        int RootCurX, RootCurY;
        Window X11ChildReturnWindow;
        XQueryPointer(X11Display, X11Root, &X11Root, &X11ChildReturnWindow, &RootCurX, &RootCurY, &WinCurX, &WinCurY, &BtnsMask);
        XTranslateCoordinates(X11Display, X11Root, X11Window, RootCurX, RootCurY, &WinCurX, &WinCurY, &X11ChildReturnWindow);
        GfxCur[0] = WinCurX;
        GfxCur[1] = WinCurY;
        GfxBtn = (BtnsMask & Button1Mask);

        AppUpdate();

        GfxKeyLeft = 0;
        GfxKeyRight = 0;
        GfxKeyUp = 0;
        GfxKeyDown = 0;
        GfxKeyShift = 0;

        glXSwapBuffers(X11Display, X11Window);
    }
}