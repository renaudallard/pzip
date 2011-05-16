/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: Generic wrappers around encoder.                              *
 *      Nasty, but avoids having to include a ton of .hpp files to use it   *
 *      Also has a PZip format wrapper which adds header to help decoding   *
 *      You need only include pz_interface.hpp, pz_io.hpp and pz_type.hpp   *
 *      Plus the library itself, to use this code.
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
#include "pz_io.hpp"

/******************************************************************
    Normal En/Decoder.  
    Writes the raw stream out to the PPMdIO interface of your choice
******************************************************************/

class PZ_Encoder {
public:
    PZ_Encoder();

    // Set encoding params.  
    // Should only be done at the start of a block, not mid encoding
    void SetParams (UINT SASize=10, UINT MaxOrder=8, MR_METHOD MRMethod=MRM_RESTART, BOOL Solid=1, char *ModelFile=NULL);

    // Encode from PZ_In to PZ_Out until either one returns EOF
    // Returns P_OK if input completely compressed or P_MORE_OUTPUT
    PZ_RESULT Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile);

    // Flush remaining bytes in the encoder.  
    // Will finish and flush the block automatically in the decoder
    // Returns P_MORE_OUTPUT if we ran out of space (fix this and recall Flush)
    // Or P_OK if everything went OK.
    // Flush: Optional second param. Only needed if you want verbose stats output
    PZ_RESULT Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile = NULL); //Optional second param. Only needed if you want verbose stats output

    // Generates verbose progress and ratio info to stdout
    void SetVerbose(UINT Verbose);

private:
    void *Enc;
};

class PZ_Decoder {
public:
    PZ_Decoder();

    // Set decoding params.  
    // Should only be done at the start of a block, not mid encoding
    void SetParams (UINT SASize=10, UINT MaxOrder=8, MR_METHOD MRMethod=MRM_RESTART, BOOL Solid=1, char *ModelFile=NULL);

    // Decode from PZ_In to PZ_Out until either one returns EOF or we finish a block
    // Returns  P_MORE_INPUT if we didn't finish the block yet
    //          P_FINISHED_BLOCK if we decoded that block completely
    //          P_MORE_OUTPUT if the output buffer is full
    //          P_OK if we are called on a new block, but there was no more input!
    PZ_RESULT DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    // Calls DecodeBlock repeatedly until we run out of input (or Output space)
    // Returns: P_OK If we finish a block and there is no more input
    //          P_MORE_INPUT if we haven't finished a block yet
    //          P_MORE_OUTPUT if we ran out of output space
    PZ_RESULT Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    void SetVerbose(UINT Verbose);

private:
    void *Dec;
};



/******************************************************************
    PZip Archive En/Decoder.  
    Writes the stream out with an initial header in pzip format
    ... again to the PPMdIO interface of your choice

    NOTE: YOU are responsible for opening any files and creating 
    the PPMdIO object.  
    Opening files is nasty and OS specific
    You do the work and then let us know your filedescriptor...
******************************************************************/

class PZA_Encoder {
public:
    PZA_Encoder();

    // As per PZ_Encoder::SetParams
    void SetParams (UINT SASize=10, UINT MaxOrder=8, MR_METHOD MRMethod=MRM_RESTART, BOOL Solid=1, char *ModelFile=NULL);

    // Encode and add a header to the block with decomp params
    // Auto flushes when the input is empty
    // Returns P_OK on success
    PZ_RESULT Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile);

    // As per PZ_Encoder::Flush
    PZ_RESULT Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile = NULL); //Optional second param. Only needed if you want verbose stats output

    // As per PZ_Encoder::SetVerbose
    void SetVerbose(UINT Verbose);

private:
    void *Enc;
};

class PZA_Decoder {
public:
    PZA_Decoder();

    // NOTE: No SetParams call.  Params are embeded in the compressed file

    // Decodes one block from PZ_In.  Reads the params from the input
    // Returns  P_MORE_INPUT if we didn't finish the block yet
    //          P_FINISHED_BLOCK if we decoded that block completely
    //          P_MORE_OUTPUT if the output buffer is full
    PZ_RESULT DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    // Calls DecodeBlock repeatedly
    // Returns P_OK on success
    PZ_RESULT Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile); 

    // As per PZ_Decoder::SetVerbose
    void SetVerbose(UINT Verbose);

private:
    void *Dec;
};
