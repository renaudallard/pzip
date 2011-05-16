/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: PPMII model description and encoding/decoding routines        *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Salvador Fandiño (2003): Converted to C++ by:                           *
 *  Ed Wildgoose (2004): Converted to library, and added model loading     * 
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


#include <string.h>
#include "pz_type.hpp"
#include "Model.hpp"
#include "Exception.hpp"

//#include "SubAlloc_impl.hpp"

inline void SEE2_CONTEXT::init(UINT InitVal) {
    Summ=InitVal << (Shift=PERIOD_BITS-4);
    Count=7;
}

inline UINT SEE2_CONTEXT::getMean() {
    UINT RetVal=(Summ >> Shift);
    Summ -= RetVal;
    return RetVal+(RetVal == 0);
}
inline void SEE2_CONTEXT::update() {
    if (Shift < PERIOD_BITS && --Count == 0) {
    	Summ += Summ;
    	Count=3 << Shift++;
    }
}


struct PPMD_STARTUP { inline PPMD_STARTUP(); } PPMd_StartUp;
inline PPMD_STARTUP::PPMD_STARTUP()         // constants initialization
{
    UINT i, k, m, Step;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
    NS2BSIndx[0]=2*0;                       NS2BSIndx[1]=2*1;
    memset(NS2BSIndx+2,2*2,9);              memset(NS2BSIndx+11,2*3,256-11);
    for (i=0;i < UP_FREQ;i++)               QTable[i]=i;
    for (m=i=UP_FREQ, k=Step=1;i < 260;i++) {
        QTable[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
}


inline void StateCpy(PPM_CONTEXT::STATE& s1,const PPM_CONTEXT::STATE& s2)
{
    (WORD&) s1=(WORD&) s2;                  s1.Successor=s2.Successor;
}

inline void SWAP(PPM_CONTEXT::STATE& s1,PPM_CONTEXT::STATE& s2)
{
    WORD t1=(WORD&) s1;                     PPM_CONTEXT* t2=s1.Successor;
    (WORD&) s1 = (WORD&) s2;                s1.Successor=s2.Successor;
    (WORD&) s2 = t1;                        s2.Successor=t2;
}

// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16]={ 25, 14, 9, 7, 5, 5, 4, 4,
				  4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))

inline void PPM_CONTEXT::encodeBinSymbol(int symbol, PPMD_Encoder &encoder)
{
    BYTE indx=NS2BSIndx[Suffix->NumStats]+encoder.PrevSuccess+Flags;
    STATE& rs=oneState();
    WORD& bs=encoder.BinSumm[QTable[rs.Freq-1]]
	[indx+((encoder.RunLength >> 26) & 0x20)];
    if (rs.Symbol == symbol) {
        encoder.FoundState=&rs;
	    rs.Freq += (rs.Freq < 196);
        encoder.ari.SubRange.LowCount=0;
    	encoder.ari.SubRange.HighCount=bs;
        bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        encoder.PrevSuccess=1;
	    encoder.RunLength++;
    } else {
        encoder.ari.SubRange.LowCount=bs;
	    bs -= GET_MEAN(bs,PERIOD_BITS,2);
        encoder.ari.SubRange.HighCount=BIN_SCALE;
	    encoder.InitEsc=ExpEscape[bs >> 10];
        encoder.CharMask[rs.Symbol]=encoder.EscCount;
        encoder.NumMasked=encoder.PrevSuccess=0;
	    encoder.FoundState=NULL;
    }
}
inline void PPM_CONTEXT::decodeBinSymbol(PPMD_Decoder &decoder)
{
    BYTE indx=NS2BSIndx[Suffix->NumStats]+decoder.PrevSuccess+Flags;
    STATE& rs=oneState();
    WORD& bs=decoder.BinSumm[QTable[rs.Freq-1]]
	[indx+((decoder.RunLength >> 26) & 0x20)];
    if (decoder.ari.GetCurrentShiftCount(TOT_BITS) < bs) {
        decoder.FoundState=&rs;
	    rs.Freq += (rs.Freq < 196);
        decoder.ari.SubRange.LowCount=0;
	    decoder.ari.SubRange.HighCount=bs;
        bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        decoder.PrevSuccess=1;
    	decoder.RunLength++;
    } else {
        decoder.ari.SubRange.LowCount=bs;
	    bs -= GET_MEAN(bs,PERIOD_BITS,2);
        decoder.ari.SubRange.HighCount=BIN_SCALE;
	    decoder.InitEsc=ExpEscape[bs >> 10];
        decoder.CharMask[rs.Symbol]=decoder.EscCount;
        decoder.NumMasked=decoder.PrevSuccess=0;
    	decoder.FoundState=NULL;
    }
}

inline void PPM_CONTEXT::update1(STATE* p, PPMD_Stream &stream)
{
    (stream.FoundState=p)->Freq += 4;
    SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) {
        SWAP(p[0],p[-1]);
	    stream.FoundState=--p;
        if (p->Freq > MAX_FREQ)
	        rescale( stream );
    }
}

inline void PPM_CONTEXT::encodeSymbol1(int symbol, PPMD_Encoder &encoder)
{
    UINT LoCnt, i=Stats->Symbol;
    STATE* p=Stats;
    encoder.ari.SubRange.scale=SummFreq;
    if (i == symbol) {
        encoder.PrevSuccess=(2*(encoder.ari.SubRange.HighCount=p->Freq)
			     >= encoder.ari.SubRange.scale);
        (encoder.FoundState=p)->Freq += 4;
	    SummFreq += 4;
        encoder.RunLength += encoder.PrevSuccess;
        if (p->Freq > MAX_FREQ)
	        rescale(encoder);
        encoder.ari.SubRange.LowCount=0;
	    return;
    }
    LoCnt=p->Freq;
    i=NumStats;
    encoder.PrevSuccess=0;
    while ((++p)->Symbol != symbol) {
        LoCnt += p->Freq;
        if (--i == 0) {
            if ( Suffix )
        		PrefetchData(Suffix);
            encoder.ari.SubRange.LowCount=LoCnt;
	        encoder.CharMask[p->Symbol]=encoder.EscCount;
            i=encoder.NumMasked=NumStats;
	        encoder.FoundState=NULL;
            do {
		        encoder.CharMask[(--p)->Symbol]=encoder.EscCount;
	        } while ( --i );
            encoder.ari.SubRange.HighCount=encoder.ari.SubRange.scale;
            return;
        }
    }
    encoder.ari.SubRange.HighCount =
	(encoder.ari.SubRange.LowCount=LoCnt)+p->Freq;
    update1(p, encoder);
}
inline void PPM_CONTEXT::decodeSymbol1(PPMD_Decoder &decoder)
{
    UINT i, count, HiCnt=Stats->Freq;
    STATE* p=Stats;
    decoder.ari.SubRange.scale=SummFreq;
    if ((count=decoder.ari.GetCurrentCount()) < HiCnt) {
        decoder.PrevSuccess=(2*(decoder.ari.SubRange.HighCount=HiCnt)
			     >= decoder.ari.SubRange.scale);
        (decoder.FoundState=p)->Freq=(HiCnt += 4);
	    SummFreq += 4;
        decoder.RunLength += decoder.PrevSuccess;
        if (HiCnt > MAX_FREQ)
	        rescale(decoder);
        decoder.ari.SubRange.LowCount=0;
	    return;
    }
    i=NumStats;
    decoder.PrevSuccess=0;
    while ((HiCnt += (++p)->Freq) <= count)
        if (--i == 0) {
            if ( Suffix )
		        PrefetchData(Suffix);
            decoder.ari.SubRange.LowCount=HiCnt;
	        decoder.CharMask[p->Symbol]=decoder.EscCount;
            i=decoder.NumMasked=NumStats;
	        decoder.FoundState=NULL;
            do {
	        	decoder.CharMask[(--p)->Symbol]=decoder.EscCount;
	        } while ( --i );
            decoder.ari.SubRange.HighCount=decoder.ari.SubRange.scale;
            return;
        }
    decoder.ari.SubRange.LowCount =
	    (decoder.ari.SubRange.HighCount=HiCnt)-p->Freq;
    update1(p, decoder);
}
inline void PPM_CONTEXT::update2(STATE* p, PPMD_Stream &stream)
{
    (stream.FoundState=p)->Freq += 4;
    SummFreq += 4;
    if (p->Freq > MAX_FREQ)
    	rescale(stream);
    stream.EscCount++;
    stream.RunLength=stream.InitRL;
}
inline SEE2_CONTEXT* PPM_CONTEXT::makeEscFreq2(PPMD_Stream &stream)
{
    BYTE* pb=(BYTE*) Stats;
    UINT t=2*NumStats;
    PrefetchData(pb);
    PrefetchData(pb+t);
    PrefetchData(pb += 2*t);
    PrefetchData(pb+t);
    SEE2_CONTEXT* psee2c;
    if (NumStats != 0xFF) {
        t=Suffix->NumStats;
        psee2c=stream.SEE2Cont[QTable[NumStats+2]-3]+(SummFreq > 11*(NumStats+1));
        psee2c += 2*(2*NumStats < t+stream.NumMasked)+Flags;
        stream.ari.SubRange.scale=psee2c->getMean();
    } else {
        psee2c=&(stream.DummySEE2Cont);
        stream.ari.SubRange.scale=1;
    }
    return psee2c;
}
inline void PPM_CONTEXT::encodeSymbol2(int symbol, PPMD_Encoder &encoder)
{
    SEE2_CONTEXT* psee2c=makeEscFreq2(encoder);
    UINT Sym, LoCnt=0, i=NumStats-encoder.NumMasked;
    STATE* p1, * p=Stats-1;
    do {
        do {
	        Sym=(++p)->Symbol;
	    } while (encoder.CharMask[Sym] == encoder.EscCount);
        encoder.CharMask[Sym]=encoder.EscCount;
        if (Sym == symbol)
	    goto SYMBOL_FOUND;
        LoCnt += p->Freq;
    } while ( --i );
    encoder.ari.SubRange.HighCount= (encoder.ari.SubRange.scale +=
				     (encoder.ari.SubRange.LowCount=LoCnt));
    psee2c->Summ += encoder.ari.SubRange.scale;
    encoder.NumMasked = NumStats;
    return;
SYMBOL_FOUND:
    encoder.ari.SubRange.LowCount=LoCnt;
    encoder.ari.SubRange.HighCount=(LoCnt+=p->Freq);
    for (p1=p; --i ; ) {
        do {
	        Sym=(++p1)->Symbol;
	    } while (encoder.CharMask[Sym] == encoder.EscCount);
        LoCnt += p1->Freq;
    }
    encoder.ari.SubRange.scale += LoCnt;
    psee2c->update();
    update2(p, encoder);
}
inline void PPM_CONTEXT::decodeSymbol2(PPMD_Decoder &decoder)
{
    SEE2_CONTEXT* psee2c=makeEscFreq2(decoder);
    UINT Sym, count, HiCnt=0, i=NumStats-decoder.NumMasked;
    STATE* ps[256], ** pps=ps, * p=Stats-1;
    do {
        do {
	        Sym=(++p)->Symbol;
	    } while (decoder.CharMask[Sym] == decoder.EscCount);
        HiCnt += p->Freq;
	    *pps++ = p;
    } while ( --i );
    decoder.ari.SubRange.scale += HiCnt;
    count=decoder.ari.GetCurrentCount();
    p=*(pps=ps);
    if (count < HiCnt) {
        HiCnt=0;
        while ((HiCnt += p->Freq) <= count)
	    p=*++pps;
        decoder.ari.SubRange.LowCount =
	    (decoder.ari.SubRange.HighCount=HiCnt)-p->Freq;
        psee2c->update();
	    update2(p, decoder);
    } else {
        decoder.ari.SubRange.LowCount=HiCnt;
	    decoder.ari.SubRange.HighCount=decoder.ari.SubRange.scale;
        i=NumStats-decoder.NumMasked;
	    decoder.NumMasked = NumStats;
        do {
	    decoder.CharMask[(*pps)->Symbol]=decoder.EscCount; pps++;
	} while ( --i );
        psee2c->Summ += decoder.ari.SubRange.scale;
    }
}

void PPM_CONTEXT::refresh(int OldNU,BOOL Scale, PPMD_Stream &stream)
{
    int i=NumStats, EscFreq;
    STATE* p = Stats = (STATE*) stream.Memory.ShrinkUnits(Stats,OldNU,(i+2) >> 1);
    Flags=(Flags & (0x10+0x04*Scale))+0x08*(p->Symbol >= 0x40);
    EscFreq=SummFreq-p->Freq;
    SummFreq = (p->Freq=(p->Freq+Scale) >> Scale);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Scale) >> Scale);
        Flags |= 0x08*(p->Symbol >= 0x40);
    } while ( --i );
    SummFreq += (EscFreq=(EscFreq+Scale) >> Scale);
}
#define P_CALL(F) ( PrefetchData(p->Successor), \
                    p->Successor=p->Successor->F(Order+1))
#define P_CALL2(F, o2) ( PrefetchData(p->Successor), \
                         p->Successor=p->Successor->F(Order+1, o2))
PPM_CONTEXT* PPM_CONTEXT::cutOff(int Order, PPMD_Stream &stream)
{
    int i, tmp;
    STATE* p;
    if ( !NumStats ) {
        if ((BYTE*) (p=&oneState())->Successor >= stream.Memory.UnitsStart) {
            if (Order < stream.MaxOrder)
		        P_CALL2(cutOff, stream);
            else
		        p->Successor=NULL;
            if (!p->Successor && Order > O_BOUND)
		        goto REMOVE;
            return this;
        } else {
	REMOVE:
	    stream.Memory.SpecialFreeUnit(this);
	    return NULL;
        }
    }
    PrefetchData(Stats);
    Stats = (STATE*) stream.Memory.MoveUnitsUp(Stats,tmp=(NumStats+2) >> 1);
    for (p=Stats+(i=NumStats);p >= Stats;p--)
	if ((BYTE*) p->Successor < stream.Memory.UnitsStart) {
	    p->Successor=NULL;
	    SWAP(*p,Stats[i--]);
	} else if (Order < stream.MaxOrder)
	    P_CALL2(cutOff, stream);
	else
	    p->Successor=NULL;
    if (i != NumStats && Order) {
        NumStats=i;
	    p=Stats;
        if (i < 0) { 
            stream.Memory.FreeUnits(p,tmp);
	        goto REMOVE; }
        else if (i == 0) {
            Flags=(Flags & 0x10)+0x08*(p->Symbol >= 0x40);
            StateCpy(oneState(),*p);
	        stream.Memory.FreeUnits(p,tmp);
            oneState().Freq=(oneState().Freq+11) >> 3;
        } else
	        refresh(tmp,SummFreq > 16*i, stream);
    }
    return this;
}

/*
// Cutoff used by ShrinkModel.  Seems to reduce model iteratively to achieve required size/order
PPM_CONTEXT* PPM_CONTEXT::cutOff(int o,int ob,double b, PPMD_Stream &stream)
{
    STATE tmp, * p, * p1;
    int ns=NumStats;
    double sf0, sf1, ExtraBits;
    if (o < ob) {
        if ( !ns ) {
            if ( (p=&oneState())->Successor )
                    p->Successor=p->Successor->cutOff(o+1,ob,b, stream);
        } else
            for (p=Stats+ns;p >= Stats;p--)
                if ( p->Successor )         p->Successor=p->Successor->cutOff(o+1,ob,b, stream);
        return this;
    } else if ( !ns ) {
        p=&oneState();
        if ( !Suffix->NumStats ) {
            sf0=(p1=&(Suffix->oneState()))->Freq+(sf1=Suffix->EscFreq);
        } else {
            for (p1=Suffix->Stats;p->Symbol != p1->Symbol;p1++)
                    ;
            sf0=Suffix->SummFreq+Suffix->EscFreq;
            sf1=sf0-p1->Freq;
        }
        if (!p->Successor && !p->Flag) {
            ExtraBits=GetExtraBits(p->Freq,EscFreq,p1->Freq,sf1,sf0,0);
            if (ExtraBits < b || 4*p->Freq < EscFreq) {
                if ( !Suffix->NumStats )    Suffix->oneState().Freq += p->Freq;
                else { p1->Freq += p->Freq; Suffix->SummFreq += p->Freq; }
                return NULL;
            }
        }
        nc++;                               SizeOfModel += 3;
        p->Flag=0;                          p1->Flag=1;
        return this;
    }
    for (p=Stats+ns;p >= Stats;p--)         CharMask[p->Symbol]=1;
    sf0=Suffix->SummFreq+(sf1=Suffix->EscFreq);
    for (p1=Suffix->Stats+Suffix->NumStats;p1 >= Suffix->Stats;p1--)
            if ( !CharMask[p1->Symbol] )    sf1 += p1->Freq;
    for (p=Stats+ns;p >= Stats;p--) {
        CharMask[p->Symbol]=0;
        if (p->Flag || p->Successor)        continue;
        for (p1=Suffix->Stats;p->Symbol != p1->Symbol;p1++)
                ;
        ExtraBits=GetExtraBits(p->Freq,EscFreq,p1->Freq,sf1,sf0,ns);
        if (ExtraBits < b || 64*p->Freq*(ns+1) < SummFreq) {
            SummFreq -= p->Freq;            EscFreq += p->Freq;
            p1->Freq += p->Freq;            Suffix->SummFreq += p->Freq;
            sf1 += p1->Freq;                sf0 += p->Freq;
            for (p1=p;p1 != Stats+ns;p1++)  SWAP(p1[0],p1[1]);
            p=Stats+ns--;
        }
    }
    for (p=Stats+ns;p >= Stats;p--) {
        for (p1=Suffix->Stats;p->Symbol != p1->Symbol;p1++)
                ;
        p->Flag=0;                          p1->Flag=1;
    }
    NumStats=ns;
    if (ns < 0)                             return NULL;
    SizeOfModel += 1+2*(ns+1);              nc++;
    if (ns == 0) { tmp=Stats[0];            oneState()=tmp; }
    return this;
}*/

PPM_CONTEXT* PPM_CONTEXT::removeBinConts(int Order, PPMD_Stream &stream)
{
    STATE* p;
    if ( !NumStats ) {
        p=&oneState();
        if ((BYTE*) p->Successor >= stream.Memory.UnitsStart
	    && Order < stream.MaxOrder)
                P_CALL2(removeBinConts, stream);
        else
	    p->Successor=NULL;
        if (!p->Successor && (!Suffix->NumStats || Suffix->Flags == 0xFF)) {
            stream.Memory.FreeUnits(this,1);
	    return NULL;
        } else
	    return this;
    }
    PrefetchData(Stats);
    for (p=Stats+NumStats;p >= Stats;p--)
	if ((BYTE*) p->Successor >= stream.Memory.UnitsStart
	    && Order < stream.MaxOrder)
	    P_CALL2(removeBinConts, stream);
	else
	    p->Successor=NULL;
    return this;
}

void PPM_CONTEXT::rescale(PPMD_Stream &stream)
{
    UINT OldNU, Adder, EscFreq, i=NumStats;
    STATE tmp, * p1, * p;
    for (p=stream.FoundState;p != Stats;p--)       
        SWAP(p[0],p[-1]);
    p->Freq += 4; SummFreq += 4;
    EscFreq=SummFreq-p->Freq;
    Adder=(stream.OrderFall != 0 || stream.MRMethod > MRM_FREEZE);
    SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            StateCpy(tmp,*(p1=p));
            do StateCpy(p1[0],p1[-1]); while (tmp.Freq > (--p1)[-1].Freq);
            StateCpy(*p1,tmp);
        }
    } while ( --i );
    if (p->Freq == 0) {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;
	    OldNU=(NumStats+2) >> 1;
        if ((NumStats -= i) == 0) {
            StateCpy(tmp,*Stats);
            tmp.Freq=(2*tmp.Freq+EscFreq-1)/EscFreq;
            if (tmp.Freq > MAX_FREQ/3)
		        tmp.Freq=MAX_FREQ/3;
            stream.Memory.FreeUnits(Stats,OldNU);
	        StateCpy(oneState(),tmp);
            Flags=(Flags & 0x10)+0x08*(tmp.Symbol >= 0x40);
            stream.FoundState=&oneState();
	        return;
        }
        Stats = (STATE*) stream.Memory.ShrinkUnits(Stats,OldNU,(NumStats+2) >> 1);
        Flags &= ~0x08;
	    i=NumStats;
        Flags |= 0x08*((p=Stats)->Symbol >= 0x40);
        do {
	        Flags |= 0x08*((++p)->Symbol >= 0x40);
	    } while ( --i );
    }
    SummFreq += (EscFreq -= (EscFreq >> 1));
    Flags |= 0x04;
    stream.FoundState=Stats;
}


void PPM_CONTEXT::makeSuffix()
{
    STATE *p, *p1;
    if ( !NumStats ) {
        if ( !(p=&oneState())->Successor )  return;
        if ( !Suffix->NumStats )
            p1=&(Suffix->oneState());
        else
            for (p1=Suffix->Stats;p1->Symbol != p->Symbol;p1++)
                    ;
        p->Successor->Suffix=p1->Successor; p->Successor->makeSuffix();
    } else {
        for (p=Stats;p <= Stats+NumStats;p++) {
            if ( !p->Successor )            continue;
            if ( !Suffix )                  p->Successor->Suffix=this;
            else {
                for (p1=Suffix->Stats;p1->Symbol != p->Symbol;p1++)
                        ;
                p->Successor->Suffix=p1->Successor;
            }
            p->Successor->makeSuffix();
        }
    }
}
void PPM_CONTEXT::readModel(FILE* fp, UINT PrevSym, PPMD_Stream &stream)
{
    STATE* p;                               
    Suffix=NULL;
    NumStats=fgetc(fp);                      
    Flags=0x10*(PrevSym >= 0x40);
    if ( !NumStats ) {
        p=&oneState();                      
        p->Freq=fgetc(fp);
        Flags |= 0x08*((p->Symbol=fgetc(fp)) >= 0x40);
        if ((p->Freq & 0x80) != 0) {
            p->Freq &= ~0x80;
            p->Successor = (PPM_CONTEXT*) stream.Memory.AllocContext();
            p->Successor->readModel(fp,p->Symbol, stream);
        } else                              
            p->Successor=NULL;
        return;
    }
    Stats = (PPM_CONTEXT::STATE*) stream.Memory.AllocUnits((NumStats+2) >> 1);
    for (p=Stats;p <= Stats+NumStats;p++) {
        p->Freq=fgetc(fp);
        Flags |= 0x08*((p->Symbol=fgetc(fp)) >= 0x40);
    }
    int EscFreq=SummFreq=(Stats->Freq & ~0x80);
    Flags |= 0x04*(EscFreq < NumStats && EscFreq < 127);
    for (p=Stats;p <= Stats+NumStats;p++) {
        if ((p->Freq & 0x80) != 0) {
            p->Freq &= ~0x80;
            p->Successor = (PPM_CONTEXT*) stream.Memory.AllocContext();
            p->Successor->readModel(fp, p->Symbol, stream);
        } else                              
            p->Successor=NULL;
        p->Freq=(p == Stats)?(64):(p[-1].Freq-p[0].Freq);
        SummFreq += p->Freq;
    }
    if (EscFreq > 32) {
        SummFreq=(EscFreq >> 1);
        for (p=Stats;p <= Stats+NumStats;p++)
                SummFreq += (p->Freq -= (3*p->Freq) >> 2);
    }
}

/*
void PPM_CONTEXT::writeModel(int o,FILE* fp)
{
    STATE* p;
    int f, a, b, c;
    if (nc < o)                             nc=o;
    putc(NumStats,fp);
    if ( !NumStats ) {
        f=(p=&oneState())->Freq;
        if ( EscFreq )                      f=(2*f)/EscFreq;
        f=CLAMP(f,1,127) | 0x80*(p->Successor != NULL);
        putc(f,fp);                         putc(p->Symbol,fp);
        if ( p->Successor )                 p->Successor->write(o+1,fp);
        return;
    }
    for (p=Stats+1;p <= Stats+NumStats;p++) {
        if (p[0].Freq > p[-1].Freq) {
            STATE* p1=p;
            do { SWAP(p1[0],p1[-1]); } while (--p1 != Stats && p1[0].Freq > p1[-1].Freq);
        }
        if (p[0].Freq == p[-1].Freq && p[0].Successor && !p[-1].Successor) {
            STATE* p1=p;
            do { SWAP(p1[0],p1[-1]); } while (--p1 != Stats && p1[0].Freq == p1[-1].Freq && !p1[-1].Successor);
        }
    }
    a=Stats->Freq+!Stats->Freq;             f=(64*EscFreq+(b=a >> 1))/a;
    f=CLAMP(f,1,127) | 0x80*(Stats->Successor != NULL);
    putc(f,fp);                             c=64;
    for (p=Stats;p <= Stats+NumStats;p++) {
        f=(64*p->Freq+b)/a;                 f += !f;
        if (p != Stats)
            putc((c-f) | 0x80*(p->Successor != NULL),fp);
        c=f;                                putc(p->Symbol,fp);
    }
    for (p=Stats;p <= Stats+NumStats;p++)
            if ( p->Successor )             p->Successor->write(o+1,fp);
}*/
/*
void PPM_CONTEXT::writeModel(int o,FILE* fp1,FILE* fp2,FILE* fp3)
{
    STATE* p, * p1;
    int f, a, b, c, d;
    if ( !NumStats ) {
        f=(p=&oneState())->Freq;
        if ( EscFreq )                      f=(2*f)/EscFreq;
        putc(QTable[CLAMP(f,1,196)-1] | 0x80,fp1);
        if ( Suffix->NumStats ) {
            for (p1=Suffix->Stats, d=0;p1->Symbol != p->Symbol;p1++, d++)
                    ;
            putc(d,fp2);
        }
        if (o < MaxOrder)                   putc(p->Successor != NULL,fp3);
        if ( p->Successor )                 p->Successor->write(o+1,fp1,fp2,fp3);
        return;
    }
    a=Stats->Freq+!Stats->Freq;             f=(64*EscFreq+(b=a >> 1))/a;
    putc(QTable[CLAMP(f,1,196)-1] | 0x80,fp1);
    memset(CharMask,0,sizeof(CharMask));    c=64;
    for (p=Stats;p <= Stats+NumStats;p++) {
        f=(64*p->Freq+b)/a;                 f += !f;
        if (p != Stats)                     putc(c-f,fp1);
        c=f;
        if ( !Suffix )                      putc(p->Symbol,fp2);
        else if (p != Stats+NumStats || NumStats != Suffix->NumStats) {
            for (p1=Suffix->Stats, d=0;p1->Symbol != p->Symbol;p1++)
                    d += !CharMask[p1->Symbol];
            putc(d,fp2);                    CharMask[p->Symbol]=1;
        }
        if (o < MaxOrder)                   putc(p->Successor != NULL,fp3);
    }
    for (p=Stats;p <= Stats+NumStats;p++)
            if ( p->Successor )             p->Successor->write(o+1,fp1,fp2,fp3);
}
*/

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

PPMD_Stream::PPMD_Stream()
    : model_file(NULL), MinContext(NULL), StartClock(0)
{
    (DWORD&) DummySEE2Cont=PPMdSignature;
}

PPMD_Stream::~PPMD_Stream() {
    StopSubAllocator();
}

inline void PPMD_Stream::UpdateModel(PPM_CONTEXT* MinContext)
{
    PPM_CONTEXT::STATE* p=NULL;
    PPM_CONTEXT* Successor, * FSuccessor, * pc, * pc1=MaxContext;
    UINT ns1, ns, cf, sf, s0, FFreq=FoundState->Freq;
    BYTE Flag, sym, FSymbol=FoundState->Symbol;
    FSuccessor=FoundState->Successor;       pc=MinContext->Suffix;
    if (FFreq < MAX_FREQ/4 && pc) {
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != FSymbol) {
                do { sym=p[1].Symbol;       p++; } while (sym != FSymbol);
                if (p[0].Freq >= p[-1].Freq) {
                    SWAP(p[0],p[-1]);       p--;
                }
            }
            cf=2*(p->Freq < MAX_FREQ-9);
            p->Freq += cf;                  pc->SummFreq += cf;
        } else { p=&(pc->oneState());       p->Freq += (p->Freq < 32); }
    }
    if (!OrderFall && FSuccessor) {
        FoundState->Successor=CreateSuccessors(PPMD_TRUE,p,MinContext);
        if ( !FoundState->Successor )       goto RESTART_MODEL;
        MaxContext=FoundState->Successor;   
        return;
    }
    *(Memory.pText++) = FSymbol;
    Successor = (PPM_CONTEXT*) Memory.pText;
    if (Memory.pText >= Memory.UnitsStart)
	goto RESTART_MODEL;
    if ( FSuccessor ) {
        if ((BYTE*) FSuccessor < Memory.UnitsStart)
	    FSuccessor=CreateSuccessors(PPMD_FALSE,p,MinContext);
    } else
	FSuccessor=ReduceOrder(p,MinContext);
    if ( !FSuccessor )
	goto RESTART_MODEL;
    if ( !--OrderFall ) {
        Successor=FSuccessor;
	Memory.pText -= (MaxContext != MinContext);
    } else if (MRMethod > MRM_FREEZE) {
        Successor=FSuccessor;
	Memory.pText=Memory.HeapStart;
        OrderFall=0;
    }
    s0=MinContext->SummFreq-(ns=MinContext->NumStats)-FFreq;
    for (Flag=0x08*(FSymbol >= 0x40);pc1 != MinContext;pc1=pc1->Suffix) {
        if ((ns1=pc1->NumStats) != 0) {
            if ((ns1 & 1) != 0) {
                p=(PPM_CONTEXT::STATE*)
		    Memory.ExpandUnits(pc1->Stats,(ns1+1) >> 1);
                if ( !p )
		    goto RESTART_MODEL;
                pc1->Stats=p;
            }
            pc1->SummFreq += (3*ns1+1 < ns);
        } else {
            p=(PPM_CONTEXT::STATE*) Memory.AllocUnits(1);
            if ( !p )                       goto RESTART_MODEL;
            StateCpy(*p,pc1->oneState());   pc1->Stats=p;
            if (p->Freq < MAX_FREQ/4-1)     p->Freq += p->Freq;
            else                            p->Freq  = MAX_FREQ-4;
            pc1->SummFreq=p->Freq+InitEsc+(ns > 2);
        }
        cf=2*FFreq*(pc1->SummFreq+6);       sf=s0+pc1->SummFreq;
        if (cf < 6*sf) {
            cf=1+(cf > sf)+(cf >= 4*sf);
            pc1->SummFreq += 4;
        } else {
            cf=4+(cf > 9*sf)+(cf > 12*sf)+(cf > 15*sf);
            pc1->SummFreq += cf;
        }
        p=pc1->Stats+(++pc1->NumStats);     p->Successor=Successor;
        p->Symbol = FSymbol;                p->Freq = cf;
        pc1->Flags |= Flag;
    }
    MaxContext=FSuccessor;                  return;
RESTART_MODEL:
    RestoreModelRare(pc1,MinContext,FSuccessor);
}

inline void PPMD_Stream::ClearMask() 
{
    EscCount=1;
    memset(CharMask,0,sizeof(CharMask));
    // if (++PrintCount == 0)
    // PrintInfo(DecodedFile,EncodedFile);
}

void PPMD_Stream::StartModelRare(int MaxOrder,MR_METHOD MRMethod)
{
    UINT i, k, m;
    memset(CharMask,0,sizeof(CharMask));
    EscCount=1; //PrintCount=1;
    if (MaxOrder < 2) {                     // we are in solid mode
        OrderFall=this->MaxOrder;
        for (PPM_CONTEXT* pc=MaxContext;pc->Suffix != NULL;pc=pc->Suffix)
                OrderFall--;
        return;
    }
    OrderFall=this->MaxOrder=MaxOrder;
    this->MRMethod=MRMethod;
    Memory.InitSubAllocator();
    RunLength=InitRL=-((MaxOrder < 12)?MaxOrder:12)-1;
    MaxContext = (PPM_CONTEXT*) Memory.AllocContext();
    MaxContext->Suffix=NULL;
    MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
    MaxContext->Stats = (PPM_CONTEXT::STATE*) Memory.AllocUnits(256/2);
    for (PrevSuccess=i=0;i < 256;i++) {
        MaxContext->Stats[i].Symbol=i;      MaxContext->Stats[i].Freq=1;
        MaxContext->Stats[i].Successor=NULL;
    }
static const WORD InitBinEsc[]={0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051};
    for (i=m=0;m < 25;m++) {
        while (QTable[i] == m)              i++;
        for (k=0;k < 8;k++)
                BinSumm[m][k]=BIN_SCALE-InitBinEsc[k]/(i+1);
        for (k=8;k < 64;k += 8)
                memcpy(BinSumm[m]+k,BinSumm[m],8*sizeof(WORD));
    }
    for (i=m=0;m < 24;m++) {
        while (QTable[i+3] == m+3)          i++;
        SEE2Cont[m][0].init(2*i+5);
        for (k=1;k < 32;k++)                SEE2Cont[m][k]=SEE2Cont[m][0];
    }
}

PPM_CONTEXT* PPMD_Stream::ReduceOrder(PPM_CONTEXT::STATE* p,
					     PPM_CONTEXT* pc)
{
    PPM_CONTEXT::STATE* p1,  * ps[MAX_O], ** pps=ps;
    PPM_CONTEXT* pc1=pc, * UpBranch = (PPM_CONTEXT*) Memory.pText;
    BYTE tmp, sym=FoundState->Symbol;
    *pps++ = FoundState;                    FoundState->Successor=UpBranch;
    OrderFall++;
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    for ( ; ; ) {
        if ( !pc->Suffix ) {
            if (MRMethod > MRM_FREEZE) {
	    FROZEN:
		do {
		    (*--pps)->Successor = pc;
		} while (pps != ps);
		Memory.pText=Memory.HeapStart+1;
		OrderFall=1;
            }
            return pc;
        }
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=2*(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else { p=&(pc->oneState());       p->Freq += (p->Freq < 32); }
    LOOP_ENTRY:
        if ( p->Successor )                 break;
        *pps++ = p;                         p->Successor=UpBranch;
        OrderFall++;
    }
    if (MRMethod > MRM_FREEZE) {
        pc = p->Successor;
	goto FROZEN;
    } else if (p->Successor <= UpBranch) {
        p1=FoundState;
	FoundState=p;
        p->Successor=CreateSuccessors(PPMD_FALSE,NULL,pc);
        FoundState=p1;
    }
    if (OrderFall == 1 && pc1 == MaxContext) {
        FoundState->Successor=p->Successor;
	Memory.pText--;
    }
    return p->Successor;
}

void PPMD_Stream::RestoreModelRare(PPM_CONTEXT* pc1,
				   PPM_CONTEXT* MinContext,
				   PPM_CONTEXT* FSuccessor)
{
    PPM_CONTEXT* pc;
    PPM_CONTEXT::STATE* p;
    for (pc=MaxContext, Memory. pText=Memory.HeapStart; pc != pc1; pc=pc->Suffix)
        if (--(pc->NumStats) == 0) {
            pc->Flags=(pc->Flags & 0x10)+0x08*(pc->Stats->Symbol >= 0x40);
            p=pc->Stats;
            StateCpy(pc->oneState(),*p);
            Memory.SpecialFreeUnit(p);
            pc->oneState().Freq=(pc->oneState().Freq+11) >> 3;
        } else
    		pc->refresh((pc->NumStats+3) >> 1,PPMD_FALSE, *this);
    for ( ;pc != MinContext;pc=pc->Suffix)
            if ( !pc->NumStats )
                pc->oneState().Freq -= pc->oneState().Freq >> 1;
            else if ((pc->SummFreq += 4) > 128+4*pc->NumStats)
        		pc->refresh((pc->NumStats+2) >> 1, PPMD_TRUE, *this);
    if (MRMethod > MRM_FREEZE) {
        MaxContext=FSuccessor;
	    Memory.GlueCount += !(Memory.BList[1].Stamp & 1);
    }
    else if (MRMethod == MRM_FREEZE) {
        while ( MaxContext->Suffix )
	        MaxContext=MaxContext->Suffix;
        MaxContext->removeBinConts(0, *this);
	    MRMethod=MR_METHOD(MRMethod+1);
        Memory.GlueCount=0;
	    OrderFall=MaxOrder;
    }
    else if (MRMethod == MRM_RESTART || Memory.GetUsedMemory() < (Memory.SubAllocatorSize >> 1)) {
        StartModelRare(MaxOrder,MRMethod);
        EscCount=0;
	// PrintCount=0xFF;
    } else {
        while ( MaxContext->Suffix )
	       MaxContext=MaxContext->Suffix;
        do {
            MaxContext->cutOff(0, *this);
	        Memory.ExpandTextArea();
        } while (Memory.GetUsedMemory() > 3*(Memory.SubAllocatorSize >> 2));
        Memory.GlueCount=0;
	    OrderFall=MaxOrder;
    }
}

PPM_CONTEXT* PPMD_Stream::CreateSuccessors(BOOL Skip,PPM_CONTEXT::STATE* p,
        PPM_CONTEXT* pc)
{
    PPM_CONTEXT ct, * UpBranch=FoundState->Successor;
    PPM_CONTEXT::STATE* ps[MAX_O], ** pps=ps;
    UINT cf, s0;
    BYTE tmp, sym=FoundState->Symbol;
    if ( !Skip ) {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  goto NO_LOOP;
    }
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    do {
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else {
            p=&(pc->oneState());
            p->Freq += (!pc->Suffix->NumStats & (p->Freq < 24));
        }
LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc=p->Successor;                break;
        }
        *pps++ = p;
    } while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          return pc;
    ct.NumStats=0;                          ct.Flags=0x10*(sym >= 0x40);
    ct.oneState().Symbol=sym=*(BYTE*) UpBranch;
    ct.oneState().Successor=(PPM_CONTEXT*) (((BYTE*) UpBranch)+1);
    ct.Flags |= 0x08*(sym >= 0x40);
    if ( pc->NumStats ) {
        if ((p=pc->Stats)->Symbol != sym)
                do { tmp=p[1].Symbol;       p++; } while (tmp != sym);
        s0=pc->SummFreq-pc->NumStats-(cf=p->Freq-1);
        ct.oneState().Freq=1+((2*cf <= s0)?(5*cf > s0):((cf+2*s0-3)/s0));
    } else
            ct.oneState().Freq=pc->oneState().Freq;
    do {
        PPM_CONTEXT* pc1 = (PPM_CONTEXT*) Memory.AllocContext();
        if ( !pc1 )                         return NULL;
        ((DWORD*) pc1)[0] = ((DWORD*) &ct)[0];
        ((DWORD*) pc1)[1] = ((DWORD*) &ct)[1];
        pc1->Suffix=pc;                     (*--pps)->Successor=pc=pc1;
    } while (pps != ps);
    return pc;
}

BOOL PPMD_Stream::StartSubAllocator(UINT SASize) {
    return Memory.StartSubAllocator(SASize);
}

void PPMD_Stream::StopSubAllocator() {
    Memory.StopSubAllocator();
}

void PPMD_Stream::PrintInfo(PZ_In* InFile,PZ_Out* OutFile,BOOL EncodeFlag)
{
    char WrkStr[320];
    UINT NDec=OutFile->BytesWritten();
    NDec += (NDec == 0);
    UINT NEnc=InFile->BytesRead();
    if ( EncodeFlag ) {UINT temp = NDec; NDec=NEnc; NEnc=temp;}
    UINT n1=(8U*NEnc)/NDec;
    UINT n2=(100U*(8U*NEnc-NDec*n1)+NDec/2U)/NDec;
    if (n2 == 100) { n1++;                  n2=0; }
    int RunTime;
    if (CLOCKS_PER_SEC <= 1000)
        RunTime=((clock()-StartClock) << 10)/int(CLOCKS_PER_SEC);
    else
        RunTime=((clock()-StartClock))/(int(CLOCKS_PER_SEC)>>10);
    UINT Speed=NDec/(RunTime+(RunTime == 0));
    UINT UsedMemory=GetUsedMemory() >> 10;
    UINT m1=UsedMemory >> 10;
    UINT m2=(10U*(UsedMemory-(m1 << 10))+(1 << 9)) >> 10;
    if (m2 == 10) { m1++; m2=0; }
    if ( EncodeFlag ) {UINT temp = NDec; NDec=NEnc; NEnc=temp;}
    sprintf(WrkStr,"%7d > %7d, %1d.%02d bpb, used:%3d.%1dMB, speed: %d KB/sec",
            NEnc,NDec,n1,n2,m1,m2,Speed);
    fprintf(stderr, "%-79.79s\r",WrkStr);
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void PPMD_Encoder::SetParams (UINT SASize, UINT MaxOrder, MR_METHOD MRMethod, BOOL Solid, char *ModelFile) 
{
    if (!StartSubAllocator(SASize)) 
    	throw PPMD_Exception("Unable to create SubAllocator: out of memory");

    this->SASize=SASize;
    this->MaxOrder=MaxOrder;
    NewMaxOrder=MaxOrder;
    this->MRMethod=MRMethod;
    this->Solid=Solid;
    this->model_file=ModelFile;
}

void PPMD_Encoder::EncodeInit() 
{
    if (!StartClock) StartClock=clock();

    ari.EncoderInit();

    if (Solid) {
        StartModelRare(NewMaxOrder,MRMethod);
        NewMaxOrder=1;
    } else {
        StartModelRare(MaxOrder,MRMethod);
    }
    MinContext=MaxContext;
}

PZ_RESULT PPMD_Encoder::Flush(PZ_Out* EncodedFile, PZ_In* DecodedFile)
{
    PZ_RESULT rtn;
    while((rtn=EncodeOneSymbol(EncodedFile, EOF)) == P_OK) {};
    if (rtn==P_FINISHED_BLOCK) {
        if ((rtn=ari.EncoderFlush(EncodedFile)) != P_OK) 
            return rtn; //unable to flush
    } else
        return rtn; //Cant finish flushing! (Should mean P_MORE_OUTPUT)

    Initialised = 0;
    if (Verbose && DecodedFile) PrintInfo(DecodedFile, EncodedFile, 1);
    return P_OK;
}


PZ_RESULT PPMD_Encoder::Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile)
{
    if (!Initialised) {
        EncodeInit();
        FinishedBlock=0;
        Initialised=1;
        OneInTheSpout=-1;
    }

    PZ_RESULT rtn;
    do {
        int c;
        if (OneInTheSpout>=0)
            // Repeat last char because we aren't finished with it yet
            c = OneInTheSpout;
        else { 
            c = DecodedFile->GetC();
            if (c == EOF) 
                return P_OK;
        }

        rtn = EncodeOneSymbol(EncodedFile, c);
        if (Verbose && (++stats_count>1000000)) {PrintInfo(DecodedFile,EncodedFile, 1); stats_count=1;}
    } while (rtn>=0); //Should only ever be P_MORE_OUTPUT

    if (Verbose) PrintInfo(DecodedFile, EncodedFile, 1);
    return rtn;
}

inline PZ_RESULT PPMD_Encoder::EncodeOneSymbol(PZ_Out* EncodedFile, int c)
{
    if (FinishedBlock) return P_FINISHED_BLOCK; //Please reset me first!

    if (NeedMoreOutput) {
        //restarting from a position of not having had enough input
        if (ari.EncoderNormalize(EncodedFile)==EOF) {
            OneInTheSpout=c;
            return P_MORE_OUTPUT; //still not enough
        } else {
            NeedMoreOutput=0;
            OneInTheSpout=-1;
            if (!FoundState) goto CONTINUE;
            else return P_OK; //finished with this symbol already!
        }
    }

    MinContext=MaxContext;
    if ( MinContext->NumStats ) {
        MinContext->encodeSymbol1(c, *this);
	    ari.EncodeSymbol();
    } else {
        MinContext->encodeBinSymbol(c, *this);
	    ari.ShiftEncodeSymbol(TOT_BITS);
    }
    while ( !FoundState ) {
        if (ari.EncoderNormalize(EncodedFile)==EOF) {
            OneInTheSpout=c;
            NeedMoreOutput=1;
            return P_MORE_OUTPUT;
        }
CONTINUE:
        do {
            OrderFall++;
            MinContext=MinContext->Suffix;
            if ( !MinContext ) {
//                MaxContext=MinContext;
                FinishedBlock=1;
		        return P_FINISHED_BLOCK;
            }
        } while (MinContext->NumStats == NumMasked);
        MinContext->encodeSymbol2(c, *this);
	    ari.EncodeSymbol();
    }
    if (!OrderFall && (BYTE*) FoundState->Successor >= Memory.UnitsStart)
//        PrefetchData(MaxContext=FoundState->Successor);
        MaxContext=FoundState->Successor;
    else {
        UpdateModel(MinContext);
//	    PrefetchData(MaxContext);
        if (EscCount == 0)
		    // ClearMask(EncodedFile,DecodedFile);
		    ClearMask();
    }
    if (ari.EncoderNormalize(EncodedFile)==EOF) {
        OneInTheSpout=c;
        NeedMoreOutput=1;
        return P_MORE_OUTPUT;
    }

    return P_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////

void PPMD_Decoder::SetParams (UINT SASize, UINT MaxOrder, MR_METHOD MRMethod, BOOL Solid, char *ModelFile) 
{
    if (!StartSubAllocator(SASize)) 
    	throw PPMD_Exception("Unable to create SubAllocator: out of memory");

    this->SASize=SASize;
    this->MaxOrder=MaxOrder;
    NewMaxOrder=MaxOrder;
    this->MRMethod=MRMethod;
    this->Solid=Solid;
    this->model_file=ModelFile;
}

PZ_RESULT PPMD_Decoder::DecodeInit(PZ_In* EncodedFile)
{
    if (!StartClock) StartClock=clock();

    if (ari.DecoderInit(EncodedFile) == P_MORE_INPUT) return P_MORE_INPUT;

    if (Solid) {
        StartModelRare(NewMaxOrder,MRMethod);
        NewMaxOrder=1;
    } else {
        StartModelRare(MaxOrder,MRMethod);
    }
    MinContext=MaxContext;

    return P_OK;
}

PZ_RESULT PPMD_Decoder::DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    if (!Initialised) {
        // We probably need the concept of part initialised
        // Instead we have some clunkiness determining init failure due to EOF versus not enough data
        if (DecodeInit(EncodedFile) == P_MORE_INPUT) {
            Initialised = 0;
            if (ari.GetInitState() == 0) {
                // Nothing happened, still on square 1.  
                return P_OK; // Rtn P_OK to show perhaps finished
            } else {
                return P_MORE_INPUT; // Part init, want to keep going
            }
        } else {
            Initialised = 1;
        }
        OneInTheSpout = -1;
    } else if (OneInTheSpout>=0) {
        //Write out our missing byte
        if (DecodedFile->PutC(OneInTheSpout) == EOF)
            return P_MORE_OUTPUT;
        OneInTheSpout = -1;
    }

    INT symbol;
    while ( (symbol = DecodeOneSymbol(EncodedFile)) >= 0) {
        if (DecodedFile->PutC(symbol) == EOF) {
            OneInTheSpout = symbol;
            return P_MORE_OUTPUT;
        }
        if (Verbose && (++stats_count>2000000)) {PrintInfo(EncodedFile,DecodedFile, 0); stats_count=1;}
    }

    if (symbol == P_FINISHED_BLOCK) {
        Initialised=0;
        if (Verbose) PrintInfo(EncodedFile,DecodedFile, 0);
    }

    if (symbol<0) 
        return (PZ_RESULT)symbol;
    else
        return P_OK; //shouldnt be able to get here
}

PZ_RESULT PPMD_Decoder::Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    // Just keep calling DecodeBlock until we run out of input or space in the output!
    PZ_RESULT rtn;
    while ( (rtn=DecodeBlock(EncodedFile, DecodedFile)) == P_FINISHED_BLOCK ) {};

//    if ( (rtn==P_MORE_INPUT) && !Initialised) {
//        // Finished a block.  No more input
//        return P_OK;
//    } else
        return rtn;
}

inline INT PPMD_Decoder::DecodeOneSymbol(PZ_In* EncodedFile)
{
    if (NeedMoreInput) {
        //restarting from a position of not having had enough input
        if (ari.DecoderNormalize(EncodedFile)==EOF) {
            return P_MORE_INPUT; //still not enough
        } else {
            NeedMoreInput=0;
            if (!FoundState) goto CONTINUE;
        }
    }

//    BYTE ns=MinContext->NumStats;
    if ( MinContext->NumStats )
	    (MinContext->decodeSymbol1(*this));
    else
        (MinContext->decodeBinSymbol(*this));

    ari.RemoveSubrange();

    while ( !FoundState ) {
        if (ari.DecoderNormalize(EncodedFile)==EOF) {
            NeedMoreInput=1;
            return P_MORE_INPUT;
        }
        do {
CONTINUE:            
            OrderFall++;
            MinContext=MinContext->Suffix;
            if ( !MinContext )
		        return P_FINISHED_BLOCK;
        } while (MinContext->NumStats == NumMasked);
        MinContext->decodeSymbol2(*this);
	    ari.RemoveSubrange();
    }

    BYTE symbol = FoundState->Symbol;
    if (!OrderFall && (BYTE*) FoundState->Successor >= Memory.UnitsStart)
//            PrefetchData(MaxContext=FoundState->Successor);
        MaxContext=FoundState->Successor;
    else {
        UpdateModel(MinContext);
//	    PrefetchData(MaxContext);
        if (EscCount == 0)
		    // ClearMask(EncodedFile,DecodedFile);
		    ClearMask();
    }
    MinContext=MaxContext;
    if (ari.DecoderNormalize(EncodedFile)==EOF) {
        // Nothing we can do about it now, make a note though
        NeedMoreInput=1;
    }

    return symbol;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

PZ_RESULT PZIP_Encoder::Encode(PZ_In* DecodedFile, PZ_Out* EncodedFile)
{
    if (Initialised == 0) {
        // Check params
        if (SASize > 2048) SASize=2048;
        if (MRMethod>2) MRMethod=MRM_RESTART;
        if (MaxOrder>64) MaxOrder=64;
        if (Solid != 0) Solid=1;

        //Need to write out header
        ARC_INFO header;
        header.signature = PPMdSignature;
        header.size = (WORD)((SASize-1) | (Solid << 11) | ((Variant-'A') << 12));
        header.info = (MRMethod << 6 | (MaxOrder-1));

        for (int i=0; i<sizeof(header); i++) {
            // BUG: Probably breaks if we can't write data?
            EncodedFile->PutC(((BYTE*)&header)[i]);
        }
    }

    //OK lets start encoding!
    PZ_RESULT rtn;
    if ((rtn=PPMD_Encoder::Encode(DecodedFile, EncodedFile)) != P_OK)
        return rtn;
    
    // TODO: Probably a bug here if we run out of output space?
    if ((rtn = PPMD_Encoder::Flush(EncodedFile, DecodedFile)) != P_OK)
        return rtn;

    // Write out CRC32 value.  
    // TODO: Calculate CRC32!!
    // BUG: Probably breaks if we can't write data?
    for (int i=0; i<4; i++) {
        EncodedFile->PutC(0);
    }

    return rtn;
}

///////////////////////////////////////////////////////////////////////////////////////////

PZ_RESULT PZIP_Decoder::DecodeBlock(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    if (Initialised == 0) {
        //Need to write in header
        ARC_INFO header;

        for (int i=0; i<sizeof(header); i++) {
            ((BYTE*)&header)[i] = EncodedFile->GetC();
        }

        UINT MaxOrder, SASize;
        BOOL Solid;
        MR_METHOD MRMethod;
        UINT Variant;

        MaxOrder = (header.info & 0x3F) + 1;
        MRMethod = (MR_METHOD)(header.info >> 6);
        SASize = (header.size & 0x7FF) + 1;
        Solid = (header.size >> 11) & 0x1;
        Variant = (header.size >> 12)+'A';

        if ((header.signature!=PPMdSignature) || (Variant!=::Variant))
            return P_INVALID_FORMAT;

        PPMD_Decoder::SetParams(SASize, MaxOrder, MRMethod, Solid, NULL);
    }

    PZ_RESULT rtn;
    if ((rtn=PPMD_Decoder::DecodeBlock(EncodedFile, DecodedFile)) != P_OK) {
        return rtn;
    } else {
        // Read and check CRC32
        // TODO: Implementation!
        // BUG: Stuff probably breaks here if we run out of data?
        for (int i=0; i<4; i++) {
            EncodedFile->GetC();
        }
    } 

    return P_OK;
}

PZ_RESULT PZIP_Decoder::Decode(PZ_In* EncodedFile,PZ_Out* DecodedFile)
{
    // Just keep calling DecodeBlock until we run out of input or space in the output!
    PZ_RESULT rtn;
    while ( (rtn=DecodeBlock(EncodedFile, DecodedFile)) == P_FINISHED_BLOCK ) {};

    if ( (rtn==P_MORE_INPUT) && !Initialised) {
        // Finished a block.  No more input
        return P_OK;
    } else
        return rtn;
}
