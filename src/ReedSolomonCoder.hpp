#pragma once

#ifndef _REEDSOLOMONCODER_HPP_
#define _REEDSOLOMONCODER_HPP_

#include <assert.h>
#include <stdarg.h>
#include <algorithm>
#include <utility>
#include "Bit.hpp"
#include "MemPool.hpp"

namespace ErrorCorrectingCodes
{

	template<uint32_t _Prime, uint32_t _Omega>
	class CGFPrime//Galois field
	{
	public:
		enum :uint32_t {
			P = _Prime,
			PhiP = P - 1,
			W = _Omega,
		};

		typedef uint32_t IdxType;
		typedef uint32_t ValType;

		struct NumType {
			ValType uiValue;
		};
		BIT_INLINE friend bool operator==(NumType a, NumType b) { return a.uiValue == b.uiValue; }
		BIT_INLINE friend bool operator!=(NumType a, NumType b) { return a.uiValue != b.uiValue; }
		BIT_INLINE static NumType Num(ValType v) { return *(NumType*)&v; }

	protected:
		NumType m_pFMap[PhiP * 2];
		IdxType m_pIMap[PhiP + 1];
	public:
		BIT_INLINE static NumType UnitElement() { return Num(1); }
		BIT_INLINE static NumType ZeroElement() { return Num(0); }

		BIT_INLINE friend NumType operator+(NumType a, NumType b) {
			return Num((a.uiValue + b.uiValue) % P);
		}
		BIT_INLINE friend NumType operator-(NumType a, NumType b) {
			return Num((a.uiValue + (P - b.uiValue)) % P);
		}
		BIT_INLINE friend NumType operator*(NumType a, NumType b) {
			if (PhiP<0x10000) {
				return Num(a.uiValue*b.uiValue%P);
			}
			else if (PhiP == 0x10000) {
				return (a.uiValue&b.uiValue & 0x10000) ? UnitElement() : Num(a.uiValue*b.uiValue%P);
			}
			else {
				return (a == ZeroElement() || b == ZeroElement()) ? ZeroElement() : Exp(Log(a) + Log(b));
			}
		}
		BIT_INLINE friend NumType operator/(NumType a, NumType b) {
			assert(b != ZeroElement());
			return (a == ZeroElement()) ? ZeroElement() : Exp(Log(a) + PhiP - Log(b));
		}

		BIT_INLINE static NumType Inv(NumType x) {
			return Exp(PhiP - Log(x));
		}

		BIT_INLINE static IdxType Log(NumType x) {
			assert(x != ZeroElement());
			static const IdxType* pIMap = GetInstance()->m_pIMap;
			return pIMap[x.uiValue];
		}

		BIT_INLINE static NumType Exp(IdxType Power) {
			static const NumType* pFMap = GetInstance()->m_pFMap;
			return pFMap[Power];
		}
	private:
		static CGFPrime* GetInstance() {
			static CGFPrime gf;
			return &gf;
		}
		CGFPrime() {
			m_pFMap[0] = UnitElement();
			for (uint32_t i = 1; i<PhiP * 2; i++) m_pFMap[i] = m_pFMap[i - 1] * Num(W);
			//////////////////////////////////////////////////////////////////////////
			m_pIMap[0] = -1;
			for (uint32_t i = 1; i <= PhiP; i++) m_pIMap[m_pFMap[i].uiValue] = i;
		}
	};


	template<uint32_t _FermatOrdinal>
	class CFNT
	{
	public:
		enum :uint32_t {
			FermatOrdinal = _FermatOrdinal,
			FermatNumber = (1 << (1 << FermatOrdinal)) + 1,
		};

		typedef CGFPrime<FermatNumber, 3>	CGFPrime;
		typedef typename CGFPrime::NumType	NumType;

		enum :uint32_t {
			MaxN = CGFPrime::PhiP,
			MaxM = 1 << FermatOrdinal,
		};

		static void FNT(NumType Data[], uint32_t Len, bool Reverse = false)
		{
			uint32_t N = Len;
			uint32_t M = Bit::bit_log2_floor(Len);
			assert(N == ((uint32_t)1 << M));
			assert(N <= MaxN);

			for (uint32_t Level = 0; Level<M; Level++) {
				uint32_t Length = (1 << (M - (Level + 1)));
				uint32_t Rate = MaxM - M + Level;
				for (uint32_t Base = 0; Base<N; Base += (Length << 1))
					for (uint32_t Offset = 0; Offset<Length; Offset++) {
						uint32_t i = Base + Offset;
						uint32_t j = i + Length;
						NumType t = Data[i];
						Data[i] = t + Data[j];
						Data[j] = (t - Data[j])*CGFPrime::Exp(Offset << Rate);
					}
			}

			if (Reverse) {
				for (uint32_t i = 0; i<N; i++) {
					uint32_t j = Bit::bit_reverse(i) >> (32 - M);
					if (j>i) Bit::bit_swap_value(Data[j].uiValue, Data[i].uiValue);
				}
			}
		}
		static void IFNT(NumType Data[], uint32_t Len, bool Reverse = false)
		{
			uint32_t N = Len;
			uint32_t M = Bit::bit_log2_floor(Len);
			assert(N == ((uint32_t)1 << M));
			assert(N <= MaxN);

			if (Reverse) {
				for (uint32_t i = 0; i<N; i++) {
					uint32_t j = Bit::bit_reverse(i) >> (32 - M);
					if (j>i) Bit::bit_swap_value(Data[j].uiValue, Data[i].uiValue);
				}
			}

			for (uint32_t Level = M - 1; Level != (uint32_t)-1; Level--) {
				uint32_t Length = (1 << (M - (Level + 1)));
				uint32_t Rate = MaxM - M + Level;
				for (uint32_t Base = 0; Base<N; Base += (Length << 1))
					for (uint32_t Offset = 0; Offset<Length; Offset++) {
						uint32_t i = Base + Offset;
						uint32_t j = i + Length;
						NumType t = Data[j] * CGFPrime::Exp(MaxN - (Offset << Rate));
						Data[j] = Data[i] - t;
						Data[i] = Data[i] + t;
					}
			}
			NumType INV_N = CGFPrime::UnitElement() / CGFPrime::Num(N);
			for (uint32_t i = 0; i<N; i++) Data[i] = Data[i] * INV_N;
		}
	};


	template<uint32_t _MaxDegree, typename CFNT>
	class CPoly
	{
	public:
		typedef typename CFNT::CGFPrime		CGFPrime;
		typedef typename CGFPrime::NumType	NumType;

		enum :uint32_t {
			MaxDeg = _MaxDegree,
		};
		typedef MemPool::CFixedMemPool<sizeof(NumType[MaxDeg + 1])> MemPool;

		uint32_t m_uiDegree;
		mutable NumType* m_pCoeffs;//[MaxDeg+1];//Coefficients

		CPoly(NumType Coeffs[], uint32_t Degree) {
			m_pCoeffs = Alloc();
			m_uiDegree = Degree;
			assert(m_uiDegree <= MaxDeg);
			for (uint32_t i = 0; i <= Degree; i++) m_pCoeffs[i] = Coeffs[i];
			Modify();
		}
		CPoly(NumType FillCoeff = CGFPrime::ZeroElement(), uint32_t Degree = 0) {
			m_pCoeffs = Alloc();
			m_uiDegree = Degree;
			assert(m_uiDegree <= MaxDeg);
			for (uint32_t i = 0; i <= Degree; i++) m_pCoeffs[i] = FillCoeff;
			Modify();
		}
		CPoly(uint32_t Degree, NumType Coeff, ...) {
			m_pCoeffs = Alloc();
			m_uiDegree = Degree;
			assert(m_uiDegree <= MaxDeg);

			va_list arg_ptr;
			va_start(arg_ptr, Coeff);

			m_pCoeffs[0] = Coeff;
			for (uint32_t i = 1; i <= Degree; i++) {
				Coeff = va_arg(arg_ptr, NumType);
				m_pCoeffs[i] = Coeff;
			}
			va_end(arg_ptr);

			Modify();
		}
		CPoly(const CPoly& A) {
			new(this) CPoly(A.m_pCoeffs, A.m_uiDegree);
		}
		CPoly(/*const*/ CPoly&& A) {
			m_pCoeffs = nullptr;
			m_uiDegree = 0;
			std::swap(m_pCoeffs, A.m_pCoeffs);
			std::swap(m_uiDegree, A.m_uiDegree);
		}
		~CPoly() {
			if (m_pCoeffs != nullptr) {
				Free(m_pCoeffs);
				m_pCoeffs = nullptr;
			}
		}

		NumType& operator[](uint32_t dwIndex)const {
			thread_local static NumType dummy;
			assert(dwIndex <= MaxDeg);
			return (dwIndex <= m_uiDegree ? m_pCoeffs[dwIndex] : (dummy = CGFPrime::ZeroElement()));
		}
		CPoly& operator=(const CPoly& A) {
			if (&A != this) {
				this->~CPoly();
				new(this) CPoly(A);
			}
			return *this;
		}
		CPoly& operator=(/*const*/ CPoly&& A) {
			if (&A != this) {
				this->~CPoly();
				new(this) CPoly(std::move(A));
			}
			return *this;
		}
		inline bool IsZero() {
			return m_uiDegree == 0 && m_pCoeffs[0] == CGFPrime::ZeroElement();
		}
	protected:
		static MemPool* GetMemPool() {
			thread_local static MemPool mp;
			return &mp;
		}

		static NumType* Alloc() {
			return (NumType*)GetMemPool()->Alloc();
			//return new NumType[MaxDeg+1];
		}
		static void Free(NumType* p) {
			GetMemPool()->Free(*(typename MemPool::MemData*)p);
			//delete[] p;
		}

		static CPoly Rev(const CPoly& A, uint32_t K)
		{
			CPoly B;
			B.m_uiDegree = std::min(A.m_uiDegree, K);
			for (uint32_t i = 0; i <= B.m_uiDegree; i++)
				B[i] = A[A.m_uiDegree - i];
			B.Modify();
			return B;
		}
	public:
		void Modify() {
			while (m_uiDegree>0 && (*this)[m_uiDegree] == CGFPrime::ZeroElement())
				m_uiDegree--;
		}
		static CPoly ModX(const CPoly& A, uint32_t K)//mod x^k
		{
			CPoly B;
			if (K)
			{
				B.m_uiDegree = std::min(A.m_uiDegree, K - 1);
				for (uint32_t i = 0; i <= B.m_uiDegree; i++) B[i] = A[i];
				B.Modify();
			}
			return B;
		}
		static CPoly Der(const CPoly& A)//derivation
		{
			CPoly B;
			if (A.m_uiDegree >= 1)
			{
				B.m_uiDegree = A.m_uiDegree - 1;
				for (uint32_t i = 1; i <= A.m_uiDegree; i++) B[i - 1] = CGFPrime::Num(i)*A[i];
				B.Modify();
			}
			return B;
		}

		static CPoly UnitElement() { return CPoly(CGFPrime::UnitElement(), 0); }
		static CPoly ZeroElement() { return CPoly(CGFPrime::ZeroElement(), 0); }
		friend CPoly operator>>(const CPoly& A, uint32_t K)
		{
			if (A.m_uiDegree >= K)
				return CPoly(A.m_pCoeffs + K, A.m_uiDegree - K);
			return CPoly();
		}
		friend CPoly operator<<(const CPoly& A, uint32_t K)
		{
			CPoly C;
			C.m_uiDegree = A.m_uiDegree + K;
			assert(C.m_uiDegree <= MaxDeg);
			for (uint32_t i = 0; i<K; i++) C[i] = CGFPrime::ZeroElement();
			for (uint32_t i = 0; i <= A.m_uiDegree; i++) C[i + K] = A[i];
			return C;
		}
		friend CPoly operator+(const CPoly& A, const CPoly& B)//C=A+B
		{
			CPoly C;
			C.m_uiDegree = std::max(A.m_uiDegree, B.m_uiDegree);
			for (uint32_t i = 0; i <= C.m_uiDegree; i++)
				C[i] = A[i] + B[i];
			C.Modify();
			return C;
		}
		friend CPoly operator-(const CPoly& A, const CPoly& B)//C=A-B
		{
			CPoly C;
			C.m_uiDegree = std::max(A.m_uiDegree, B.m_uiDegree);
			for (uint32_t i = 0; i <= C.m_uiDegree; i++)
				C[i] = A[i] - B[i];
			C.Modify();
			return C;
		}
		friend CPoly operator*(const CPoly& A, const CPoly& B)//c=a*b
		{
			CPoly C;
			C.m_uiDegree = A.m_uiDegree + B.m_uiDegree;
			assert(C.m_uiDegree <= MaxDeg);
			uint32_t M = Bit::bit_log2_ceil(C.m_uiDegree + 1);
			uint32_t N = (1 << M);
			uint32_t fnt_mul_cost = N*M*(&A == &B ? 2 : 3);
			uint32_t direct_mul_cost = (A.m_uiDegree + 1)*(B.m_uiDegree + 1);
			if (N>CFNT::MaxN || direct_mul_cost<fnt_mul_cost || std::min(A.m_uiDegree, B.m_uiDegree)<32)
			{
				for (uint32_t i = 0; i <= C.m_uiDegree; i++)
					C[i] = CGFPrime::ZeroElement();
				for (uint32_t i = 0; i <= A.m_uiDegree; i++)
					for (uint32_t j = 0; j <= B.m_uiDegree; j++)
						C[i + j] = C[i + j] + A[i] * B[j];
			}
			else
			{
				NumType* Arr1 = C.m_pCoeffs;
				for (uint32_t i = 0; i<N; i++) Arr1[i] = A[i];
				CFNT::FNT(Arr1, N);
				if (&A == &B) {
					for (uint32_t i = 0; i<N; i++) Arr1[i] = Arr1[i] * Arr1[i];
				}
				else {
					NumType Arr2[MaxDeg + 1];
					for (uint32_t i = 0; i<N; i++) Arr2[i] = B[i];
					CFNT::FNT(Arr2, N);
					for (uint32_t i = 0; i<N; i++) Arr1[i] = Arr1[i] * Arr2[i];
				}
				CFNT::IFNT(Arr1, N);
			}
			C.Modify();
			return C;
		}

		friend CPoly operator/(const CPoly& A, const CPoly& B)//C=A/B
		{
			if (B.m_uiDegree>A.m_uiDegree)
				return ZeroElement();

			uint32_t Delta = A.m_uiDegree - B.m_uiDegree;
			uint32_t M = Bit::bit_log2_ceil(Delta + 1) + 1;
			uint32_t N = 1 << M;

			uint32_t newton_div_cost = 9 * N*M;
			uint32_t direct_div_cost = (B.m_uiDegree + 1)*(A.m_uiDegree - B.m_uiDegree + 1);

			if (direct_div_cost<newton_div_cost || std::min(A.m_uiDegree - B.m_uiDegree, B.m_uiDegree)<32)
			{
				CPoly C, D = A;
				C.m_uiDegree = A.m_uiDegree - B.m_uiDegree;
				for (uint32_t i = D.m_uiDegree; i != B.m_uiDegree - 1; i--)
				{
					uint32_t k = i - B.m_uiDegree;
					C[k] = D[i] / B[B.m_uiDegree];
					for (uint32_t j = 0; j <= B.m_uiDegree; j++)
						D[k + j] = D[k + j] - B[j] * C[k];
				}
				C.Modify();
				return C;
			}
			else if (N <= CFNT::MaxN)
			{
				CPoly RevB = Rev(B, B.m_uiDegree);
				uint32_t Degree = RevB.m_uiDegree;
				CPoly RecB(CGFPrime::UnitElement() / RevB[0], 0);

				for (uint32_t i = 1, j = 1 << i; i<M; j = 1 << ++i)
				{
					RevB.m_uiDegree = std::min(Degree, j - 1);
					CPoly Tmp;
					//CPoly tmp=Mul(Mul(RecB,RecB),RevB);
					//tmp.m_uiDegree=min(tmp.m_uiDegree,j-1);
					{
						Tmp.m_uiDegree = j - 1;
						uint32_t L = 2 * j;
						NumType* Arr1 = Tmp.m_pCoeffs;
						for (uint32_t i = 0; i<L; i++) Arr1[i] = RecB[i];
						CFNT::FNT(Arr1, L);
						NumType Arr2[MaxDeg + 1];
						for (uint32_t i = 0; i<L; i++) Arr2[i] = RevB[i];
						CFNT::FNT(Arr2, L);
						for (uint32_t i = 0; i<L; i++) Arr1[i] = Arr1[i] * Arr1[i] * Arr2[i];
						CFNT::IFNT(Arr1, L);
						Tmp.Modify();
					}
					RecB = RecB + RecB - Tmp;
				}

				CPoly RevA = Rev(A, Delta);
				CPoly RevQ = RevA*RecB;
				RevQ.m_uiDegree = Delta;
				return Rev(RevQ, Delta);
			}
			else
			{
				uint32_t K = (A.m_uiDegree + 1) / 2;
				CPoly HA = A >> K;
				CPoly LA = ModX(A, K);
				CPoly Q = HA / B;
				CPoly R = HA - Q*B;
				return (Q << K) + ((R << K) + LA) / B;
			}
		}
		friend CPoly operator%(const CPoly& A, const CPoly& B)
		{
			if (B.m_uiDegree>A.m_uiDegree)
				return A;

			uint32_t Delta = A.m_uiDegree - B.m_uiDegree;
			uint32_t M = Bit::bit_log2_ceil(Delta + 1) + 1;
			uint32_t N = 1 << M;

			uint32_t newton_div_cost = 9 * N*M;
			uint32_t direct_div_cost = (B.m_uiDegree + 1)*(A.m_uiDegree - B.m_uiDegree + 1);

			if (direct_div_cost<newton_div_cost)
			{
				CPoly D = A;
				for (uint32_t i = D.m_uiDegree; i != B.m_uiDegree - 1; i--)
				{
					uint32_t k = i - B.m_uiDegree;
					NumType tmp = D[i] / B[B.m_uiDegree];
					for (uint32_t j = 0; j <= B.m_uiDegree; j++)
						D[k + j] = D[k + j] - B[j] * tmp;
				}
				D.Modify();
				return D;
			}
			else if (N <= CFNT::MaxN) {
				return A - (A / B)*B;
			}
			else
			{
				uint32_t K = (A.m_uiDegree + 1) / 2;
				CPoly HA = A >> K;
				CPoly LA = ModX(A, K);
				CPoly R = HA%B;
				return ((R << K) + LA) % B;
			}
		}
	};


	template<uint32_t _Rows, uint32_t _Cols, typename CellType>
	class CMatrix
	{
	public:
		enum :uint32_t {
			R = _Rows,
			C = _Cols,
		};
		typedef CellType(CMatRow)[C];

		mutable CellType Mat[R][C];

		CMatrix() {}
		CMatrix(CMatrix& A) {
			for (uint32_t i = 0; i<R; i++)
				for (uint32_t j = 0; j<C; j++)
					(*this)[i][j] = A[i][j];
		}
		CMatrix(CMatrix&& A) {
			for (uint32_t i = 0; i<R; i++)
				for (uint32_t j = 0; j<C; j++)
					(*this)[i][j] = std::move(A[i][j]);
		}

		BIT_INLINE CMatRow& operator[](uint32_t Row)const {
			assert(Row<R);
			return Mat[Row];
		}

		CMatrix& operator=(const CMatrix& A) {
			if (&A != this) {
				this->~CMatrix();
				new(this) CMatrix(A);
			}
			return *this;
		}
		CMatrix& operator=(/*const*/ CMatrix&& A) {
			if (&A != this) {
				this->~CMatrix();
				new(this) CMatrix(std::move(A));
			}
			return *this;
		}
		/*
		static CMat<R,C,CellType> E()
		{
		assert(R==C);
		CMat<R,C,CellType> M;
		for(UInt32 i=0;i<R;i++)
		for(UInt32 j=0;j<C;j++)
		M[i][j]=(i==j?CellType::UnitElement():CellType::ZeroElement());
		return M;
		}*/
		template<uint32_t NewC>
		CMatrix<R, NewC, CellType> operator*(const CMatrix<C, NewC, CellType>& A)
		{
			CMatrix<R, NewC, CellType> M;
			for (uint32_t i = 0; i<R; i++)
				for (uint32_t k = 0; k<NewC; k++)
					for (uint32_t j = 0; j < C; j++) {
						if(j==0)
							M[i][k] = (*this)[i][j] * A[j][k];
						else
							M[i][k] = M[i][k] + (*this)[i][j] * A[j][k];
					}
						
			return M;
		}
	};

	class IReedSolomonCoder
	{
	public:
		enum :uint32_t {
			ECC_NOERROR = 0,
			ECC_SUCCESS = 1,
			ECC_FAILED = 2,
		};
		// Decode return: 0 for no error;1 for ecc success;2 for ecc failed
		virtual ~IReedSolomonCoder() {}

		virtual		bool Init() = 0;

		virtual		void EncodeT(const void* arrData, void* arrECC) = 0;
		virtual uint32_t DecodeT(void* arrData, const void* arrECC) = 0;

		virtual		void EncodeF(const void* inArr, void* outArr) = 0;
		virtual uint32_t DecodeF(const void* inArr, void* outArr) = 0;
	};

	template<typename _CodeWordType>
	class IReedSolomonCoder2 :public IReedSolomonCoder
	{
	public:
		typedef _CodeWordType CodeWordType;
		enum :uint32_t {
			CodeWord_bits = Bit::BITS_PER_INT8*sizeof(CodeWordType),
			FermatOrdinal = Bit::bit_log2_floor_static<CodeWord_bits>::value,
		};
		typedef /*typename*/ CFNT<FermatOrdinal> CFNT;
		enum :uint32_t {
			N = CFNT::MaxN,
		};
	protected:
		typedef typename CFNT::CGFPrime CGFPrime;
		typedef typename CFNT::NumType	NumType;
		typedef CPoly<N - 1, CFNT>		CPoly;

		static CPoly Eval(const CPoly& A)//evaluation
		{
			CPoly B;
			static_assert(CPoly::MaxDeg + 1 == CFNT::MaxN, "poly's deg does not match ntt length!");
			B.m_uiDegree = CFNT::MaxN - 1;
			for (uint32_t i = 0; i <= A.m_uiDegree; i++) B[i] = A[i];
			for (uint32_t i = A.m_uiDegree + 1; i<CFNT::MaxN; i++) B[i] = CGFPrime::ZeroElement();
			CFNT::FNT(B.m_pCoeffs, CFNT::MaxN, true);
			return B;
		}

		static CPoly Inter(const CPoly& A)//interpolation
		{
			CPoly B;
			static_assert(CPoly::MaxDeg + 1 == CFNT::MaxN, "poly's deg does not match ntt length!");
			B.m_uiDegree = CFNT::MaxN - 1;
			for (uint32_t i = 0; i <= A.m_uiDegree; i++) B[i] = A[i];
			for (uint32_t i = A.m_uiDegree + 1; i<CFNT::MaxN; i++) B[i] = CGFPrime::ZeroElement();
			CFNT::IFNT(B.m_pCoeffs, CFNT::MaxN, true);
			B.Modify();
			return B;
		}

		static void SchonhageEuclideanProc(CMatrix<2, 1, CPoly>& R, CMatrix<2, 2, CPoly>& M, uint32_t k)
		{
			CMatrix<2, 1, CPoly> RA;
			RA[0][0] = R[0][0] >> k;
			RA[1][0] = R[1][0] >> k;
			CMatrix<2, 1, CPoly> RB;
			//RB[0][0]=CPoly::Sub(R[0][0],CPoly::Shl(RA[0][0],k));
			//RB[1][0]=CPoly::Sub(R[1][0],CPoly::Shl(RA[1][0],k));
			RB[0][0] = CPoly::ModX(R[0][0], k);
			RB[1][0] = CPoly::ModX(R[1][0], k);
			SchonhageEuclidean(RA, M);
			RB = M*RB;
			R[0][0] = (RA[0][0] << k) + RB[0][0];
			R[1][0] = (RA[1][0] << k) + RB[1][0];
		}

		static void SchonhageEuclidean(CMatrix<2, 1, CPoly>& R, CMatrix<2, 2, CPoly>& M)
		{
			uint32_t k = (R[0][0].m_uiDegree + 1) / 2;
			if (R[1][0].IsZero() || R[1][0].m_uiDegree<k) {
				//M=CMat<2,2,CPoly>::E();
				M[0][0] = CPoly::UnitElement();
				M[0][1] = CPoly::ZeroElement();
				M[1][0] = CPoly::ZeroElement();
				M[1][1] = CPoly::UnitElement();
				return;
			}

			SchonhageEuclideanProc(R, M, k);
			if (R[1][0].IsZero() || R[1][0].m_uiDegree<k) return;

			CMatrix<2, 2, CPoly> Q;
			Q[0][0] = CPoly::ZeroElement();
			Q[0][1] = CPoly::UnitElement();
			Q[1][0] = CPoly::UnitElement();
			Q[1][1] = Q[0][0] - R[0][0] / R[1][0];
			R = Q*R;
			M = Q*M;

			SchonhageEuclideanProc(R, Q, k * 2 - R[0][0].m_uiDegree);
			M = Q*M;
		}

		virtual		void EncodeT(const void* arrData, void* arrECC) {
			EncodeT2((CodeWordType*)arrData, (CodeWordType*)arrECC);
		}
		virtual uint32_t DecodeT(void* arrData, const void* arrECC) {
			return DecodeT2((CodeWordType*)arrData, (CodeWordType*)arrECC);
		}
		virtual		void EncodeF(const void* inArr, void* outArr) {
			EncodeF2((CodeWordType*)inArr, (CodeWordType*)outArr);
		}
		virtual uint32_t DecodeF(const void* inArr, void* outArr) {
			return DecodeF2((CodeWordType*)inArr, (CodeWordType*)outArr);
		}
	public:
		virtual		void EncodeT2(const CodeWordType arrData[/*N-(T*2+1)*/], CodeWordType arrECC[/*T*2+1*/]) = 0;
		virtual uint32_t DecodeT2(CodeWordType arrData[/*N-(T*2+1)*/], const CodeWordType arrECC[/*T*2+1*/]) = 0;
		virtual		void EncodeF2(const CodeWordType inArr[/*N-(T*2+1)*/], CodeWordType outArr[/*N*/]) = 0;
		virtual uint32_t DecodeF2(const CodeWordType inArr[/*N*/], CodeWordType outArr[/*N-(T*2+1)*/]) = 0;
	};

	template<typename _CodeWordType>
	class CReedSolomonCoder :public IReedSolomonCoder2<_CodeWordType>
	{
	protected:
		typedef _CodeWordType                           CodeWordType;
		typedef IReedSolomonCoder2<_CodeWordType>       IReedSolomonCoder2;
		typedef typename IReedSolomonCoder2::CPoly      CPoly;
		typedef typename IReedSolomonCoder2::CGFPrime   CGFPrime;
		typedef typename CGFPrime::NumType              NumType;

	public:
		const uint32_t T;
		const uint32_t T2;
		const uint32_t K;

		enum :uint32_t {
			N = IReedSolomonCoder2::N,
		};
	protected:
		CPoly G;
		CPoly EvalQ;
		CPoly EvalQ_;

		void Euclidean(const CPoly& S/*[T2]*/, CPoly& Lambda/*[T+1]*/, CPoly& Omega/*[T]*/)
		{
			CMatrix<2, 1, CPoly> R;
			R[0][0] = (CPoly::UnitElement() << T2);
			R[1][0] = S;
			CMatrix<2, 2, CPoly> M;
			this->SchonhageEuclidean(R, M);

			Lambda = std::move(M[1][1]);
			Omega = std::move(R[1][0]);
		}

		virtual bool Init()
		{
			if (G.m_uiDegree == 0)
			{
				//calc G=¡Ç{i=1->T2}(x-w^i)
				CPoly EvalG;
				EvalG.m_uiDegree = N - 1;
				for (uint32_t i = 1; i <= T2; i++) EvalG[i] = CGFPrime::ZeroElement();
				EvalG[0] = EvalG[T2 + 1] = CGFPrime::UnitElement();
				for (uint32_t i = 1; i <= T2; i++) {
					EvalG[0] = EvalG[0] * (CGFPrime::Exp(0) - CGFPrime::Exp(i));
					EvalG[T2 + 1] = EvalG[T2 + 1] * (CGFPrime::Exp(T2 + 1) - CGFPrime::Exp(i));
				}
				for (uint32_t i = T2 + 2; i<N; i++) {
					NumType Exp_T2 = CGFPrime::Exp(T2);
					NumType Exp_i_1 = CGFPrime::Exp(i - 1);
					EvalG[i] = EvalG[i - 1] * Exp_T2*(Exp_i_1 - CGFPrime::UnitElement()) / (Exp_i_1 - Exp_T2);
				}
				G = this->Inter(EvalG);
			}
			if (EvalQ.m_uiDegree == 0)
			{
				//calc Q=(x-1)¡Ç{i=T2+1->N-1}(x-w^i)
				EvalQ.m_uiDegree = T2;
				EvalQ[0] = CGFPrime::UnitElement();
				for (uint32_t i = T2 + 1; i<N; i++)
					EvalQ[0] = EvalQ[0] * (CGFPrime::Exp(0) - CGFPrime::Exp(i));
				for (uint32_t i = 1; i <= T2; i++) {
					NumType Exp_i_1 = CGFPrime::Exp(i - 1);
					EvalQ[i] = EvalQ[i - 1] * CGFPrime::Exp(K)*(Exp_i_1 - CGFPrime::Exp(T2)) / (Exp_i_1 - CGFPrime::Exp(N - 1));
				}
				CPoly Q = this->Inter(EvalQ);
				Q = (Q << 1) - Q;//mul x-1
				EvalQ = this->Eval(Q);
				EvalQ_ = this->Eval(CPoly::Der(Q));
			}

			return true;
		}

	public:
		CReedSolomonCoder(uint32_t _T) :T(_T), T2(T * 2), K(N - T2 - 1) {
			static_assert(N == 0x10000 || N == 0x100, "code length N does not match 8bit or 16bit!");
			assert(N>T2 + 1);
		}

		virtual void EncodeT2(const CodeWordType arrData[/*N-(T*2+1)*/], CodeWordType arrECC[/*T*2+1*/])
		{
			Init();

			CPoly M;
			M.m_uiDegree = N - 1;
			for (uint32_t i = 0; i <= T2; i++) {
				M[i] = CGFPrime::ZeroElement();
			}
			for (uint32_t i = 0; i<K; i++) {
				M[i + T2 + 1].uiValue = arrData[i];
			}

			CPoly EvalR = this->Eval(M);
			//G|(M-R),so EvalM_R:[1]->[T2] is 0,then EvalR:[1]->[T2] equal EvalM:[1]->[T2]
			//deg(R)=deg(G)-1=T2-1,so [1]->[T2] calc R is enough
			//let Q=(x-1)¡Ç{i=T2+1->N-1}(x-w^i)
			//deg(R*Q)=N-1,can calc R*Q,just set unkonw [index] as 0
			CPoly EvalQR;
			EvalQR.m_uiDegree = T2;
			EvalQR[0] = CGFPrime::ZeroElement();
			for (uint32_t i = 1; i <= T2; i++) {
				EvalQR[i] = EvalQ[i] * EvalR[i];
			}
			CPoly QR = this->Inter(EvalQR);
			//calc QR/Q£¬use deconvolution
			//when calc QR[i]/Q[i],use QR'[i]/Q'[i] if Q[i] equal 0
			//Q has none repeated factor,so Q'[i] will not equal 0
			CPoly EvalQR_ = this->Eval(CPoly::Der(QR));
			EvalR[0] = EvalQR_[0] / EvalQ_[0];
			for (uint32_t i = T2 + 1; i<N; i++) EvalR[i] = EvalQR_[i] / EvalQ_[i];
			CPoly R = this->Inter(EvalR);//R=QR/Q;
			M.m_uiDegree = T2;
			M = M - R;
			uint32_t Count[N + 1] = { 0 };
			for (uint32_t i = 0; i <= T2; i++) {
				Count[((CGFPrime::Num(N) - M[i]) / G[i]).uiValue]++;
			}
			NumType ModifyNum = CGFPrime::ZeroElement();
			for (uint32_t i = 0; i<N; i++) {
				if (Count[i] == 0) { ModifyNum = CGFPrime::Num(i); break; }
			}
			for (uint32_t i = 0; i <= T2; i++) {
				arrECC[i] = (M[i] + ModifyNum*G[i]).uiValue;
			}
		}

		virtual uint32_t DecodeT2(CodeWordType arrData[/*N-(T*2+1)*/], const CodeWordType arrECC[/*T*2+1*/])
		{
			CPoly M;
			M.m_uiDegree = N - 1;
			for (uint32_t i = 0; i <= T2; i++) {
				M[i] = CGFPrime::Num(arrECC[i]);
			}
			for (uint32_t i = 0; i<K; i++) {
				M[i + T2 + 1] = CGFPrime::Num(arrData[i]);
			}
			CPoly EvalM = this->Eval(M);
			//////////////////////////////////////////////////////////////////////////
			uint32_t uiRet = 0;
			for (uint32_t i = 1; i <= T2 && uiRet == 0; i++) {
				uiRet |= EvalM[i].uiValue;
			}
			if (uiRet == 0) {
				return IReedSolomonCoder::ECC_NOERROR;
			}
			//////////////////////////////////////////////////////////////////////////
			CPoly S = CPoly(&EvalM.m_pCoeffs[1], T2 - 1);
			CPoly Lambda;
			CPoly Omega;
			Euclidean(S, Lambda, Omega);
			CPoly Lambda_ = CPoly::Der(Lambda);
			CPoly EvalLambda = this->Eval(Lambda);
			CPoly EvalOmega = this->Eval(Omega);
			CPoly EvalLambda_ = this->Eval(Lambda_);

			for (uint32_t i = 0; i<N; i++) {
				if (EvalLambda[i] == CGFPrime::ZeroElement()) {
					uint32_t j = (N - i) % N;
					M[j] = M[j] + EvalOmega[i] / EvalLambda_[i];
					if (j>T2) {
						arrData[j - T2 - 1] = M[j].uiValue;
					}
				}
			}
			EvalM = this->Eval(M);
			//////////////////////////////////////////////////////////////////////////
			for (uint32_t i = 1; i <= T2; i++) {
				if (EvalM[i].uiValue != 0) {
					return IReedSolomonCoder::ECC_FAILED;
				}
			}
			//////////////////////////////////////////////////////////////////////////
			return IReedSolomonCoder::ECC_SUCCESS;
		}

		virtual void EncodeF2(const CodeWordType inArr[/*N-(T*2+1)*/], CodeWordType outArr[/*N*/])
		{
			CPoly EvalM;
			EvalM.m_uiDegree = N - 1;
			for (uint32_t i = 0; i <= T2; i++) {
				EvalM[i] = CGFPrime::ZeroElement();
			}
			for (uint32_t i = 0; i<K; i++) {
				EvalM[i + T2 + 1].uiValue = inArr[i];
			}
			CPoly M = this->Inter(EvalM);

			uint32_t Count[N + 1] = { 0 };
			for (uint32_t i = 0; i<N; i++) {
				Count[M[i].uiValue]++;
			}
			NumType ModifyNum = CGFPrime::ZeroElement();
			for (uint32_t i = N; i != (uint32_t)-1; i--) {
				if (Count[i] == 0) { ModifyNum = CGFPrime::Num(N - i); break; }
			}
			for (int i = 0; i<N; i++) {
				outArr[i] = (M[i] + ModifyNum).uiValue;
			}
		}

		virtual uint32_t DecodeF2(const CodeWordType inArr[/*N*/], CodeWordType outArr[/*N-(T*2+1)*/])
		{
			CPoly M;
			M.m_uiDegree = N - 1;
			for (uint32_t i = 0; i<N; i++) {
				M[i] = CGFPrime::Num(inArr[i]);
			}
			CPoly EvalM = this->Eval(M);
			//////////////////////////////////////////////////////////////////////////
			uint32_t uiRet = 0;
			for (uint32_t i = 1; i <= T2 && uiRet == 0; i++) {
				uiRet |= EvalM[i].uiValue;
			}
			if (uiRet == 0) {
				return IReedSolomonCoder::ECC_NOERROR;
			}
			//////////////////////////////////////////////////////////////////////////
			CPoly S = CPoly(&EvalM.m_pCoeffs[1], T2 - 1);
			CPoly Lambda;
			CPoly Omega;
			Euclidean(S, Lambda, Omega);
			CPoly Lambda_ = CPoly::Der(Lambda);
			CPoly EvalLambda = this->Eval(Lambda);
			CPoly EvalOmega = this->Eval(Omega);
			CPoly EvalLambda_ = this->Eval(Lambda_);

			for (uint32_t i = 0; i<N; i++) {
				if (EvalLambda[i] == CGFPrime::ZeroElement()) {
					uint32_t j = (N - i) % N;
					M[j] = M[j] + EvalOmega[i] / EvalLambda_[i];
				}
			}
			EvalM = this->Eval(M);
			//////////////////////////////////////////////////////////////////////////
			for (uint32_t i = 1; i <= T2; i++) {
				if (EvalM[i].uiValue != 0) {
					return IReedSolomonCoder::ECC_FAILED;
				}
			}
			//////////////////////////////////////////////////////////////////////////
			for (uint32_t i = 0; i<K; i++) {
				outArr[i] = EvalM[i + T2 + 1].uiValue;
			}
			return IReedSolomonCoder::ECC_SUCCESS;
		}
	};

};


#endif
