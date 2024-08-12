#include "gfx.c"

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
    GLint GlAttributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo* X11VisualInfo = glXChooseVisual(X11Display, 0, GlAttributes);
    if(!X11VisualInfo)
    {
        perror("No appropriate visual found for display");
        return 1;
    }

    Colormap X11Colormap = XCreateColormap(X11Display, X11Root, X11VisualInfo->visual, AllocNone);

    XSetWindowAttributes X11WindowAttributes = {0};
    X11WindowAttributes.colormap = X11Colormap;
    X11WindowAttributes.event_mask = ExposureMask|KeyPressMask;
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

    u32 num = 'a';

    XEvent X11Event;
    u32 ShouldExit = 0;
    while(ShouldExit == 0)
    {
        XNextEvent(X11Display, &X11Event);
        if(X11Event.type == KeyPress)
        {
            KeySym X11Key = XLookupKeysym(&X11Event.xkey, 0);
            printf("%lu\n", X11Key);
            switch(X11Key)
            {
                case XK_Left:
                {
                    num = (num - 1) & 255;
                } break;

                case XK_Right:
                {
                    num = (num + 1) & 255;
                } break;

                case XK_Escape:
                {
                    ShouldExit = 1;
                } break;
            }
        }
        else if(X11Event.type == ClientMessage)
        {
            if(X11Event.xclient.data.l[0] == (int)WM_DELETE_WINDOW)
            {
                break;
            }
        }

        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glBindTexture(GL_TEXTURE_2D, Fnt.Text);

        f32 rat = num / 256.0f;
        f32 bat = rat + 1/256.0f;
        glBegin(GL_TRIANGLES);
        {
            /* glColor3f(1.0f, 0.0f, 0.0f); */ glTexCoord2f(0.0f, bat); glVertex2f(-1.0f, -1.0f);
            /* glColor3f(0.0f, 1.0f, 0.0f); */ glTexCoord2f(1.0f, bat); glVertex2f(+1.0f, -1.0f);
            /* glColor3f(0.0f, 0.0f, 1.0f); */ glTexCoord2f(0.0f, rat); glVertex2f(-1.0f, +1.0f);

            /* glColor3f(1.0f, 0.0f, 0.0f); */ glTexCoord2f(1.0f, rat); glVertex2f(+1.0f, +1.0f);
            /* glColor3f(0.0f, 1.0f, 0.0f); */ glTexCoord2f(1.0f, bat); glVertex2f(+1.0f, -1.0f);
            /* glColor3f(0.0f, 0.0f, 1.0f); */ glTexCoord2f(0.0f, rat); glVertex2f(-1.0f, +1.0f);
        }
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);

        glXSwapBuffers(X11Display, X11Window);
    }
}