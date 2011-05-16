/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: Generic wrappers around encoder.                              *
 *      Nasty, but avoids having to include a ton of .hpp files to use it   *
 *      Also has a PZip format wrapper which adds header to help decoding   * 
 *  CHANGELOG:
 *  Ed Wildgoose (2004): Added                                              * 
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

#include "pz_type.hpp"
#include "pz_interface.hpp"
#include "Model.hpp"

/**********************************************************************************
    Normal En/Decoder.  
    Writes the raw stream out to the PPMdIO interface of your choice
**********************************************************************************/


PZ_Encoder::PZ_Encoder()
{
    Enc = new PPMD_Encoder();
}

void PZ_Encoder::SetParams (UINT SASize, UINT MaxOrder, MR_METHOD MRMethod, BOOL Solid, char *ModelFile)
{
    ((PPMD_Encoder*)Enc)->SetParams(SASize, MaxOrder, MRMethod, Solid, ModelFile);
}

PZ_RESULT PZ_Encoder::Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile)
{
    return ((PPMD_Encoder*)Enc)->Encode(DecodedFile, EncodedFile);
}

/*Optional second param. Only needed if you want verbose stats output*/
PZ_RESULT PZ_Encoder::Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile) 
{
    return ((PPMD_Encoder*)Enc)->Flush(EncodedFile, DecodedFile);
}

void PZ_Encoder::SetVerbose(UINT Verbose) 
{
    ((PPMD_Encoder*)Enc)->Verbose = Verbose;
}

/******************************************************************************************/

PZ_Decoder::PZ_Decoder() 
{
    Dec = new PPMD_Decoder();
}

void PZ_Decoder::SetParams(UINT SASize, UINT MaxOrder, MR_METHOD MRMethod, BOOL Solid, char *ModelFile) 
{
    return ((PPMD_Decoder*)Dec)->SetParams(SASize, MaxOrder, MRMethod, Solid, ModelFile);
}

PZ_RESULT PZ_Decoder::Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    return ((PPMD_Decoder*)Dec)->Decode(EncodedFile, DecodedFile);
}

PZ_RESULT PZ_Decoder::DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    return ((PPMD_Decoder*)Dec)->DecodeBlock(EncodedFile, DecodedFile);
}

void PZ_Decoder::SetVerbose(UINT Verbose) 
{
    ((PPMD_Decoder*)Dec)->Verbose = Verbose;
}


/******************************************************************
    PZip Archive En/Decoder.  
    Writes the stream out with an initial header in pzip format
    ... again to the PPMdIO interface of your choice

    NOTE: YOU are responsible for opening any files and creating 
    the PPMdIO object.  
    Opening files is nasty and OS specific
    You do the work and then let us know your filedescriptor...
******************************************************************/

PZA_Encoder::PZA_Encoder()
{
    Enc = new PZIP_Encoder();
}

void PZA_Encoder::SetParams (UINT SASize, UINT MaxOrder, MR_METHOD MRMethod, BOOL Solid, char *ModelFile)
{
    ((PZIP_Encoder*)Enc)->SetParams(SASize, MaxOrder, MRMethod, Solid, ModelFile);
}

PZ_RESULT PZA_Encoder::Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile)
{
    return ((PZIP_Encoder*)Enc)->Encode(DecodedFile, EncodedFile);
}
PZ_RESULT PZA_Encoder::Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile) //Optional second param. Only needed if you want verbose stats output
{
    return ((PZIP_Encoder*)Enc)->Flush(EncodedFile, DecodedFile);
}

void PZA_Encoder::SetVerbose(UINT Verbose) 
{
    ((PZIP_Encoder*)Enc)->Verbose = Verbose;
}

/******************************************************************************************/


PZA_Decoder::PZA_Decoder()
{
    Dec = new PZIP_Decoder();
}

PZ_RESULT PZA_Decoder::Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    return ((PZIP_Decoder*)Dec)->Decode(EncodedFile, DecodedFile);
}

PZ_RESULT PZA_Decoder::DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    return ((PZIP_Decoder*)Dec)->DecodeBlock(EncodedFile, DecodedFile);
}

void PZA_Decoder::SetVerbose(UINT Verbose) 
{
    ((PZIP_Decoder*)Dec)->Verbose = Verbose;
}
