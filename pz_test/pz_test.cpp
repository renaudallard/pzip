/***************************************************************************
 *   Copyright (C) 2004 by Ed Wildgoose                                    *
 *   edward@wildgooses.com                                                 *
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

#include <iostream>
#include <fstream>


#include "pz_interface.hpp"
#include "pz_io.hpp"
#include "pz_type.hpp"

using namespace std;


unsigned int SASIZE=1, MAXORDER=8;
MR_METHOD MRM = MRM_CUT_OFF;

main ()
{
    char compressMe[] = "This is some test text to compress.  I wonder what compression ratio we will get on this text.";
    char compressed[65536];

    cerr << "Encoding string:\n" << compressMe << "\n\n";

    // Now lets fire up the compressor
    // It's not a one liner, but then it's OO and we are expecting to reuse the object...
    PZ_Encoder myEncoder;
    myEncoder.SetParams(SASIZE, MAXORDER, MRM, 1, NULL);
    // Create the In and Out IO objects
    PZ_In_ByteArray c_in_buff( (BYTE*)compressMe, (UINT)strlen(compressMe) );
    // Could also set input size with: c_in_buff.Reset( strlen(compressMe) );
    PZ_Out_ByteArray c_out_buff( (BYTE*)compressed, 65535); 

    // OK Now lets encode
    PZ_RESULT res;
    res=myEncoder.Encode(&c_in_buff, &c_out_buff);
    // Check for any probs, eg ran out of output buffer space
    if (res != P_OK) { fprintf(stderr, "Error during encoding!" ); exit(-1); }
    // Flush buffer
    res=myEncoder.Flush(&c_out_buff);
    // Check for any probs, eg ran out of output buffer space
    if (res != P_OK) { cerr << "Error during encoding!"; exit(-1); }



    cout << "Original String Len: " << (long)strlen(compressMe) << " bytes\n";
    cout << "Compressed string to: " << c_out_buff.BytesWritten() << " bytes\n\n";

    // Now lets test the decoder by decoding just a single byte at a time, with a one byte output space
    // The point is to show that you can set arbitrary size input and output buffers
    // for decoding (and encoding)
    char ibuff, obuff;
    PZ_Decoder myDecoder;
    myDecoder.SetParams(SASIZE, MAXORDER, MRM, 1, NULL);
    // Create the In and Out IO objects
    PZ_In_ByteArray d_in_buff( (BYTE*)&ibuff, 1);
    // Could also set input size with: c_in_buff.Reset (strlen(compressMe));
    PZ_Out_ByteArray d_out_buff( (BYTE*)&obuff, 1); 


    // We are going to be a bit fancy and feed in each compressed symbol one by one
    // And then drain down the decoder, so we can see the bytes coming out in spurts 
    // ie. one compressed symbol can be zero or lots of decoded symbols
    char *next_symbol = compressed;
    while ( next_symbol < (char*)c_out_buff.NextChar() ) 
    {
        // Copy next symbol to decompress into the input buffer
        d_in_buff.Reset();
        ibuff = *next_symbol;
        next_symbol++;

        cout << "Decode symbol: ";
        do {
            res = myDecoder.Decode(&d_in_buff, &d_out_buff);
            if (d_out_buff.Length()) // Note .BytesWritten() is all bytes, .Length() is bytes in buffer!
                cout << obuff;
            d_out_buff.Reset();
        } while (res == P_MORE_OUTPUT);
        cout << "\n";

    }

    // Assuming everything went OK, then the last call to decode should 
    // return P_OK if we decompressed everything and found the end of a block
    // Otherwise something went wrong because we really should be finished now!
    if (res != P_OK) { fprintf(stderr, "Error during encoding!" ); exit(-1); }
}