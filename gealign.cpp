//
// gealign.cpp : a utility to demonstrate performance tuning by Raymond Zhang。
// I started this utility to test effects of different alignment, but later I 
// added some other tests such as mul/div etc. for the C++ Conferenece 2020. 
//

#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <vector>
#include <string.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#include <math.h>
#endif
#include <numeric>

using namespace std;
struct alignas(64) GE
{
    double a[8];
};
struct GE2
{
    double b[10]; // 0n80
};

struct  test_t
{
    double a;
    int b;
    char h;
    uint64_t c;
    char d;
    //alignas(64) GE  ge;
    GE  ge;
    int b2b;
    int b2c;
    GE2 ge2;
};

//#pragma pack(show)   // C4810 //alignas(64)
struct  school
{
    double a;
    int b;
    char h;
    uint64_t c;
    char d;
};

#pragma pack (push)
#pragma pack (1)
struct school_pack1
{
    double a;
    int aligned_n;
    char h;
    int unaligned_n;
    uint64_t c;
    char d[7];
};


struct alignas(64) box_t
{
    int l,t,r,b;
};

//#pragma pack(show)   // C4810
#pragma pack (pop)
//#pragma pack(show)   // C4810
struct S {
    int i;   // size 4
    short j;   // size 2
    double k;   // size 8
};

//#pragma pack(show)   // C4810
#pragma pack(2)
struct T {
    int i;
    short j;
    double k;
};
const int BLOCK_SIZE = 64;

int  ge_fill_sum(std::vector<int*> blocks, const char * name)
{
    int64_t start = __rdtsc();
    int sum=0;
    for (auto it : blocks)
    {
        std::iota(it, it + BLOCK_SIZE, 0);
        for (int i = 0; i < BLOCK_SIZE; i++)
        {
            sum += *(it + i);
        }
    }
    int64_t end = __rdtsc();

    std::cout << "fill and sum " << name << " took " <<
        end - start << " ticks " << ". sum = "<< sum << std::endl;

    return sum;
}

int tryalignalloc(int n)
{
    std::vector<int*> vec_na_blocks; // not aligned mallocs
    std::vector<int*> vec_al_blocks; // aligned mallocs

    for(int i=0;i<n;i++)
    { 
        int* p1 = static_cast<int*>(std::malloc(BLOCK_SIZE * sizeof(int)));
        vec_na_blocks.push_back(p1);
        std::printf("default-aligned address:   %p\n", static_cast<void*>(p1));
    }

    for (int i = 0; i < n; i++)
    {
#ifndef _MSC_VER 
        int* p2 = static_cast<int*>(std::aligned_alloc(1024, 1024));
#else
        int* p2 = static_cast<int*>(_aligned_malloc(BLOCK_SIZE *sizeof(int), 64));
#endif
        vec_al_blocks.push_back(p2);
        std::printf("64-byte aligned address: %p\n", static_cast<void*>(p2));
    }
    //
    // try accessing the 
    //
    ge_fill_sum(vec_na_blocks, "notaligned");
    ge_fill_sum(vec_al_blocks, "aligned");

    for(auto it: vec_na_blocks)
    { 
        std::free(it);
    }
    for (auto it : vec_al_blocks)
    {
#ifndef _MSC_VER 
        free(it);
#else
        _aligned_free(it);
#endif
    }

    return 0;
}

int ge_add(int a, int count)
{
    int ret = 0;

    uint64_t start = __rdtsc();
    for (int i = 0; i < count; i++)
        ret += (a+i);
    uint64_t end = __rdtsc();

    cout << "sum of " << a << " + i times = " << ret << " took " << end - start << " ticks " << endl;
    return ret;
}

int ge_mul(int a, int count)
{
    int ret = 0;

    uint64_t start = __rdtsc();
    for (int i = 0; i < count; i++)
        ret += (a * i);
    uint64_t end = __rdtsc();

    cout << "sum of " << a << "*i times = " << ret << " took " << end - start << " ticks " << endl;
    return ret;
}

int ge_div(int a, int count)
{
    int ret = 0;

    uint64_t start = __rdtsc();
    for (int i = 0; i < count; i++)
        ret += (a / (i+1));
    uint64_t end = __rdtsc();

    cout << "sum of " << a << "/(i+1) times = " << ret << " took " << end - start << " ticks " << endl;
    return ret;
}

int ge_sum_aligned(school_pack1* sc1, school_pack1* sc2, int count)
{
    int ret = 0;

    uint64_t start = __rdtsc();
    for (int i = 0; i < count; i++)
        ret += sc1[i].aligned_n;
    uint64_t end = __rdtsc();

    cout << "sum   aligned n " << count << " times= " << ret << " took " << end - start << " ticks " << endl;

    ret = 0;
    start = __rdtsc();
    for (int i = 0; i < count; i++)
        ret += sc2[i].unaligned_n;
    end = __rdtsc();

    cout << "sum unaligned n " << count << " times= "<<ret<<" took " << end - start << " ticks " << endl;
    return ret;
}

#define SC_SIZE 100000
template<typename T>
int ge_pack(T* sc, int count, const char * name)
{
    int sum = 0;
    std::vector<T> schools;

    uint64_t start = __rdtsc();
#ifdef USE_STL
    for (int i = 0; i < count; i++)
    {
        sc.c = i;
        schools.push_back(sc);
    }
    for (auto it : schools)
    {
        sum += sc.c+ sc.c;
    }
#else
    for (int i = 0; i < count; i++)
    {
        sc[i].c = count * i;
    }
    for (int i = 0; i < count; i++)
    {
        sum += sc[i].c;
    }
#endif
    uint64_t end = __rdtsc();

    cout << "assign and sum " << name << " took " << end - start << " ticks " << endl;

    return sum;
}

typedef struct tag_odd_pak{ 
    char tag;
    char bytes;
    char idx; 
    int data_array_n[BLOCK_SIZE] = {1,2,3};
}odd_pak;
odd_pak g_opak;
alignas(64) int data_array_a[BLOCK_SIZE] = {1,2,3};

int ge_calc_align(int seed)
{
    int sum = 0;

    memset(data_array_a, 1, BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; i++)
        sum += data_array_a[i];

    return sum;
}

int ge_calc_noalign(int seed)
{
    int sum = 0;

    memset(g_opak.data_array_n, 1, BLOCK_SIZE);
    for (int i = 0; i < BLOCK_SIZE; i++)
        sum += g_opak.data_array_n[i];

    return sum;
}
int ge_alignas(int count)
{
    int i;
    struct box_t box;
    uint64_t start, end, sum=0;
    sum = 0;
    cout << "addr of data_array_n " << &g_opak.data_array_n << " align of data_array_n is " 
        //<< alignof(g_opak.data_array_n)
        << endl;
    start = __rdtsc();
    for (int n = 0; n < count; n++)
        sum += ge_calc_noalign(n);
    end = __rdtsc();

    cout << "Invoked " << count << " ge_calc_noalign() took " 
        << end - start << " ticks. " << sum << endl;

    sum = 0;
    //cout << "addr of data_array_a " << &g_opak.data_array_n << " align of data_array_a is " << alignof(data_array_n) << endl;
    start = __rdtsc();
    for (int n = 0; n < count; n++)
        sum += ge_calc_align(n);
    end = __rdtsc();
    cout << "Invoked " << count << " ge_calc_align()   took "
        << end - start << " ticks. " << sum << endl;
    return 0;
}

float ge_mul_div(int count)
{
    int ret = 0;
    uint64_t start, end;
    float sum = 0;

    start = __rdtsc();
    for (int n = 0; n < count; n++)
        sum += (n*0.01);
    end = __rdtsc();

    cout << "sum of i*0.01 " << count << " took " << end - start << " ticks. " << sum << endl;

    sum = 0;
    start = __rdtsc();
    for (int n = 0; n < count; n++)
        sum += (n / 100.0);
    end = __rdtsc();

    cout << "sum of i/100.0 " << count << " took " << end - start << " ticks. " << sum << endl;
    return sum;
}

float ge_mul_div_interlace(int count)
{
    int ret = 0;
    uint64_t start, end;
    float sum = 0;

    start = __rdtsc();
    for (int n = 0; n < count; n++)
    {
        sum += (n * 0.01);
        sum += (n / 100.0);
    }
    end = __rdtsc();


    cout << "mul div interlace " << count << " times took " << end - start << " ticks. " << sum << endl;
    return sum;
}

float ge_mul_div_interlace_para(int count)
{
    int ret = 0;
    uint64_t start, end;
    float sum1 = 0, sum2 =0;

    start = __rdtsc();
    for (int n = 0; n < count; n++)
    {
        sum1 += (n * 0.01);
        sum2 += (n / 100.0);
    }
    end = __rdtsc();

    cout << "parallel interlace " << count << " times took " << end - start << " ticks. " << sum1 + sum2 << endl;
    return sum1+sum2;
}

// most stupid method to determine prime no
bool is_prime(uint64_t n)
{
    uint64_t start, end, qroot;
    uint64_t remains, counter = 0;

    qroot = sqrt(n);
    start = __rdtsc();
    for (uint64_t i = 2; i < qroot; i++) {
        remains = (n % i);
        counter += (remains == 0);
    }
    end = __rdtsc();

    cout << "Most stupid prime checker: " << n << " has " << counter << " dividers. " << endl;
    cout << "It took " << end - start << " ticks." << endl;

    return counter == 0;
}

const int GE_ARRAY_SIZE = 1000;
double ge_array1[GE_ARRAY_SIZE];
double ge_array2[GE_ARRAY_SIZE];
double ge_array3[GE_ARRAY_SIZE];
int crazy_div_double(int count)
{
    int result = 0;
    uint64_t start, end;
    start = __rdtsc();
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < GE_ARRAY_SIZE; j++) {
            ge_array3[j] = ge_array1[j] / (ge_array2[j] + j + 1);
            result += ge_array3[j] > 10000.0 ? 1 : -1;
        }
    }
    end = __rdtsc();
    cout << "It took " << end - start << " ticks for "<< count * GE_ARRAY_SIZE  <<" double divides， aver " <<
        (end - start)/(float)(count* GE_ARRAY_SIZE) << endl;
    return result;
}

int ge_array_int1[GE_ARRAY_SIZE];
int ge_array_int2[GE_ARRAY_SIZE];
int ge_array_int3[GE_ARRAY_SIZE];
int crazy_div_integer(int count)
{
    int result = 0;
    uint64_t start, end;
    start = __rdtsc();
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < GE_ARRAY_SIZE; j++) {
            ge_array_int3[j] = ge_array_int1[j] / (ge_array_int2[j] + j + 1);
            result += ge_array_int3[j] > 10000 ? 1 : -1;
        }
    }
    end = __rdtsc();
    cout << "It took " << end - start << " ticks for " << count * GE_ARRAY_SIZE << " integer divides， aver " <<
        (end - start) / (float)(count * GE_ARRAY_SIZE) << endl;
    return result;
}
int crazy_div_double_depends(int count)
{
    int result = 0;
    uint64_t start, end;
    start = __rdtsc();
    for (int i = 0; i < count; i++) {
        for (int j = 1; j < GE_ARRAY_SIZE; j++) {
            ge_array3[j] = ge_array1[j] / (ge_array3[j-1] + j + 1);
            result += ge_array3[j] > 10000.0 ? 1 : -1;
        }
    }
    end = __rdtsc();
    cout << "It took " << end - start << " ticks for " << count * GE_ARRAY_SIZE << " double divides， aver " <<
        (end - start) / (float)(count * GE_ARRAY_SIZE) << endl;
    return result;
}
int main(int argc, char ** argv)
{
    uint64_t r=0;
    test_t students;
    int labno = 1;
    int count = 100000;

    printf("gealign by Raymond rev 0.2\n"
        "gealign [labno] [repeat times]\n");

    if ((argc > 1))
        labno = atoi(argv[1]);
    if(argc > 2)
        count = atoi(argv[2]);
    school_pack1* scp1 = (school_pack1*)malloc(count * sizeof(school_pack1));
    school_pack1* scp2 = (school_pack1*)malloc(count * sizeof(school_pack1));
    school* sc = (school*)malloc(count * sizeof(school));
    if (scp1 == nullptr || scp2 == nullptr || sc == nullptr)
    {
        printf("failed to allocate buffer.");
        return -1;
    }
    for (int i = 0; i < count; i++)
    {
        scp1[i].aligned_n = i;
        scp1[i].unaligned_n = i;
        scp2[i].aligned_n = i;
        scp2[i].unaligned_n = i;
    }

    switch (labno)
    {
    case 0:
        printf("size of struct %ld \n", sizeof(students));

        printf("%zu ", offsetof(S, i));
        printf("%zu ", offsetof(S, j));
        printf("%zu\n", offsetof(S, k));
        //printf("%zu\n", offsetof(S, h));

        printf("%zu ", offsetof(T, i));
        printf("%zu ", offsetof(T, j));
        printf("%zu\n", offsetof(T, k));

        printf("offset of c %zu\n", offsetof(school_pack1, c));
        break;
    case 1:
    {
        r = ge_pack<school_pack1>(scp1, count, "packed");
        cout << "Sum =" << r << endl;
        r = ge_pack<school>(sc, count, "boundary aligned");
        cout << "Sum =" << r << endl;

        break;
    }
    case 2:
        r = ge_sum_aligned(scp1, scp2, count);
        break;
    case 4:
        tryalignalloc(count);
        break;
    case 5:
        ge_alignas(count);
        break;
    case 6:
        ge_add(1888888, count);
        ge_mul(1888888, count);
        ge_div(1888888, count);
        ge_add(0, count);
        ge_mul(0, count);
        ge_div(0, count);
        break;
    case 7:
        r = ge_mul_div(count);
        r = ge_mul_div(count);
        r = ge_mul_div(count);
        break;
    case 8:
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div_interlace_para(count);
        break;
    case 9:
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div_interlace_para(count);
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div_interlace_para(count);
        r = ge_mul_div(count);
        r = ge_mul_div_interlace(count);
        r = ge_mul_div_interlace_para(count);
        break;
    case 10:
        is_prime((uint64_t)-1);
        break;
    case 11:
        r = crazy_div_double(count);
        break;
    case 12:
        r = crazy_div_integer(count);
        break;
    case 13:
        crazy_div_double_depends(count);
        break;
    }
    free(sc);
    free(scp1);
    free(scp2);
    return r;
}
