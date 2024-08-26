gfx_img TestBmp;

static void AppUpdate(void)
{
    static b32 Initialized = 0;
    if(!Initialized)
    {
        Assert(gfxLoadBmp(&TestBmp, "test.bmp"));
        TestBmp.Cols /= 4;
        TestBmp.Rows /= 4;
        Initialized = 1;
    }

    glViewport(0, 0, (i32)GfxCols, (i32)GfxRows);

    glClearColor(0.0f, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    m4f ProjectionMatrix;
    gfxIdentity(ProjectionMatrix);
    gfxOrtho(ProjectionMatrix, 0, GfxCols, 0, GfxRows, -1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(ProjectionMatrix);

    m4f ModelViewMatrix;
    gfxIdentity(ModelViewMatrix);
    gfxTranslateY(ModelViewMatrix, GfxRows);
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
        gfxString("Hello world!");
        gfxString("Welcome to Windows.");

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

        gfxImage(&TestBmp);

        static i32 RadioValue = 0;
        gfxRadioButton("Radio button 0", &RadioValue, 0);
        gfxRadioButton("Radio button 1", &RadioValue, 1);
        gfxRadioButton("Radio button 2", &RadioValue, 2);

        static b32 CheckBox1 = 1;
        gfxCheckBox("Check box 1", &CheckBox1);
        static b32 CheckBox2 = 0;
        gfxCheckBox("Check box 2", &CheckBox2);

        static f32 Slider1 = 125.0f;
        gfxSliderFloat(100.0f, 200.f, &Slider1, "%.1lf");
    }
    gfxEnd();

    gfxBegin();
    {
        GfxPos[0] += 500;
        gfxGroupBox("Some group");

        if(gfxButton("Yet another"))
        {
            gfxDebugPrint("Yet another");
        }
    }
    gfxEnd();

    GfxPos[0] = GfxCur[0];
    GfxPos[1] = GfxCur[1];
    glColor3f(1.0f, 0.0f, 0.0f);
    gfxString("I am moving");

    if(!GfxBtn)
    {
        if(GfxHot)
        {
            GfxPrevHot = GfxHot;
        }

        GfxHot = 0;
    }
}