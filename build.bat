@echo off
set WFLAGS=/W4 /wd4100 /wd4101 /wd4189 /wd4706
set LFLAGS=/NOLOGO /INCREMENTAL:NO /SUBSYSTEM:Windows /WX /DEBUG:FULL user32.lib opengl32.lib gdi32.lib
set CFLAGS=/nologo /MT /Oi /Gw- /GL- /GR- /GS- /Zf /Zi /EHa- /EHs- /EHc- /DBUILD_WIN32=1
cl win32_text.c /Fe:text.exe %CFLAGS% %WFLAGS% /link %LFLAGS%