static void AppUpdate(void)
{
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

    if(!GfxBtn)
    {
        GfxHot = 0;
    }
}