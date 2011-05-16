/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: Main encoder. See pz_interface for easy way to use it         *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Salvador Fandiño (2003): Converted to C++                               *
 *  Ed Wildgoose (2004): Converted to library, added pzip mode              * 
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

#if !defined(_MODEL_H_)
#define _MODEL_H_

#include <time.h>

#include "pz_io.hpp"
#include "Coder.hpp"
#include "SubAlloc.hpp"

class PPMD_Stream;
class PPMD_Encoder;
class PPMD_Decoder;

enum { UP_FREQ=5, INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
    INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124, O_BOUND=9 };

#pragma pack(1)
class SEE2_CONTEXT { // SEE-contexts for PPM-contexts with masked symbols
public:
    WORD Summ;
    BYTE Shift, Count;
    inline void init(UINT InitVal);
    inline UINT getMean();
    inline void update();
} _PACK_ATTR;

class PPM_CONTEXT {                         // Notes:
public:
    BYTE NumStats, Flags;                   // 1. NumStats & NumMasked contain
    WORD SummFreq;                          //  number of symbols minus 1
    struct STATE {                          // 2. sizeof(WORD) > sizeof(BYTE)
        BYTE Symbol, Freq;                  // 3. contexts example:
        PPM_CONTEXT* Successor;             // MaxOrder:
    } _PACK_ATTR * Stats;                   //  ABCD    context
    PPM_CONTEXT* Suffix;                    //   BCD    suffix

    inline void encodeBinSymbol( int symbol,
				 PPMD_Encoder &stream);//   BCDE   successor
    inline void encodeSymbol1(int symbol, PPMD_Encoder &stream);// other orders:
    inline void encodeSymbol2(int symbol, PPMD_Encoder &stream);//   BCD    context
    inline void decodeBinSymbol(PPMD_Decoder &stream);//    CD    suffix
    inline void decodeSymbol1(PPMD_Decoder &stream);//   BCDE   successor
    inline void decodeSymbol2(PPMD_Decoder &stream);
    inline void update1(STATE* p, PPMD_Stream &stream);
    inline void update2(STATE* p, PPMD_Stream &stream);
    inline SEE2_CONTEXT* makeEscFreq2(PPMD_Stream &stream);
    void rescale(PPMD_Stream &stream);
    void refresh(int OldNU,BOOL Scale, PPMD_Stream &stream);
    PPM_CONTEXT* cutOff(int Order, PPMD_Stream &stream);
    PPM_CONTEXT* cutOff(int o,int ob,double b, PPMD_Stream &stream); //iterative pruning to shrink model
    PPM_CONTEXT* removeBinConts(int Order, PPMD_Stream &stream);
    void readModel(FILE* fp,UINT PrevSym, PPMD_Stream &stream);
    void makeSuffix();
    void writeModel(int o,FILE* fp1,FILE* fp2,FILE* fp3);
    void writeModel(int o,FILE* fp);

    STATE& oneState() const { return (STATE&) SummFreq; }
} _PACK_ATTR;
#pragma pack()

class PPMD_Stream {
    friend class PPM_CONTEXT;
public:
    PPMD_Stream();
    ~PPMD_Stream();

    inline DWORD GetUsedMemory() {return Memory.GetUsedMemory();} ;
    void PrintInfo(PZ_In* InFile,PZ_Out* OutFile,BOOL EncodeFlag);

private:
    SEE2_CONTEXT SEE2Cont[24][32], DummySEE2Cont;
    int  InitEsc, RunLength, InitRL, MaxOrder;
    BYTE CharMask[256], PrevSuccess; //, PrintCount;
    WORD BinSumm[25][64]; // binary SEE-contexts
    MR_METHOD MRMethod;

protected:
    BOOL StartSubAllocator(UINT SASize);
    void StopSubAllocator();

    void StartModelRare(int MaxOrder,MR_METHOD MRMethod);
    PPM_CONTEXT* ReduceOrder(PPM_CONTEXT::STATE* p,PPM_CONTEXT* pc);
    void RestoreModelRare(PPM_CONTEXT* pc1,
			  PPM_CONTEXT* MinContext,
			  PPM_CONTEXT* FSuccessor);
    PPM_CONTEXT *CreateSuccessors(BOOL Skip,
				  PPM_CONTEXT::STATE* p,
				  PPM_CONTEXT* pc);
    inline void UpdateModel(PPM_CONTEXT* MinContext);
    inline void ClearMask();

    PPM_CONTEXT::STATE* FoundState;      // found next state transition
    struct PPM_CONTEXT* MaxContext;
    PPM_CONTEXT* MinContext; //Used to track current state
    int OrderFall;
    BYTE NumMasked, EscCount;
    SubAlloc Memory;
    Ari ari;

    char *model_file; //File from which to load context model
    clock_t StartClock;
};

///////////////////////////////////////////////////////////////////////////////////////

class PPMD_Encoder : public PPMD_Stream {
public:
    PPMD_Encoder()
        : PPMD_Stream(), Initialised(0), SASize(10), MaxOrder(8), NewMaxOrder(8), MRMethod(MRM_RESTART), 
        Solid(1), NeedMoreOutput(0), FinishedBlock(0), Verbose(0), OneInTheSpout(-1), stats_count(1) {};

    // Set encoding params.  
    // Should only be done at the start of a block, not mid encoding
    void SetParams (UINT SASize=10, UINT MaxOrder=8, 
            MR_METHOD MRMethod=MRM_RESTART, BOOL Solid=1, char *ModelFile=NULL);

    // Encode from PZ_In to PZ_Out until either one returns EOF
    // Returns P_OK if input completely compressed or P_MORE_OUTPUT
    PZ_RESULT Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile);

    // Flush remaining bytes in the encoder.  
    // Will finish and flush the block automatically in the decoder
    // Returns P_MORE_OUTPUT if we ran out of space (fix this and recall Flush)
    // Or P_OK if everything went OK.
    // Flush: Optional second param. Only needed if you want verbose stats output
    PZ_RESULT Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile = NULL); 

    UINT Verbose;

protected:
    BOOL Initialised;
    UINT SASize;
    UINT MaxOrder, NewMaxOrder;
    MR_METHOD MRMethod;
    BOOL Solid;

private:
    // TODO: These next two should be turned into some kind of state variable
    BOOL NeedMoreOutput; //used to track when we are still waiting on more output to finish decoding
    BOOL FinishedBlock; //Used to track when we are flushing data
    INT OneInTheSpout; //Overflow for when we run out of output space
    UINT stats_count;

    void EncodeInit();
    inline PZ_RESULT EncodeOneSymbol(PZ_Out* EncodedFile, int symbol);
};


class PPMD_Decoder : public PPMD_Stream {
public:
    PPMD_Decoder() 
        : PPMD_Stream(), NeedMoreInput(0), OneInTheSpout(-1), SASize(10),
          Initialised(0), MaxOrder(8), NewMaxOrder(8), MRMethod(MRM_RESTART), 
          Solid(1), stats_count(1), Verbose(0) {};

    // Set Decoding params.  
    // Should only be done at the start of a block, not mid encoding
    void SetParams (UINT SASize=10, UINT MaxOrder=8, MR_METHOD MRMethod=MRM_RESTART, BOOL Solid=1, char *ModelFile=NULL);

    // Decode from PZ_In to PZ_Out until either one returns EOF or we finish a block
    // Returns  P_MORE_INPUT if we didn't finish the block yet
    //          P_FINISHED_BLOCK if we decoded that block completely
    //          P_MORE_OUTPUT if the output buffer is full
    PZ_RESULT DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    // Calls DecodeBlock repeatedly until we run out of input (or Output space)
    // Returns: P_OK If we finish a block and there is no more input
    //          P_MORE_INPUT if we haven't finished a block yet
    //          P_MORE_OUTPUT if we ran out of output space
    PZ_RESULT Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    UINT Verbose;

protected:
    BOOL Initialised;
    UINT SASize;
    UINT MaxOrder, NewMaxOrder;
    MR_METHOD MRMethod;
    BOOL Solid;

private:
    BOOL NeedMoreInput; //used to track when we are still waiting on more input to finish decoding
    INT OneInTheSpout; //Overflow for when we run out of output space
    UINT stats_count;

    PZ_RESULT DecodeInit(PZ_In* EncodedFile);
    inline INT DecodeOneSymbol(PZ_In* EncodedFile);
};


///////////////////////////////////////////////////////////////////////////////////////

// Our PZIP header format
struct ARC_INFO { // FileLength & CRC? Hmm, maybe in another times...
    DWORD signature;
    WORD  size;
    BYTE info;
} _PACK_ATTR;


class PZIP_Encoder : public PPMD_Encoder {
public:
    // Encode and add a header to the block with decomp params
    // Auto flushes when the input is empty
    // Returns P_OK on success
    PZ_RESULT Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile);
};

class PZIP_Decoder : public PPMD_Decoder {
public:
    // Decodes one block from PZ_In.  Reads the params from the input
    // Returns  P_MORE_INPUT if we didn't finish the block yet
    //          P_FINISHED_BLOCK if we decoded that block completely
    //          P_MORE_OUTPUT if the output buffer is full
    PZ_RESULT DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile);

    // Calls DecodeBlock repeatedly
    // Returns P_OK on success
    PZ_RESULT Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile);
};

#endif
