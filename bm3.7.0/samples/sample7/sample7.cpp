/*
Copyright(c) 2002-2005 Anatoliy Kuznetsov(anatoliy_kuznetsov at yahoo.com)

Permission is hereby granted, free of charge, to any person 
obtaining a copy of this software and associated documentation 
files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, 
publish, distribute, sublicense, and/or sell copies of the Software, 
and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
OTHER DEALINGS IN THE SOFTWARE.
*/

/* @example sample7.cpp
 This example demonstrates using of memory save mode of bitset operations.

For more information please visit:  http://bmagic.sourceforge.net

*/

/* @file
    @ingroup mset
*/

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Example disabled as deprecated...

int main()
{
    return 1;
}

/*

// BM library version 3.1.3 and later can keep internal bit flags in pointers.
// This is efficient but not completely portable hack. 
// For compatibility it can be disabled it by defining BM_DISBALE_BIT_IN_PTR.
// If you do not disable bits in pointer the second template parameter of bvector
// is simply ignored and portion of this example is becoming irrelevant.
#define BM_DISBALE_BIT_IN_PTR
#include "bm.h"

using namespace std;

// Customized bitvector uses standard memory allocator and uses
// an alternative implementation of internal set. Saves memory when we 
// work with sparse or dense bitsets.
typedef bm::bvector<bm::standard_allocator, 
                    bm::miniset<bm::block_allocator, bm::set_total_blocks> > bvect;


const unsigned setscount = 10000;
const unsigned randombits = 150;
const unsigned maxbit = 100000000;

bvect*  bitsets[setscount];

// ---------------------------------------------------------

void CreateSets()
{
    unsigned mu = 0;
    for (unsigned i = 0; i < setscount; ++i)
    {
        if ((i % 100) == 0) { cout << "."; cout.flush(); }
        // create bitvector using in GAP mode using an alternative
        // GAP levels table (minimalistic).
        bitsets[i] = 
            new bvect(bm::BM_GAP, bm::gap_len_table_min<true>::_len, maxbit);
        bvect& bv = *bitsets[i];
        bvect::statistics st;
        bv.calc_stat(&st);
        mu += st.memory_used;
    }
    cout << endl << "Created " << setscount << " sets." << endl; 
    cout << "Used " << mu / (1024*1024)<< " MB." << endl;
}

// ---------------------------------------------------------

void FillSets()
{
    unsigned mu, bit_blocks, gap_blocks;
    cout << "Filling sets...";
    mu = bit_blocks = gap_blocks = 0;
    for (unsigned i = 0; i < setscount; ++i)
    {
        if ((i % 100) == 0) { cout << "."; cout.flush(); }
        if ((i % 3) == 0) continue;
        bvect& bv = *bitsets[i];
        unsigned bn = 0;
        for (unsigned j = 0; j < randombits; j+=3)
        {
            bn += (maxbit / randombits) + rand() % 10;
            if (bn > maxbit) bn = rand() % maxbit;

            bv[bn] = true;
            bv[bn+1] = true;
            bv[bn+2] = true;
        }
        bvect::statistics st;
        bv.calc_stat(&st);
        mu += st.memory_used;
        bit_blocks += st.bit_blocks;
        gap_blocks += st.gap_blocks;
    }
    cout << endl << "Used " << mu / (1024*1024)<< " MB." << endl;

    cout << "BIT Blocks=" << bit_blocks << endl;
    cout << "GAP Blocks=" << gap_blocks << endl;
}

// ---------------------------------------------------------

void EnumerateSets()
{
    cout << "Enumerating sets...";
    unsigned bitcnt = 0;
    for (unsigned i = 0; i < setscount; ++i)
    {
        if ((i % 100) == 0) { cout << "."; cout.flush(); }
        bvect& bv = *bitsets[i];

        bvect::enumerator en = bv.first();
        bvect::enumerator en_end = bv.end();

        for (;en < en_end; ++en)
        {
            ++bitcnt;
        }
    }
    cout << endl << bitcnt << endl;
}

// ---------------------------------------------------------

void DestroySets()
{
    for (unsigned i = 0; i < setscount; ++i)
    {
        delete bitsets[i];
    }
}

// ---------------------------------------------------------

void OrSets()
{
    bvect res;
    cout << "Calculating Or...";
    for (unsigned i = 0; i < setscount; ++i)
    {
        if ((i % 100) == 0) { cout << "."; cout.flush(); }
        const bvect& bv = *bitsets[i];

        res |= bv;
    }
    cout << endl << res.count() << endl;
}

// ---------------------------------------------------------



int main(void)
{
    time_t start_time = time(0);

    CreateSets();
    FillSets();
    EnumerateSets();
    OrSets();

    time_t end_time = time(0);
    time_t elapsed = end_time - start_time;
    cout << "elapsed=" << elapsed << endl;
    unsigned ops;
    if (elapsed)
        ops = setscount / elapsed;
    else
        ops = setscount;

    cout << "Time = " << (end_time - start_time) << endl;
    cout << "Operations per second:" << ops << endl;

    DestroySets();

    return 0;
}
*/
  
