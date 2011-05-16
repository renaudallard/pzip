/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: Generic IO routines for the en/decoder.  Feel free to         * 
 *      implement your own!                                                 *
 *      You only need to inherit from PPMD_In and provide_GetC and _Putc    *
 *      Return EOF if you can't take of provide data                        *
 *      You need only include pz_interface.hpp, pz_io.hpp and pz_type.hpp   *
 *      Plus the library itself, to use this code.                          *
 *  CHANGELOG:                                                             *
 *  Ed Wildgoose (2004): Added (based on an idea by Salvador Fandiño       * 
 *  Copyright 2004 Ed Wildgoose                                             *
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

class PZ_In {
public:
    PZ_In() :
      bytes(0) {};

    inline int GetC() {
        INT status = _GetC();
        if (status != EOF) bytes++;
        return status; 
    }

    virtual UINT BytesRead() { return bytes; };

protected:
    UINT bytes;
    inline virtual int _GetC() = 0;

};

class PZ_Out {
public:
    PZ_Out() {
        bytes = 0;
    };

    inline int PutC(BYTE c) {
        INT status = _PutC(c);
        if (status != EOF) bytes++;
        return status;
    }

    virtual UINT BytesWritten() { return bytes; };

protected:
    UINT bytes;

    inline virtual int _PutC(BYTE c) = 0;
};

class PZ_In_File : public PZ_In {
public:
    PZ_In_File(FILE *fp) :
      PZ_In(), fp(fp) {};

protected:
    inline int _GetC() {
//        static int foo=1;
//        if (foo++ < 3) { return EOF;}
//        foo=1;
        return getc(fp);
    }

private:
    FILE *fp;

};

class PZ_Out_File : public PZ_Out {
public:
    PZ_Out_File(FILE *fp) : 
      PZ_Out(), fp(fp) {};

protected:
    inline int _PutC(BYTE c) {
//        static int foo=1;
//        if (foo++ < 3) { return EOF; }
//        foo=1;
        return putc(c, fp);
    }

private:
    FILE *fp;

};



class PZ_In_ByteArray : public PZ_In  {
public:
    PZ_In_ByteArray(BYTE * const buffer, UINT len) :
    	PZ_In(), buf(buffer), buf_pos(buffer), limit(buffer+len)  {}

//    inline OFST BytesLeft() { return limit-cur; }; //Bytes left to decode
    //Next char to be de/encoded
    inline const BYTE* NextChar() {return buf_pos; }; 
    //Reset the buffer pointer & input length
    inline void Reset(UINT length=0) {buf_pos = buf; if (length) limit=buf+length;}; 
    inline BYTE const* Buffer() { return buf; };

protected:
    inline int _GetC() {
	    if (buf_pos<limit)
	        return *(buf_pos++);
        else
	        return EOF;
    }

private:
    BYTE const *buf; // Original buffer pointer
    BYTE const *buf_pos; // Next character to use as input
    BYTE const *limit; // Top of the buffer

};

class PZ_Out_ByteArray : public PZ_Out {
public:
    PZ_Out_ByteArray(BYTE *buffer, UINT len) : 
    	PZ_Out(), buf(buffer), buf_pos(buffer), limit(buffer+len) {}

//    inline OFST BytesLeft() { return buffer+size-1 - buffer_pos; };
    inline BYTE* NextChar() {return buf_pos; }; //Next output position
    inline void Reset() {buf_pos = buf;}; // Reset the buffer pointer
    inline DWORD Length() {return (DWORD)(buf_pos-buf);}; //Bytes in the buffer
    inline BYTE* Buffer() { return buf; };

protected:
    inline int _PutC(BYTE c) {
        if (buf_pos<limit) {
            //Write our new byte
            *buf_pos++ = c;
            return c; //standard putc behaviour
        } else
            return EOF;
    }

private:
    BYTE *buf; // Original buffer pointer
    BYTE *buf_pos; // Next character to write at
    BYTE const *limit; // Top of the buffer
};

#endif
