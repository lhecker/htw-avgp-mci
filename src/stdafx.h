#pragma once

#include "targetver.h"

// Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN

// Prevent the definition of the min()/max() macros
#define NOMINMAX

// Make CString constructors explicit
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

// Enable all of MFC's warning messages
#define _AFX_ALL_WARNINGS

// MFC core and standard components + extensions
#include <afxwin.h>
#include <afxext.h>

// Defining NOMINMAX is incompatible with GDI+ which we have to patch here
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) > (y) ? (x) : (y))
#include <gdiplus.h>
#undef min
#undef max

// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif

// MFC support for Windows Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

// MFC support for ribbons and control bars
#include <afxcontrolbars.h>

#include <mmsystem.h>

#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>

#include "utils.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
