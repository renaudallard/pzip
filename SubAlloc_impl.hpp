/****************************************************************************
 *  This file is part of PZip project                                       *
 *  Contents: memory allocation routines                                    *
 *  CHANGELOG:
 *  Initial algorithm and code: Dmitry Shkarin 1997, 1999-2001              *
 *  Salvador Fandi�o (2003): Converted to C++                               *
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

#if !defined(__SUBALLOC_IMPL_HPP__)
#define __SUBALLOC_IMPL_HPP__

#include <string.h>


inline void PrefetchData(void* Addr)
{
#if defined(_USE_PREFETCHING)
    BYTE PrefetchByte = *(volatile BYTE*) Addr;
#endif /* defined(_USE_PREFETCHING) */
}
inline void BLK_NODE::insert(void* pv,int NU) {
    MEM_BLK* p=(MEM_BLK*) pv;               link(p);
    p->Stamp=~0UL;                          p->NU=NU;
    Stamp++;
}
inline UINT U2B(UINT NU) { return 8*NU+4*NU; }
inline void SubAlloc::SplitBlock(void* pv,UINT OldIndx,UINT NewIndx)
{
    UINT i, k, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
    BYTE* p=((BYTE*) pv)+U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) {
        k=Indx2Units[--i];                  BList[i].insert(p,k);
        p += U2B(k);                        UDiff -= k;
    }
    BList[Units2Indx[UDiff-1]].insert(p,UDiff);
}
inline DWORD SubAlloc::GetUsedMemory()
{
    DWORD i, RetVal=SubAllocatorSize-(HiUnit-LoUnit)-(UnitsStart-pText);
    for (i=0;i < N_INDEXES;i++)
            RetVal -= UNIT_SIZE*Indx2Units[i]*BList[i].Stamp;
    return RetVal;
}
inline void SubAlloc::StopSubAllocator() {
    if ( SubAllocatorSize ) {
        SubAllocatorSize=0;                 
        delete[] HeapStart;
    }
}
inline BOOL SubAlloc::StartSubAllocator(UINT SASize)
{
    DWORD t=SASize << 20U;
    if (SubAllocatorSize == t)
	return PPMD_TRUE;
    StopSubAllocator();
    if ((HeapStart=new BYTE[t]) == NULL)
	return PPMD_FALSE;
    SubAllocatorSize=t;
    return PPMD_TRUE;
}

inline void SubAlloc::InitSubAllocator()
{
    memset(BList,0,sizeof(BList));
    HiUnit=(pText=HeapStart)+SubAllocatorSize;
    UINT Diff=UNIT_SIZE*(SubAllocatorSize/8/UNIT_SIZE*7);
    LoUnit=UnitsStart=HiUnit-Diff;          GlueCount=0;
}
inline void SubAlloc::GlueFreeBlocks()
{
    UINT i, k, sz;
    MEM_BLK s0, * p, * p0, * p1;
    if (LoUnit != HiUnit)                   *LoUnit=0;
    for (i=0, (p0=&s0)->next=NULL;i < N_INDEXES;i++)
            while ( BList[i].avail() ) {
                p=(MEM_BLK*) BList[i].remove();
                if ( !p->NU )               continue;
                while ((p1=p+p->NU)->Stamp == ~0UL) {
                    p->NU += p1->NU;        p1->NU=0;
                }
                p0->link(p);                p0=p;
            }
    while ( s0.avail() ) {
        p=(MEM_BLK*) s0.remove();           sz=p->NU;
        if ( !sz )                          continue;
        for ( ;sz > 128;sz -= 128, p += 128)
                BList[N_INDEXES-1].insert(p,128);
        if (Indx2Units[i=Units2Indx[sz-1]] != sz) {
            k=sz-Indx2Units[--i];           BList[k-1].insert(p+(sz-k),k);
        }
        BList[i].insert(p,Indx2Units[i]);
    }
    GlueCount=1 << 13;
}
inline void* SubAlloc::AllocUnitsRare(UINT indx)
{
    UINT i=indx;
    if ( !GlueCount ) {
        GlueFreeBlocks();
        if ( BList[i].avail() )             
            return BList[i].remove();
    }
    do {
        if (++i == N_INDEXES) {
            GlueCount--;                    i=U2B(Indx2Units[indx]);
            return ((UINT)(UnitsStart-pText) > i)?(UnitsStart -= i):(NULL);
        }
    } while ( !BList[i].avail() );
    void* RetVal=BList[i].remove();         SplitBlock(RetVal,i,indx);
    return RetVal;
}
inline void* SubAlloc::AllocUnits(UINT NU)
{
    UINT indx=Units2Indx[NU-1];
    if ( BList[indx].avail() )              
        return BList[indx].remove();
    void* RetVal=LoUnit;                    
    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit)                   
        return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);        
    return AllocUnitsRare(indx);
}
inline void* SubAlloc::AllocContext()
{
    if (HiUnit != LoUnit)                   return (HiUnit -= UNIT_SIZE);
    else if ( BList->avail() )              return BList->remove();
    else                                    return AllocUnitsRare(0);
}
inline void UnitsCpy(void* Dest,void* Src,UINT NU)
{
    DWORD* p1=(DWORD*) Dest, * p2=(DWORD*) Src;
    do {
        p1[0]=p2[0];
	    p1[1]=p2[1];
        p1[2]=p2[2];
        p1 += 3;
	    p2 += 3;
    } while ( --NU );
}
inline void* SubAlloc::ExpandUnits(void* OldPtr,UINT OldNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
    if (i0 == i1)                           
        return OldPtr;
    void* ptr=AllocUnits(OldNU+1);
    if ( ptr ) {
        UnitsCpy(ptr,OldPtr,OldNU);         
        BList[i0].insert(OldPtr,OldNU);
    }
    return ptr;
}
inline void* SubAlloc::ShrinkUnits(void* OldPtr,UINT OldNU,UINT NewNU)
{
    UINT i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
    if (i0 == i1)                           return OldPtr;
    if ( BList[i1].avail() ) {
        void* ptr=BList[i1].remove();       UnitsCpy(ptr,OldPtr,NewNU);
        BList[i0].insert(OldPtr,Indx2Units[i0]);
        return ptr;
    } else {
        SplitBlock(OldPtr,i0,i1);           return OldPtr;
    }
}
inline void SubAlloc::FreeUnits(void* ptr,UINT NU) {
    UINT indx=Units2Indx[NU-1];
    BList[indx].insert(ptr,Indx2Units[indx]);
}
inline void SubAlloc::SpecialFreeUnit(void* ptr)
{
    if ((BYTE*) ptr != UnitsStart) {
	BList->insert(ptr,1);
    }
    else {
	*(DWORD*) ptr=~0UL;
	UnitsStart += UNIT_SIZE;
    }
}
inline void* SubAlloc::MoveUnitsUp(void* OldPtr,UINT NU)
{
    UINT indx=Units2Indx[NU-1];
    if ((BYTE*) OldPtr > UnitsStart+16*1024 || (BLK_NODE*) OldPtr > BList[indx].next)
            return OldPtr;
    void* ptr=BList[indx].remove();
    UnitsCpy(ptr,OldPtr,NU);                NU=Indx2Units[indx];
    if ((BYTE*) OldPtr != UnitsStart)       BList[indx].insert(OldPtr,NU);
    else                                    UnitsStart += U2B(NU);
    return ptr;
}
inline void SubAlloc::ExpandTextArea()
{
    BLK_NODE* p;
    UINT Count[N_INDEXES];                  memset(Count,0,sizeof(Count));
    while ((p=(BLK_NODE*) UnitsStart)->Stamp == ~0UL) {
        MEM_BLK* pm=(MEM_BLK*) p;           UnitsStart=(BYTE*) (pm+pm->NU);
        Count[Units2Indx[pm->NU-1]]++;      pm->Stamp=0;
    }
    for (UINT i=0;i < N_INDEXES;i++)
        for (p=BList+i;Count[i] != 0;p=p->next)
            while ( !p->next->Stamp ) {
                p->unlink();                BList[i].Stamp--;
                if ( !--Count[i] )          break;
            }
}

#endif
