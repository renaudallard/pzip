/****************************************************************************
 *  This file is part of PZip project                                      *
 *  Contents: Perl wrappers around encoder.                              	 *
 *  CHANGELOG:
 *  Ed Wildgoose (2004): Added                                             * 
 *  Copyright 2004 Ed Wildgoose                                            *
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

#if !defined(__INCLUDE_PPMD_BUFFER_HPP__)
#define __INCLUDE_PPMD_BUFFER_HPP__

#include "Exception.hpp"

class PZ_In_Perl : public PZ_In {
public:
    PZ_In_Perl(BYTE * const string, unsigned int len) :
	str(string), limit(string+len), die(d) {}

    PZ_In_Perl(SV *sv)
    {
	    STRLEN len;
	    str=(BYTE *)SvPV(sv, len);
	    limit=str+len;
    }

    inline int GetC() {
	    if (str<limit) {
	        return *(str++);
	    }
	    return EOF;
    }

private:
    BYTE const * str;
    BYTE const * limit;
    int die;
};

class PZ_Out_Perl : public PZ_Out  {
public:
    PZ_Out_Perl(SV *buffer) : 
	    sv(buffer) {}
    inline void PutC(BYTE c) {
	    sv_catpvn(sv, (char *)&c, 1);
    }

private:
    SV *sv;
};

#endif
