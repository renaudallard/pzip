/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Salvador Fandiño (2003): Converted to C++                               *
 *  Ed Wildgoose (2004): Converted to library                               * 
 
 *  Copyright 1999 by Dmitry Subbotin, 2004 Ed Wildgoose                    *
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


/**********************  Original text  *************************************
////////   Carryless rangecoder (c) 1999 by Dmitry Subbotin   ////////

typedef unsigned int  uint;
typedef unsigned char uc;

#define  DO(n)     for (int _=0; _<n; _++)
#define  TOP       (1<<24)
#define  BOT       (1<<16)


class RangeCoder
{
 uint  low, code, range, passed;
 FILE  *f;

 void OutByte (uc c)           { passed++; fputc(c,f); }
 uc   InByte ()                { passed++; return fgetc(f); }

public:

 uint GetPassed ()             { return passed; }
 void StartEncode (FILE *F)    { f=F; passed=low=0;  range= (uint) -1; }
 void FinishEncode ()          { DO(4)  OutByte(low>>24), low<<=8; }
 void StartDecode (FILE *F)    { passed=low=code=0;  range= (uint) -1;
                                 f=F; DO(4) code= code<<8 | InByte();
                               }

 void Encode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq * (range/= totFreq);
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       OutByte(low>>24), range<<=8, low<<=8;
 }

 uint GetFreq (uint totFreq) {
   uint tmp= (code-low) / (range/= totFreq);
   if (tmp >= totFreq)  throw ("Input data corrupt"); // or force it to return
   return tmp;                                         // a valid value :)
 }

 void Decode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq*range;
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       code= code<<8 | InByte(), range<<=8, low<<=8;
 }
};
*****************************************************************************/


#if !defined(_CODER_H_)
#define _CODER_H_

#include "pz_io.hpp"

struct SUBRANGE {
    DWORD LowCount, HighCount, scale;
};
enum { TOP=1 << 24, BOT=1 << 15 };

class Ari {
public:
    inline Ari(): Interrupted(0), InitState(0) {};

    inline UINT GetCurrentCount();
    inline UINT GetCurrentShiftCount(UINT SHIFT);
    inline void RemoveSubrange();

    inline void EncoderInit();
    inline void EncodeSymbol();
    inline void ShiftEncodeSymbol(UINT SHIFT);
    inline INT EncoderNormalize(PZ_Out *stream);
    inline PZ_RESULT EncoderFlush(PZ_Out *stream);


    inline INT DecoderInit(PZ_In *stream);
    inline INT DecoderNormalize(PZ_In *stream);
//    inline INT DecoderNormalize_New(PZ_In *stream);

    inline UINT GetInitState() { return InitState; };

    SUBRANGE SubRange;

protected:
    DWORD low, code, range;

private:
    BOOL Interrupted;
    UINT InitState;
};


inline UINT Ari::GetCurrentCount() {
    return (code-low)/(range /= SubRange.scale);
}
inline UINT Ari::GetCurrentShiftCount(UINT SHIFT) {
    return (code-low)/(range >>= SHIFT);
}
inline void Ari::RemoveSubrange()
{
    low += range*SubRange.LowCount;
    range *= SubRange.HighCount-SubRange.LowCount;
}

inline void Ari::EncoderInit() {
    low=0;
    range=DWORD(-1);
    Interrupted=0;
}

inline INT Ari::EncoderNormalize(PZ_Out *stream) {
    while (Interrupted || (low ^ (low+range)) < TOP || range < BOT &&
	   ((range= -low & (BOT-1)),1)) 
    {
        Interrupted=0;
        if (stream->PutC((unsigned char)(low >> 24))==EOF) {
            Interrupted=1;
            return EOF;
        }
        range <<= 8;
	    low <<= 8;
    }
    return 0;
}

inline void Ari::EncodeSymbol()
{
    low += SubRange.LowCount*(range /= SubRange.scale);
    range *= SubRange.HighCount-SubRange.LowCount;
}

inline void Ari::ShiftEncodeSymbol(UINT SHIFT)
{
    low += SubRange.LowCount*(range >>= SHIFT);
    range *= SubRange.HighCount-SubRange.LowCount;
}

inline PZ_RESULT Ari::EncoderFlush(PZ_Out *stream) {
    for (UINT i=InitState;i < 4;i++) {
        if (stream->PutC(low >> 24)==EOF) {
            InitState=i;
            return P_MORE_OUTPUT;
        }
	    low <<= 8;
    }

    InitState=0;
    return P_OK;
}

inline INT Ari::DecoderInit(PZ_In *stream) {
    if (InitState==0) {
        //Else we are part initialised
        low=code=0;
        Interrupted=0;
        range=DWORD(-1);
    }

    for (UINT i=InitState;i < 4;i++) {
        int symbol = stream->GetC();
        if (symbol == EOF) {
            InitState=i;
            return P_MORE_INPUT;
        }

	    code=(code << 8) | symbol;
    }
    InitState = 0;
    return P_OK;
}

inline INT Ari::DecoderNormalize(PZ_In *stream) {
    while ( Interrupted || (low ^ (low+range)) < TOP || range < BOT &&
	   ((range= -low & (BOT-1)),1) ) 
    {
        Interrupted = 0;
        int NextChar = stream->GetC();
        if (NextChar != EOF) {
            code=(code << 8) | NextChar;
            range <<= 8;
	        low <<= 8;
        } else {
            Interrupted=1;
            return EOF;
        }
    }
    return 0;
}

#endif /* !defined(_CODER_H_) */
