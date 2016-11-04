#pragma once

#ifndef _BIT_HPP_
#define _BIT_HPP_

#include <stdint.h>

#if defined(_MSC_VER)
#	define BIT_INLINE __forceinline
#elif defined(__GNUC__)
#	define BIT_INLINE inline __attribute__((__always_inline__))
#else
#	error __FILE__": unsupported ide, compile stoped!"
#endif

namespace Bit
{
	
	struct UInt128{
		union{
			uint64_t ui64[ 2];
			uint32_t ui32[ 4];
			uint16_t ui16[ 8];
			 uint8_t  ui8[16];
		};
	};
	struct FLOAT{
		union{
			float f;
			struct{
				uint32_t mantissa:23;
				uint32_t exponent:8;
				uint32_t sign:1;
			};
		};
	};
	struct DOUBLE{
		union{
			double d;
			struct{
				uint64_t mantissa:52;
				uint64_t exponent:11;
				uint64_t sign:1;
			};
		};
	};


	enum :uint32_t {
		BITS_PER_UINT8 =8,
		BITS_PER_UINT16=sizeof(uint16_t)/sizeof(uint8_t)*BITS_PER_UINT8,
		BITS_PER_UINT32=sizeof(uint32_t)/sizeof(uint8_t)*BITS_PER_UINT8,
		BITS_PER_UINT64=sizeof(uint64_t)/sizeof(uint8_t)*BITS_PER_UINT8,
		BITS_PER_INT8 =BITS_PER_UINT8,
		BITS_PER_INT16=BITS_PER_UINT16,
		BITS_PER_INT32=BITS_PER_UINT32,
		BITS_PER_INT64=BITS_PER_UINT64,
	};

	
	BIT_INLINE     bool bit_is_set(uint32_t x,uint32_t i){return (x>>i)&1;}
	BIT_INLINE uint32_t bit_set_1(uint32_t& x,uint32_t i){return x|= ((uint32_t)1<<i);}
	BIT_INLINE uint32_t bit_set_0(uint32_t& x,uint32_t i){return x&=~((uint32_t)1<<i);}
	BIT_INLINE uint32_t bit_set_r(uint32_t& x,uint32_t i){return x^= ((uint32_t)1<<i);}
	BIT_INLINE     bool bit_is_set(uint64_t x,uint32_t i){return (x>>i)&1;}
	BIT_INLINE uint64_t bit_set_1(uint64_t& x,uint32_t i){return x|= ((uint64_t)1<<i);}
	BIT_INLINE uint64_t bit_set_0(uint64_t& x,uint32_t i){return x&=~((uint64_t)1<<i);}
	BIT_INLINE uint64_t bit_set_r(uint64_t& x,uint32_t i){return x^= ((uint64_t)1<<i);}


	BIT_INLINE     bool bit_is_odd(uint32_t x){return x&1;}
	BIT_INLINE     bool bit_is_odd(uint64_t x){return x&1;}
	//取x最低1位掩码
	BIT_INLINE uint32_t bit_lowest_1(uint32_t x){return x&((~x)+1);}
	BIT_INLINE uint64_t bit_lowest_1(uint64_t x){return x&((~x)+1);}

	//取x最低0位掩码
	BIT_INLINE uint32_t bit_lowest_0(uint32_t x){return (~x)&(x+1);}
	BIT_INLINE uint64_t bit_lowest_0(uint64_t x){return (~x)&(x+1);}

	//低位0置1
	BIT_INLINE uint32_t bit_invert_low_0(uint32_t x){return x|(x-1);}
	BIT_INLINE uint64_t bit_invert_low_0(uint64_t x){return x|(x-1);}
	//低位1置0
	BIT_INLINE uint32_t bit_invert_low_1(uint32_t x){return x&(x+1);}
	BIT_INLINE uint64_t bit_invert_low_1(uint64_t x){return x&(x+1);}

	//取低位0掩码
	BIT_INLINE uint32_t bit_mask_for_Low_0(uint32_t x){return x^(x-1)^bit_lowest_1(x);}
	BIT_INLINE uint64_t bit_mask_for_Low_0(uint64_t x){return x^(x-1)^bit_lowest_1(x);}
	//取低位1掩码
	BIT_INLINE uint32_t bit_mask_for_Low_1(uint32_t x){return x^(x+1)^bit_lowest_0(x);}
	BIT_INLINE uint64_t bit_mask_for_Low_1(uint64_t x){return x^(x+1)^bit_lowest_0(x);}

	//循环移位
	BIT_INLINE uint32_t bit_rotate_left (uint32_t x,uint32_t n){n&=0x1f;return (x<<n)|(x>>(32-n));}
	BIT_INLINE uint32_t bit_rotate_right(uint32_t x,uint32_t n){n&=0x1f;return (x>>n)|(x<<(32-n));}
	BIT_INLINE uint64_t bit_rotate_left (uint64_t x,uint32_t n){n&=0x3f;return (x<<n)|(x>>(64-n));}
	BIT_INLINE uint64_t bit_rotate_right(uint64_t x,uint32_t n){n&=0x3f;return (x>>n)|(x<<(64-n));}

	//翻转x的bit
	BIT_INLINE uint32_t bit_reverse(uint32_t x){
		x=((x>> 1)&0x55555555)|((x&0x55555555)<< 1);
		x=((x>> 2)&0x33333333)|((x&0x33333333)<< 2);
		x=((x>> 4)&0x0f0f0f0f)|((x&0x0f0f0f0f)<< 4);
		x=((x>> 8)&0x00ff00ff)|((x&0x00ff00ff)<< 8);
		//x=((x>>16)&0x0000ffff)|((x&0x0000ffff)<<16);
		//x=(x&0x0000ffff)+((x>>16)&0x0000ffff);
		x=(x>>16)|(x<<16);
		return x;
	}

	BIT_INLINE uint64_t bit_reverse(uint64_t x){
		uint32_t l=(uint32_t)x,h=(x>>32);
		return ((uint64_t)bit_reverse(l))<<32|bit_reverse(h);
	}
	//计算x末尾0的个数

	BIT_INLINE uint32_t bit_count_trailing_0(uint32_t x){
		static constexpr uint32_t MultiplyDeBruijnBitPosition[]={
			0,1,28,2,29,14,24,3,30,22,20,15,25,17,4,8,
			31,27,13,23,21,19,16,7,26,12,18,6,11,5,10,9
		};
		return MultiplyDeBruijnBitPosition[bit_lowest_1(x)*0x077CB531>>27];
	}

	BIT_INLINE int bit_count_leading_0(uint32_t x){
		return bit_count_trailing_0(bit_reverse(x));
	}

	BIT_INLINE uint32_t bit_count_trailing_0(uint64_t x){
		/*
		BIT_TLS static constexpr int MultiplyDeBruijnBitPosition[]=
		{
			0,1,2,7,3,13,8,19,4,25,14,28,9,34,20,40,5,17,26,38,15,46,29,48,10,31,35,54,21,50,41,57,63,6,
			12,18,24,27,33,39,16,37,45,47,30,53,49,56,62,11,23,32,36,44,52,55,61,22,43,51,60,42,59,58
		};
		return MultiplyDeBruijnBitPosition[Lowest1(x)*0x0218A392CD3D5DBF>>58];
		*/
		uint32_t l=(uint32_t)x,h=(x>>32);
		return (l?bit_count_trailing_0(l):(h?bit_count_trailing_0(h)|32:0));
	}

	BIT_INLINE int bit_count_leading_0(uint64_t x){
		return bit_count_trailing_0(bit_reverse(x));
	}

	//计算1的数目
	BIT_INLINE uint32_t bit_count_1(uint32_t x){
		/*
		x=(x&0x55555555)+((x>> 1)&0x55555555);
		x=(x&0x33333333)+((x>> 2)&0x33333333);
		x=(x&0x0f0f0f0f)+((x>> 4)&0x0f0f0f0f);
		x=(x&0x00ff00ff)+((x>> 8)&0x00ff00ff);
		x=(x&0x0000ffff)+((x>>16)&0x0000ffff);
		*/
		x=x-((x>>1)&0x55555555);
		x=(x&0x33333333)+((x>>2)&0x33333333);
		x=(x+(x>>4))&0x0f0f0f0f;
		x=x*0x01010101>>24;
		return x;
	}
	BIT_INLINE uint32_t bit_count_1(uint64_t x){
		uint32_t l=(uint32_t)x,h=(x>>32);
		return bit_count_1(l)+bit_count_1(h);
	}


	BIT_INLINE uint32_t bit_log2_floor(uint32_t x){
		uint32_t l=0;
		if(x&0xffff0000){l|=16;x>>=16;}
		if(x&0x0000ff00){l|= 8;x>>= 8;}
		if(x&0x000000f0){l|= 4;x>>= 4;}
		if(x&0x0000000c){l|= 2;x>>= 2;}
		if(x&0x00000002){l|= 1;/*x>>= 1;*/}
		return l;
	}

	BIT_INLINE uint32_t bit_log2_floor(uint64_t x){
		uint32_t l=0;
		if(x&0xffffffff00000000){l|=32;x>>=32;}
		l+=bit_log2_floor((uint32_t)x);
		return l;
	}
	BIT_INLINE uint32_t bit_log2_ceil(uint32_t x){
		uint32_t l=bit_log2_floor(x);
		return l+(x>((uint32_t)1<<l));
	}
	BIT_INLINE uint32_t bit_log2_ceil(uint64_t x){
 		uint32_t l=bit_log2_floor(x);
 		return l+(x>((uint64_t)1<<l));
 	}

	BIT_INLINE void bit_swap_value(uint32_t& a,uint32_t& b){
		if(&a==&b) return;
		a^=b;b^=a;a^=b;
	}
	/*
	BIT_INLINE float InvSqrt(float f)
	{
		float x2=f*0.5f;
		UInt32 i=*(UInt32*)&f;// evil floating point bit level hacking
		i=0x5f375a86-(i>>1);// what the fuck?
		float y=*(float*)&i;
		y*=1.5f-x2*y*y;		// 1st iteration
		//y*=1.5f-x2*y*y;	// 2nd iteration, this can be removed
		return y;
	}

	BIT_INLINE double InvSqrt(double d)
	{
		double x2=d*0.5;
		UInt64 i=*(UInt64*)&d;
		i=0x5fe6ec85e7de30da-(i>>1);
		double y=*(double*)&i;
		y*=1.5-x2*y*y;
		//y*=1.5-x2*y*y;
		return y;
	}
	*/


	BIT_INLINE uint32_t bit_sqrt(uint32_t n){
		static constexpr uint16_t sqrtTab[]={
			0x0,0x1,0x1,0x1,0x2,0x2,0x2,0x2,0x2,0x3,0x3,0x3,0x3,0x3,0x3,0x3,
			0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x5,0x5,0x5,0x5,0x5,0x5,0x5,
			0x5,0x5,0x5,0x5,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,0x6,
			0x6,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,0x7,
			256,258,260,262,264,266,268,270,272,274,276,278,280,282,284,286,
			288,288,290,292,294,296,298,300,300,302,304,306,308,310,310,312,
			314,316,318,320,320,322,324,326,326,328,330,332,334,334,336,338,
			340,340,342,344,346,346,348,350,352,352,354,356,356,358,360,362,
			362,364,366,366,368,370,370,372,374,374,376,378,378,380,382,384,
			384,386,386,388,390,390,392,394,394,396,398,398,400,402,402,404,
			406,406,408,408,410,412,412,414,416,416,418,418,420,422,422,424,
			424,426,428,428,430,430,432,434,434,436,436,438,438,440,442,442,
			444,444,446,448,448,450,450,452,452,454,454,456,458,458,460,460,
			462,462,464,464,466,468,468,470,470,472,472,474,474,476,476,478,
			480,480,482,482,484,484,486,486,488,488,490,490,492,492,494,494,
			496,496,498,498,500,500,502,502,504,504,506,506,508,508,510,512
		};
		static constexpr uint32_t Table2[]={
			0<<10,1<<10,4<<10,9<<10,16<<10,25<<10,36<<10,49<<10,
		};

		if(n<64) return sqrtTab[n];

		uint32_t r;
		if(n<65536){
			r=sqrtTab[n>>10];
			n-=Table2[r];
			r<<=10;
			uint32_t c=0x100|r;r>>=1;
			if(n>=c){n-=c;r|=0x100;}
			c=0x40|r;r>>=1;
			if(n>=c){n-=c;r|=0x40;}
			c=0x10|r;r>>=1;
			if(n>=c){n-=c;r|=0x10;}
			c=0x04|r;r>>=1;
			if(n>=c){n-=c;r|=0x04;}
			c=0x01|r;r>>=1;
			if(n>=c) r|=0x01;
			return r;
		}

		switch(bit_log2_floor(n)&0xfe){
			case 16:r=sqrtTab[n>>10]   ;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 18:r=sqrtTab[n>>12]<<1;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 20:r=sqrtTab[n>>14]<<2;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 22:r=sqrtTab[n>>16]<<3;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 24:r=sqrtTab[n>>18]<<4;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 26:r=sqrtTab[n>>20]<<5;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 28:r=sqrtTab[n>>22]<<6;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
			case 30:r=sqrtTab[n>>24]<<7;{r=(r+n/r)>>1;if(r*r>n) --r;break;}
		}
		return r;
	}

//////////////////////////////////////////////////////////////////////////
	template<uint32_t x>
	class bit_log2_floor_static
	{
		enum
		{
			x32=x,
			l32=0,

			r16=(x32>>16)&0x0000ffff,
			l16=l32+(r16?16:0),
			x16=(r16?r16:x32),

			r08=(x16>> 8)&0x000000ff,
			l08=l16+(r08? 8:0),
			x08=(r08?r08:x16),

			r04=(x08>> 4)&0x0000000f,
			l04=l08+(r04? 4:0),
			x04=(r04?r04:x08),

			r02=(x04>> 2)&0x00000003,
			l02=l04+(r02? 2:0),
			x02=(r02?r02:x04),

			r01=(x02>> 1)&0x00000001,
			l01=l02+(r01? 1:0),
			x01=(r01?r01:x02),
		};
	public:
		enum{value=l01,};
	};
	template<uint32_t x>
	class bit_log2_ceil_static
	{
		enum{l=bit_log2_floor_static<x>::value,};
	public:
		enum{value=l+(x>(1u<<l)),};
	};

//////////////////////////////////////////////////////////////////////////

};


#endif
