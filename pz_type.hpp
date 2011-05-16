/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: compilation parameters and miscellaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Ed Wildgoose (2004): Converted to library, added pzip mode              * 
 *  Copyright 2004 Ed Wildgoose                    													*
 *                                                                         *
 *   Permission is hereby granted, free of charge, to any person obtaining *
 *   a copy of this software and associated documentation files (the       *
 *   "Software"), to deal in the Software without restriction, including   *
 *   without limitation the rights to use, copy, modify, merge, publish,   *
 *   distribute, sublicense, and/or sell copies of the Software, and to    *
 *   permit persons to whom the Software is furnished to do so, subject to *
 *   the following conditions:                                             *
 *                                                                         *
 *   The above copyright notice and this permission notice shall be        *
 *   included in all copies or substantial portions of the Software.       *
 *                                                                         *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
 *   OTHER DEALINGS IN THE SOFTWARE.                                       *
 ***************************************************************************/

#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_

#include <stdio.h>

/* Try to guess environment */
#if defined(_MSC_VER) || defined(_WIN32)
	#define _WIN32_ENVIRONMENT_
#else
	#define _POSIX_ENVIRONMENT_
#endif

/* Else set it manually */
//#define _WIN32_ENVIRONMENT_
//#define _DOS32_ENVIRONMENT_
//#define _POSIX_ENVIRONMENT_
//#define _UNKNOWN_ENVIRONMENT_
#if defined(_WIN32_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1
#error Only one environment must be defined
#endif /* defined(_WIN32_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1 */

#if defined(_WIN32_ENVIRONMENT_)
#include <windows.h>
typedef __w64 int OFST;
#else /* _DOS32_ENVIRONMENT_ || _POSIX_ENVIRONMENT_ || _UNKNOWN_ENVIRONMENT_ */
typedef int   BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
typedef signed int   INT;
typedef long OFST;
#endif /* defined(_WIN32_ENVIRONMENT_)  */

#define PPMD_FALSE 0
#define PPMD_TRUE  1

const DWORD PPMdSignature=0x84ACAF8F, Variant='J';
const char Version[] = "0.50";
const int MAX_O=16;                         /* maximum allowed model order  */

//#define _USE_PREFETCHING                    /* for puzzling mainly          */

#if !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__)
#define _FASTCALL __fastcall
#define _STDCALL  __stdcall
#else
#define _FASTCALL
#define _STDCALL
#endif /* !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__) */

#if defined(__GNUC__)
#define _PACK_ATTR __attribute__ ((packed))
#else /* "#pragma pack" is used for other compilers */
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

// Want to build a DLL?
//#define _DLL_
#if defined(_DLL_) && defined(_WIN32_ENVIRONMENT_)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif


enum MR_METHOD { MRM_RESTART=0, MRM_CUT_OFF=1, MRM_FREEZE=2 };

enum PZ_RESULT {
    P_OK = 0,
    P_MORE_INPUT = -1,
    P_MORE_OUTPUT = -2,
    P_MEM_ERROR = -3,
    P_ERROR = -4,
    P_FINISHED_BLOCK = -5,
    P_INVALID_FORMAT = -6
};

//#include "PPMdIO.hpp"

#endif /* !defined(_PPMDTYPE_H_) */
