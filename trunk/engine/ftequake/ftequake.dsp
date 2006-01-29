# Microsoft Developer Studio Project File - Name="ftequake" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ftequake - Win32 Debug Dedicated ServerQ3
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ftequake.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ftequake.mak" CFG="ftequake - Win32 Debug Dedicated ServerQ3"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ftequake - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 GLDebug" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 GLRelease" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 MDebug" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 MRelease" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 MinGLDebug" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 MinGLRelease" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 Debug Dedicated Server" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 Release Dedicated Server" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 MinSW" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 GLDebugQ3" (based on "Win32 (x86) Application")
!MESSAGE "ftequake - Win32 Debug Dedicated ServerQ3" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX /O2 /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "SWQUAKE" /FR /Yu"quakedef.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /out:"../../fteswqw.exe" /libpath:"../libs/dxsdk7/lib"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W3 /Gm /GX /ZI /Od /I "../client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SWQUAKE" /FR".\Debug/" /Fp".\Debug/qwcl.pch" /Yu"quakedef.h" /Fo".\Debug/" /Fd".\Debug/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../../fteswqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "GLDebug"
# PROP BASE Intermediate_Dir "GLDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "GLDebug"
# PROP Intermediate_Dir "GLDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /Yu"quakedef.h" /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../../fteglqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ftequake___Win32_GLRelease"
# PROP BASE Intermediate_Dir "ftequake___Win32_GLRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ftequake___Win32_GLRelease"
# PROP Intermediate_Dir "ftequake___Win32_GLRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\client" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /W3 /GX /O2 /Ob2 /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "NDEBUG" /D "_MBCS" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR /Yu"quakedef.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib ..\dxsdk\sdk\lib\dxguid.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /machine:I386 /out:"../../../fteqw.exe"
# ADD LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /out:"../../fteglqw.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_MDebug"
# PROP BASE Intermediate_Dir "ftequake___Win32_MDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\MDebug"
# PROP Intermediate_Dir ".\MDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client\gltod3d\sdk7\include" /I "..\client\gltod3d\D3DFrame" /I "..\dxsdk\sdk\inc" /I "..\scitech\include" /I "..\client" /D "NQPROT" /D "_DEBUG" /D "GLQUAKE" /D "SERVERDLL" /D "WIN32" /D "_WINDOWS" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /YX /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD CPP /nologo /G5 /W3 /Gi /GX /ZI /Od /I "..\client" /I "../libs/dxsdk7/include" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "GLQUAKE" /D "SWQUAKE" /D "USE_D3D" /Fr /Fp".\MDebug/qwcl.pch" /Yu"quakedef.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:".\GLDebug/dglqwcl.pdb" /debug /machine:I386 /out:"../../../fteglqw.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../../fteqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /profile /pdb:none

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ftequake___Win32_MRelease"
# PROP BASE Intermediate_Dir "ftequake___Win32_MRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MRelease"
# PROP Intermediate_Dir "MRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\client\gltod3d\sdk7\include" /I "..\client\gltod3d\D3DFrame" /I "..\dxsdk\sdk\inc" /I "..\scitech\include" /I "..\client" /D "NOSOUNDASM" /D "NDEBUG" /D "_MBCS" /D "GLQUAKE" /D "SERVERDLL" /D "NQPROT" /D "WIN32" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /G6 /GX /O2 /I "..\client" /I "../libs/dxsdk7/include" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "GLQUAKE" /D "SWQUAKE" /D "USE_D3D" /Fr /Yu"quakedef.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /machine:I386 /out:"../../../fteglqw.exe"
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /out:"../../fteqw.exe" /libpath:"../libs/dxsdk7/lib"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_MinGLDebug"
# PROP BASE Intermediate_Dir "ftequake___Win32_MinGLDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ftequake___Win32_MinGLDebug"
# PROP Intermediate_Dir "ftequake___Win32_MinGLDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client\gltod3d\sdk7\include" /I "..\client\gltod3d\D3DFrame" /I "..\dxsdk\sdk\inc" /I "..\scitech\include" /I "..\client" /D "NQPROT" /D "_DEBUG" /D "GLQUAKE" /D "SERVERDLL" /D "WIN32" /D "_WINDOWS" /D "Q2SERVER" /D "DYNAMIC_ENTS" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /YX /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "MINIMAL" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /Yu"quakedef.h" /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:".\GLDebug/dglqwcl.pdb" /debug /machine:I386 /out:"../../../fteglqw.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:none /map /debug /machine:I386 /out:"../../fteminglqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ftequake___Win32_MinGLRelease"
# PROP BASE Intermediate_Dir "ftequake___Win32_MinGLRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ftequake___Win32_MinGLRelease"
# PROP Intermediate_Dir "ftequake___Win32_MinGLRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gr /W3 /GX /O2 /Ob2 /I "..\client\gltod3d\sdk7\include" /I "..\client\gltod3d\D3DFrame" /I "..\dxsdk\sdk\inc" /I "..\scitech\include" /I "..\client" /D "NOSOUNDASM" /D "NDEBUG" /D "_MBCS" /D "GLQUAKE" /D "SERVERDLL" /D "NQPROT" /D "WIN32" /D "_WINDOWS" /D "Q2SERVER" /D "DYNAMIC_ENTS" /FR /YX /FD /c
# ADD CPP /nologo /G6 /Gr /W3 /GX /O2 /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "MINIMAL" /D "NDEBUG" /D "_MBCS" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR /Yu"quakedef.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /machine:I386 /out:"../../../fteglqw.exe"
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /out:"../../fteminglqw.exe" /libpath:"../libs/dxsdk7/lib"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_Debug_Dedicated_Server"
# PROP BASE Intermediate_Dir "ftequake___Win32_Debug_Dedicated_Server"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugServer"
# PROP Intermediate_Dir "DebugServer"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /D "MINIMAL" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /YX /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD CPP /nologo /G6 /ML /W3 /Gm /Gi /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SERVERONLY" /FR".\DebugServer/" /Fp".\DebugServer/qwcl.pch" /Yu"quakedef.h" /Fo".\DebugServer/" /Fd".\DebugServer/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:".\GLDebug/dglqwcl.pdb" /debug /machine:I386 /out:"../../../fteminglqw.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /map /debug /machine:I386 /out:"../../fteqwsv.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_Release_Dedicated_Server"
# PROP BASE Intermediate_Dir "ftequake___Win32_Release_Dedicated_Server"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ftequake___Win32_Release_Dedicated_Server"
# PROP Intermediate_Dir "ftequake___Win32_Release_Dedicated_Server"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /D "MINIMAL" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "SERVERONLY" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /YX /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD CPP /nologo /G6 /ML /W3 /GX /O1 /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "SERVERONLY" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /Yu"quakedef.h" /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /pdb:".\GLDebug/dglqwcl.pdb" /debug /machine:I386 /out:"../../../fteminglqw.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 winmm.lib wsock32.lib user32.lib shell32.lib Advapi32.lib /nologo /subsystem:console /pdb:none /map /machine:I386 /out:"../../fteqwsv.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_MinSW"
# PROP BASE Intermediate_Dir "ftequake___Win32_MinSW"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ftequake___Win32_MinSW"
# PROP Intermediate_Dir "ftequake___Win32_MinSW"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W3 /Gm /GX /ZI /Od /I "../client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SWQUAKE" /FR".\Debug/" /Fp".\Debug/qwcl.pch" /YX /Fo".\Debug/" /Fd".\Debug/" /FD /c
# ADD CPP /nologo /G6 /W3 /Gm /GX /ZI /Od /I "../client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SWQUAKE" /D "MINIMAL" /FR".\Debug/" /Fp".\Debug/qwcl.pch" /Yu"quakedef.h" /Fo".\Debug/" /Fd".\Debug/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../fteswqw_dbg.exe" /pdbtype:sept
# ADD LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /pdb:none /map /debug /machine:I386 /out:"../../fteswqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_GLDebugQ3"
# PROP BASE Intermediate_Dir "ftequake___Win32_GLDebugQ3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ftequake___Win32_GLDebugQ3"
# PROP Intermediate_Dir "ftequake___Win32_GLDebugQ3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /ML /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "AVAIL_OGGVORBIS" /D "Q3CLIENT" /FR".\GLDebug/" /Fp".\GLDebug/qwcl.pch" /YX /Fo".\GLDebug/" /Fd".\GLDebug/" /FD /c
# SUBTRACT BASE CPP /X
# ADD CPP /nologo /G5 /W3 /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "GLQUAKE" /D "WIN32" /D "_WINDOWS" /D "Q3CLIENT" /D "Q3SERVER" /FR /Fp".\GLDebugQ3/qwcl.pch" /Yu"quakedef.h" /Fo".\GLDebugQ3/" /Fd".\GLDebugQ3/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /fo"GLDebugQ3/winquake.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"GLDebugQ3/ftequake.bsc"
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:windows /pdb:".\GLDebug/dglqwcl.pdb" /debug /machine:I386 /out:"../../fteglqw_dbg.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 comctl32.lib wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"../../fteglqw_dbg.exe" /libpath:"../libs/dxsdk7/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ftequake___Win32_Debug_Dedicated_ServerQ3"
# PROP BASE Intermediate_Dir "ftequake___Win32_Debug_Dedicated_ServerQ3"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ftequake___Win32_Debug_Dedicated_ServerQ3"
# PROP Intermediate_Dir "ftequake___Win32_Debug_Dedicated_ServerQ3"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /ML /W3 /Gm /Gi /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SERVERONLY" /FR".\DebugServer/" /Fp".\DebugServer/qwcl.pch" /Yu"quakedef.h" /Fo".\DebugServer/" /Fd".\DebugServer/" /FD /c
# ADD CPP /nologo /G6 /ML /W3 /Gm /Gi /GX /ZI /Od /I "..\client" /I "../common" /I "../server" /I "../gl" /I "../sw" /I "../qclib" /I "../libs" /I "../libs/dxsdk7/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "SERVERONLY" /D "Q3SERVER" /FR".\DebugServer/" /Fp".\DebugServer/qwcl.pch" /Yu"quakedef.h" /Fo".\DebugServer/" /Fd".\DebugServer/" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"../../fteqwsv.exe" /libpath:"../libs/dxsdk7/lib"
# ADD LINK32 comctl32.lib wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /subsystem:console /pdb:none /map /debug /machine:I386 /out:"../../fteqwsv.exe" /libpath:"../libs/dxsdk7/lib"

!ENDIF 

# Begin Target

# Name "ftequake - Win32 Release"
# Name "ftequake - Win32 Debug"
# Name "ftequake - Win32 GLDebug"
# Name "ftequake - Win32 GLRelease"
# Name "ftequake - Win32 MDebug"
# Name "ftequake - Win32 MRelease"
# Name "ftequake - Win32 MinGLDebug"
# Name "ftequake - Win32 MinGLRelease"
# Name "ftequake - Win32 Debug Dedicated Server"
# Name "ftequake - Win32 Release Dedicated Server"
# Name "ftequake - Win32 MinSW"
# Name "ftequake - Win32 GLDebugQ3"
# Name "ftequake - Win32 Debug Dedicated ServerQ3"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "server"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\server\net_preparse.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\pr_cmds.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\savegame.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_ccmds.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_chat.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_demo.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_ents.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_init.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_main.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_master.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_move.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_mvd.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_nchan.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_phys.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_rankin.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_send.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\sv_user.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\svq2_ents.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\svq2_game.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# Begin Source File

SOURCE=..\server\svq3_game.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\server\world.c
# ADD CPP /Yu"qwsvdef.h"
# End Source File
# End Group
# Begin Group "client"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\client\cd_win.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_cam.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_cg.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_demo.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_ents.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_ignore.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_input.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_main.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_parse.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_pred.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_screen.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_tent.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\cl_ui.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\clq2_cin.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\clq2_ents.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\clq3_parse.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\console.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\fragstats.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\image.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\in_win.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\keys.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_download.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_items.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_master.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_mp3.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_multi.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_options.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_script.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\m_single.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\menu.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\net_master.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\pr_csqc.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\pr_menu.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1
# ADD CPP /WX

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD BASE CPP /WX
# ADD CPP /WX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\r_bulleten.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\r_efrag.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# ADD CPP /G6 /O1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\r_part.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\r_partset.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\renderer.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\renderque.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\roq_read.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\sbar.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\skin.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_directx.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_dma.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_mem.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_mix.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_ov.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_win.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\sys_win.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\teamplay.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\textedit.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\valid.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\view.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\wad.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\zqtp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "gl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gl\doomclip.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\doommap.h
# End Source File
# Begin Source File

SOURCE=..\gl\gl_alias.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_backend.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_draw.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gltod3d\gl_fakegl.cpp

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_heightmap.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_hlmdl.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_model.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_ngraph.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_ppl.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\gl_q2bsp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_rlight.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_rmain.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_rmisc.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_rsurf.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_screen.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_shader.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_vidcommon.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_vidnt.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\gl_warp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\glmod_doom.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\LTFACE.C

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\gl\shader.h
# End Source File
# End Group
# Begin Group "sw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sw\d_edge.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_fill.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_init.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_modech.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_part.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_polyse.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_scan.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_sky.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_sprite.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_surf.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_trans.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_vars.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_zpoint.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\nonintel.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_aclip.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_alias.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_bsp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_draw.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_edge.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_light.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_main.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_misc.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_sky.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_sprite.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_surf.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_vars.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\sw_draw.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\sw_model.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\sw_screen.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\vid_ddraw.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\vid_dib.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\vid_win2.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\client\cl_plugin.inc
# End Source File
# Begin Source File

SOURCE=..\common\cmd.c
# End Source File
# Begin Source File

SOURCE=..\common\common.c
# ADD CPP /Yc"quakedef.h"
# End Source File
# Begin Source File

SOURCE=..\common\crc.c
# End Source File
# Begin Source File

SOURCE=..\common\cvar.c
# End Source File
# Begin Source File

SOURCE=..\common\fs.c
# End Source File
# Begin Source File

SOURCE=..\common\huff.c
# End Source File
# Begin Source File

SOURCE=..\common\log.c
# End Source File
# Begin Source File

SOURCE=..\common\mathlib.c
# End Source File
# Begin Source File

SOURCE=..\common\md4.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\net_chan.c
# End Source File
# Begin Source File

SOURCE=..\common\net_wins.c
# End Source File
# Begin Source File

SOURCE=..\common\plugin.c
# End Source File
# Begin Source File

SOURCE=..\common\pmove.c
# End Source File
# Begin Source File

SOURCE=..\common\pmovetst.c
# End Source File
# Begin Source File

SOURCE=..\common\q1bsp.c
# End Source File
# Begin Source File

SOURCE=..\common\q2pmove.c
# End Source File
# Begin Source File

SOURCE=..\common\q3common.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\qvm.c
# End Source File
# Begin Source File

SOURCE=..\common\translate.c
# End Source File
# Begin Source File

SOURCE=..\common\zone.c
# End Source File
# End Group
# Begin Group "QCLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\QCLIB\Comprout.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\qclib\execloop.h
# End Source File
# Begin Source File

SOURCE=..\QCLIB\hash.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\initlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\pr_edict.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\Pr_exec.c
# SUBTRACT CPP /WX /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\pr_multi.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\progtype.h
# End Source File
# Begin Source File

SOURCE=..\QCLIB\qcc_cmdlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\qcc_pr_comp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\QCLIB\qcc_pr_lex.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\QCLIB\QccMain.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\QCLIB\qcd_main.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\QCLIB\qcdecomp.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sw\d_draw.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_draw.s
InputName=d_draw

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_draw16.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_draw16.s
InputName=d_draw16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_parta.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_parta.s
InputName=d_parta

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_polysa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_polysa.s
InputName=d_polysa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_scana.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_scana.s
InputName=d_scana

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_spr8.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_spr8.s
InputName=d_spr8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\d_varsa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\d_varsa.s
InputName=d_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\common\math.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLRelease
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# Begin Custom Build
OutDir=.\MRelease
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLDebug
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLRelease
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# Begin Custom Build
OutDir=.\DebugServer
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# Begin Custom Build
OutDir=.\ftequake___Win32_Release_Dedicated_Server
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLDebugQ3
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_Debug_Dedicated_ServerQ3
InputPath=..\common\math.s
InputName=math

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_aclipa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\r_aclipa.s
InputName=r_aclipa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_aliasa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\r_aliasa.s
InputName=r_aliasa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_drawa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\r_drawa.s
InputName=r_drawa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_edgea.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\r_edgea.s
InputName=r_edgea

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\r_varsa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\r_varsa.s
InputName=r_varsa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\snd_mixa.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLRelease
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# Begin Custom Build
OutDir=.\MRelease
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLDebug
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLRelease
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLDebugQ3
InputPath=..\client\snd_mixa.s
InputName=snd_mixa

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\surf16.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\surf16.s
InputName=surf16

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sw\surf8.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\sw\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\sw\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\sw\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# Begin Custom Build
OutDir=.\MRelease
InputPath=..\sw\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\sw\surf8.s
InputName=surf8

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\sys_wina.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common $(InputPath)  > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLRelease
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# Begin Custom Build
OutDir=.\MRelease
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLDebug
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLRelease
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLDebugQ3
InputPath=..\client\sys_wina.s
InputName=sys_wina

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\server\worlda.s

!IF  "$(CFG)" == "ftequake - Win32 Release"

# Begin Custom Build
OutDir=.\Release
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# Begin Custom Build
OutDir=.\Debug
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common  $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp > $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# Begin Custom Build
OutDir=.\GLDebug
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLRelease
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Intermediate_Dir ".\MDebug"
# Begin Custom Build
OutDir=.\MDebug
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# Begin Custom Build
OutDir=.\MRelease
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLDebug
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinGLRelease
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# Begin Custom Build
OutDir=.\DebugServer
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common $(InputPath) > $(OUTDIR)\$(InputName).spp 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# Begin Custom Build
OutDir=.\ftequake___Win32_Release_Dedicated_Server
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# Begin Custom Build
OutDir=.\ftequake___Win32_MinSW
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_GLDebugQ3
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# Begin Custom Build
OutDir=.\ftequake___Win32_Debug_Dedicated_ServerQ3
InputPath=..\server\worlda.s
InputName=worlda

"$(OUTDIR)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	cl /EP /I ..\common > $(OUTDIR)\$(InputName).spp $(InputPath) 
	..\gas2masm\debug\gas2masm < $(OUTDIR)\$(InputName).spp >          $(OUTDIR)\$(InputName).asm 
	ml /c /Cp /coff /Fo$(OUTDIR)\$(InputName).obj /Zm /Zi                                                                                                                                                                                                                           $(OUTDIR)\$(InputName).asm 
	del $(OUTDIR)\$(InputName).spp 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "http"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\http\ftpclient.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\http\ftpserver.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\http\httpclient.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\http\httpserver.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\irc\ircclient.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\http\iweb.h
# End Source File
# Begin Source File

SOURCE=..\http\iwebiface.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\http\webgen.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

!ENDIF 

# End Source File
# End Group
# Begin Group "vc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sndcodec\g711.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\g721.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\g723_24.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\g723_40.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\g72x.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\snd_vc.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\snd_voicecodecs.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\sndcodec\voicechat.h

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "dedicated"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\server\sv_sys_win.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# ADD CPP /Yu"qwsvdef.h"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# ADD CPP /Yu"qwsvdef.h"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# ADD BASE CPP /Yu"qwsvdef.h"
# ADD CPP /Yu"qwsvdef.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\server\svmodel.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# ADD CPP /Yu"qwsvdef.h"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# ADD CPP /Yu"qwsvdef.h"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# ADD BASE CPP /Yu"qwsvdef.h"
# ADD CPP /Yu"qwsvdef.h"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\client\winquake.rc

!IF  "$(CFG)" == "ftequake - Win32 Release"

# ADD BASE RSC /l 0x809 /i "\ftetgcvs\engine\client" /i "\windows\J\ftetgcvs\engine\client" /i "\ftetgcvs\source\client" /i "\ftetgcvs\temp\client" /i "\ftetgcvs\fte\QW\client"
# ADD RSC /l 0x809 /i "\ftetgcvs\engine\client" /i "\windows\J\ftetgcvs\engine\client" /i "\ftetgcvs\source\client" /i "\ftetgcvs\temp\client" /i "\ftetgcvs\fte\QW\client" /d "MINIMAL"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\common\bothdefs.h
# End Source File
# Begin Source File

SOURCE=..\client\client.h
# End Source File
# Begin Source File

SOURCE=..\common\cvar.h
# End Source File
# Begin Source File

SOURCE=..\sw\d_ifacea.h
# End Source File
# Begin Source File

SOURCE=..\gl\gl_model.h
# End Source File
# Begin Source File

SOURCE=..\nqnet\net_dgrm.h
# End Source File
# Begin Source File

SOURCE=..\common\protocol.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\client\bymorphed.ico
# End Source File
# Begin Source File

SOURCE=..\client\q2.ico
# End Source File
# Begin Source File

SOURCE=..\client\qwcl2.ico
# End Source File
# End Group
# Begin Group "libmad"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\mp3\libmad\bit.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\decoder.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\fixed.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\frame.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\huffman.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\layer12.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\layer3.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\client\mymad.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\stream.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\synth.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\timer.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\mp3\libmad\version.c

!IF  "$(CFG)" == "ftequake - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLRelease"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinGLRelease"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Release Dedicated Server"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 MinSW"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 GLDebugQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ftequake - Win32 Debug Dedicated ServerQ3"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
