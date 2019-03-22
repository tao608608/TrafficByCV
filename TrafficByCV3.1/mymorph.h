#ifndef MYMORPH_H
#define MYMORPH_H

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "iostream"

#define __mySSE2__
//#define __myARM_NEON__

#if defined __mySSE2__
#include <emmintrin.h>
#define  mySSE2 1
#endif

#if defined __myARM_NEON__ 
# include "arm_neon.h"
# define myNEON 1
#endif

void myRowErode_8u(const cv::Mat& _src, cv::Mat _dst, int ksize)
{
	if (_src.type() == CV_8UC1)
	{
		int i, j, k, width = _src.cols;
		const uchar* sptr = _src.ptr();
		uchar* dptr = _dst.ptr();


#if mySSE2
		for (i = 0; i <= width - 16; i += 16)
		{
			__m128i s = _mm_loadu_si128((const __m128i*)(_src.ptr() + i));
			for (k = 1; k < ksize; ++k)
			{
				__m128i x = _mm_loadu_si128((const __m128i*)(_src.ptr() + i + k));
				s = _mm_and_si128(s, x);
			}
			_mm_storeu_si128((__m128i*)(_dst.ptr() + i), s);
		}

		for (; i < width; i += 4)
		{
			__m128i s = _mm_cvtsi32_si128(*(const int*)(_src.ptr() + i));
			for (k = 1; k < ksize - 1; ++k)
			{
				__m128i x = _mm_cvtsi32_si128(*(const int*)(_src.ptr() + i + k));
				s = _mm_and_si128(s, x);
			}
			*(int*)(_dst.ptr() + i) = _mm_cvtsi128_si32(s);
		}
#endif

#if myNeon
		uint8x16_t tmp;
		for (i = 0; i <= width - 16; i += 16)
		{
			const uchar* s=sptr + i;
			uint8x16_t stmp=vld1q_u8(s);
			for (k = 1; k < ksize-1; ++k)
			{
				const uchar*x=sptr + i + k;
				tmp=vandq_u8(stmp, vld1q_u8(x));
			}
			vst1q_u8(dptr + i, tmp);
		}
#endif
		for (; i < width; ++i)
		{
			const uchar* s = sptr + i;
			uchar m = s[0];
			for (j = 1; j < ksize - 1; ++j)
				m = (m&s[j]);
			dptr[j] = m;
		}
	}
	else
	{
		std::cout << "the type of the picture must be CV_8UC1!" << std::endl;
	}
}
void myColumnErode_8u(const cv::Mat&_src, cv::Mat _dst, int ksize)
{
	if (_src.type() == CV_8UC1)
	{
		int i, k, count = _src.rows, width = _src.cols, srcstep = _src.step, dststep = _dst.step;
		const uchar* src = _src.ptr();
		uchar* D = _dst.ptr();
		int rows = 0;

#if mySSE2

		for (; count > 0; --count, D += dststep, src += srcstep,rows++)
		{
			for (i = 0; i <= width - 32; i += 32)
			{
				const uchar* sptr = src + i;
				__m128i s0 = _mm_loadu_si128((const __m128i*)sptr);
				__m128i s1 = _mm_loadu_si128((const __m128i*)(sptr + 16));
				__m128i x0, x1;

				for (k = 1; (k < ksize - 1)&&(rows+k<_src.rows); ++k)
				{
					sptr = src + k*srcstep + i;
					x0 = _mm_loadu_si128((const __m128i*)sptr); 
					x1 = _mm_loadu_si128((const __m128i*)(sptr + 16));
					s0 = _mm_and_si128(s0, x0);
					s1 = _mm_and_si128(s1, x1);
				}
				_mm_storeu_si128((__m128i*)(D + i), s0);
				_mm_storeu_si128((__m128i*)(D + i + 16), s1);
			}

			for (; i < width - 8; i += 8)
			{
				const uchar* sptr = src + i;
				__m128i s0 = _mm_loadl_epi64((const __m128i*)sptr), x0;

				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + k*srcstep + i;
					x0 = _mm_loadl_epi64((const __m128i*)sptr);
					s0 = _mm_and_si128(s0, x0);
				}
				_mm_storel_epi64((__m128i*)(D + i), s0);
			}
		}
#endif

#if myNeon
		for (; count > 0;--count,D+=dststep,src+=srcstep,rows++)
		{
			for (i = 0; i <= width - 32;i+=32)
			{
				const uchar* sptr = src+i;
				uint8x16_t s0 = vld1q_u8(sptr);
				uint8x16_t s1 = vld1q_u8(sptr+ 16);
				uint8x16_t x0, x1;

				for (k = 1; (k < ksize - 1) && (rows + k< _src.rows);++k)
				{
					sptr = src+k*srcstep + i;
					x0 = vld1q_u8(sptr);
					x1 = vld1q_u8(sptr + 16);
					s0 = vandq_u8(s0,x0);
					s1 = vandq_u8(s1,x1);
				}
				vst1q_u8((D + i), s0);
				vst1q_u8((D + i + 16), s1);
			}

		}
#endif

		count = _src.rows; rows = 0;
		src = _src.ptr();D = _dst.ptr();
		for (; count > 1; count -= 2, D += dststep * 2, src += 2 * srcstep,rows=+2)
		{
			for (; i < width - 4; i += 4)
			{
				const uchar* sptr = src + srcstep + i;
				uchar s0 = sptr[0], s1 = sptr[1], s2 = sptr[2], s3 = sptr[3];

				for (k = 2; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + srcstep*k + i;
					s0 = (s0&sptr[0]); s1 = (s1&sptr[1]);
					s2 = (s2&sptr[2]); s3 = (s3&sptr[3]);
				}

				sptr = _src.ptr(0) + i;
				D[i] = (s0&sptr[0]);
				D[i + 1] = (s1&sptr[1]);
				D[i + 2] = (s2&sptr[2]);
				D[i + 3] = (s3&sptr[3]);

				sptr = src + srcstep*k + i;
				D[i + dststep] = (s0&sptr[0]);
				D[i + dststep + 1] = (s1&sptr[1]);
				D[i + dststep + 2] = (s2&sptr[2]);
				D[i + dststep + 3] = (s3&sptr[3]);
			}
			for (; i < width; ++i)
			{
				uchar s0 = *(src + srcstep + i);

				for (k = 2; (k < ksize - 1) && (rows + k<_src.rows); ++k)
					s0 = (s0&*(src + srcstep*k + i));

				D[i] = (s0&*(src + i));
				D[i + dststep] = (s0 & *(src + srcstep*k + i));
			}
		}

		for (; count>0; --count, D += dststep, src += srcstep,rows++)
		{
			for (; i <= width - 4; i += 4)
			{
				const uchar* sptr = src + i;
				uchar s0 = sptr[0], s1 = sptr[1], s2 = sptr[2], s3 = sptr[3];

				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + srcstep*k + i;
					s0 = (s0&sptr[0]); s1 = (s1&sptr[1]);
					s2 = (s2&sptr[2]); s3 = (s3&sptr[3]);
				}

				D[i] = s0; D[i + 1] = s1;
				D[i + 2] = s2; D[i + 3] = s3;
			}
			for (; i < width; ++i)
			{
				uchar s0 = *(src + i);
				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
					s0 = (s0 & *(src + srcstep*k + i));
				D[i] = s0;
			}
		}
	}
	else
	{
		std::cout << "the type of the picture must be CV_8UC1!" << std::endl;
	}
}
class ErodeRowRunner :public cv::ParallelLoopBody
{
public:
	ErodeRowRunner(cv::Mat _src, cv::Mat _dst, cv::Size _ksize)
	{
		src = _src;
		dst = _dst;

		ksize = _ksize;
	}

	void operator()(const cv::Range& range)const
	{
		int row0 = range.start;
		int row1 = range.end;

		cv::Mat srcStripe = src.rowRange(row0, row1);
		cv::Mat dstStripe = dst.rowRange(row0, row1);

		myRowErode_8u(srcStripe, dstStripe, ksize.width);

	}
private:
	cv::Mat src;
	cv::Mat dst;

	cv::Size ksize;
};
class ErodeColumnRunner :public cv::ParallelLoopBody
{
public:
	ErodeColumnRunner(cv::Mat _src, cv::Mat _dst, cv::Size _ksize)
	{
		src = _src;
		dst = _dst;

		ksize = _ksize;
	}

	void operator()(const cv::Range& range)const
	{
		int row0 = range.start;
		int row1 = range.end;

		cv::Mat srcStripe = src.rowRange(row0, row1);
		cv::Mat dstStripe = dst.rowRange(row0, row1);

		myColumnErode_8u(srcStripe, dstStripe, ksize.height);

	}
private:
	cv::Mat src;
	cv::Mat dst;

	cv::Size ksize;
};
void myOpErode(cv::Mat& src, cv::Mat& dst, cv::Size ksize)
{

	//dst.create(src.size(), src.type());
	cv::Mat tmpdst(src.size(), src.type());

	cv::parallel_for_(cv::Range(0, src.rows), ErodeRowRunner(src, tmpdst, ksize));
	cv::parallel_for_(cv::Range(0, src.rows), ErodeColumnRunner(tmpdst, dst, ksize));
}

void myRowDilate_8u(const cv::Mat& _src, cv::Mat _dst, int ksize)
{
	if (_src.type() == CV_8UC1)
	{
		int i, j, k, width = _src.cols;
		const uchar* sptr = _src.ptr();
		uchar* dptr = _dst.ptr();


#if mySSE2
		for (i = 0; i <= width - 16; i += 16)
		{
			__m128i s = _mm_loadu_si128((const __m128i*)(_src.ptr() + i));
			for (k = 1; k < ksize - 1; ++k)
			{
				__m128i x = _mm_loadu_si128((const __m128i*)(_src.ptr() + i + k));
				s = _mm_or_si128(s, x);
			}
			_mm_storeu_si128((__m128i*)(_dst.ptr() + i), s);
		}

		for (; i < width; i += 4)
		{
			__m128i s = _mm_cvtsi32_si128(*(const int*)(_src.ptr() + i));
			for (k = 1; k < ksize - 1; ++k)
			{
				__m128i x = _mm_cvtsi32_si128(*(const int*)(_src.ptr() + i + k));
				s = _mm_or_si128(s, x);
			}
			*(int*)(_dst.ptr() + i) = _mm_cvtsi128_si32(s);
		}
#endif

#if myNeon
		uint8x16_t tmp;
		for (i = 0; i <= width - 16; i += 16)
		{
			const uchar* s = _src.ptr() + i;
			uint8x16_t stmp=vld1q_u8(s);
			for (k = 1; k < ksize-1; ++k)
			{
				const uchar*x = _src.ptr() + i + k;
				tmp=vorrq_u8(stmp, vld1q_u8(x));
			}
			vst1q_u8(_dst.ptr()+i,tmp);
		}
#endif

		for (; i < width; ++i)
		{
			const uchar* s = sptr + i;
			uchar m = s[0];
			for (j = 1; j < ksize - 1; ++j)
				m = (m | s[j]);
			dptr[j] = m;
		}
	}
	else
	{
		std::cout << "the type of the picture must be CV_8UC1!" << std::endl;
	}
}
void myColumnDilate_8u(const cv::Mat&_src, cv::Mat _dst, int ksize)
{
	if (_src.type() == CV_8UC1)
	{
		int i, k, count = _src.rows, width = _src.cols, srcstep = _src.step, dststep = _dst.step;
		const uchar* src = _src.ptr();
		uchar* D = _dst.ptr();
		int rows = 0;

#if mySSE2
		for (; count > 0; --count, D += dststep, src += srcstep,rows++)
		{
			for (i = 0; i <= width - 32; i += 32)
			{
				const uchar* sptr = src + i;
				__m128i s0 = _mm_loadu_si128((const __m128i*)sptr);
				__m128i s1 = _mm_loadu_si128((const __m128i*)(sptr + 16));
				__m128i x0, x1;

				for (k = 1; (k < ksize - 1) && (rows + k< _src.rows); ++k)
				{
					sptr = src + k*srcstep + i;
					x0 = _mm_loadu_si128((const __m128i*)sptr);
					x1 = _mm_loadu_si128((const __m128i*)(sptr + 16));
					s0 = _mm_or_si128(s0, x0);
					s1 = _mm_or_si128(s1, x1);
				}
				_mm_storeu_si128((__m128i*)(D + i), s0);
				_mm_storeu_si128((__m128i*)(D + i + 16), s1);
			}

			for (; i < width - 8; i += 8)
			{
				const uchar* sptr = src + i;
				__m128i s0 = _mm_loadl_epi64((const __m128i*)sptr), x0;

				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + k*srcstep + i;
					x0 = _mm_loadl_epi64((const __m128i*)sptr);
					s0 = _mm_or_si128(s0, x0);
				}
				_mm_storel_epi64((__m128i*)(D + i), s0);
			}
		}
#endif

#if myNeon

		for (; count > 0; --count, D += dststep, ++src,rows++)
		{
			for (i = 0; i <= width - 32; i += 32)
			{
				const uchar* sptr =src + i;
				uint8x16_t s0 = vld1q_u8(sptr);
				uint8x16_t s1 = vld1q_u8(sptr + 16);
				uint8x16_t x0, x1;

				for (k = 1; (k < ksize - 1) && (rows + k< _src.rows); ++k)
				{
					sptr = src+srcstep*k + i;
					x0 = vld1q_u8(sptr);
					x1 = vld1q_u8(sptr + 16);
					s0 = vorrq_u8(s0, x0);
					s1 = vorrq_u8(s1, x1);
				}
				vst1q_u8((D + i), s0);
				vst1q_u8((D + i + 16), s1);
			}

		}
#endif

		count = _src.rows; rows = 0;
		src = _src.ptr(); D = _dst.ptr();
		for (; count > 1; count -= 2, D += dststep * 2, src += 2 * srcstep,rows+=2)
		{
			for (; i < width - 4; i += 4)
			{
				const uchar* sptr = src + srcstep + i;
				uchar s0 = sptr[0], s1 = sptr[1], s2 = sptr[2], s3 = sptr[3];

				for (k = 2; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + srcstep*k + i;
					s0 = (s0 | sptr[0]); s1 = (s1 | sptr[1]);
					s2 = (s2 | sptr[2]); s3 = (s3 | sptr[3]);
				}

				sptr = src + i;
				D[i] = (s0 | sptr[0]);
				D[i + 1] = (s1 | sptr[1]);
				D[i + 2] = (s2 | sptr[2]);
				D[i + 3] = (s3 | sptr[3]);

				sptr = src + srcstep*k + i;
				D[i + dststep] = (s0 | sptr[0]);
				D[i + dststep + 1] = (s1 | sptr[1]);
				D[i + dststep + 2] = (s2 | sptr[2]);
				D[i + dststep + 3] = (s3 | sptr[3]);
			}
			for (; i < width; ++i)
			{
				uchar s0 = *(src + srcstep + i);

				for (k = 2; (k < ksize - 1) && (rows + k<_src.rows); ++k)
					s0 = (s0 | *(src + srcstep*k + i));

				D[i] = (s0 | *(src + i));
				D[i + dststep] = (s0 | *(src + srcstep*k + i));
			}
		}

		for (; count>0; --count, D += dststep, src += srcstep,rows++)
		{
			for (; i <= width - 4; i += 4)
			{
				const uchar* sptr = src + i;
				uchar s0 = sptr[0], s1 = sptr[1], s2 = sptr[2], s3 = sptr[3];

				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
				{
					sptr = src + srcstep*k + i;
					s0 = (s0 | sptr[0]); s1 = (s1 | sptr[1]);
					s2 = (s1 | sptr[2]); s3 = (s3 | sptr[3]);
				}

				D[i] = s0; D[i + 1] = s1;
				D[i + 2] = s2; D[i + 3] = s3;
			}
			for (; i < width; ++i)
			{
				uchar s0 = *(src + i);
				for (k = 1; (k < ksize - 1) && (rows + k<_src.rows); ++k)
					s0 = (s0 | *(src + srcstep*k + i));
				D[i] = s0;
			}
		}
	}
	else
	{
		std::cout << "the type of the picture must be CV_8UC1!" << std::endl;
	}
}
class DilateRowRunner :public cv::ParallelLoopBody
{
public:
	DilateRowRunner(cv::Mat _src, cv::Mat _dst, cv::Size _ksize)
	{
		src = _src;
		dst = _dst;

		ksize = _ksize;
	}

	void operator()(const cv::Range& range)const
	{
		int row0 = range.start;
		int row1 = range.end;

		cv::Mat srcStripe = src.rowRange(row0, row1);
		cv::Mat dstStripe = dst.rowRange(row0, row1);


		myRowDilate_8u(srcStripe, dstStripe, ksize.width);

	}
private:
	cv::Mat src;
	cv::Mat dst;

	cv::Size ksize;
};
class DilateColumnRunner :public cv::ParallelLoopBody
{
public:
	DilateColumnRunner(cv::Mat _src, cv::Mat _dst, cv::Size _ksize)
	{
		src = _src;
		dst = _dst;

		ksize = _ksize;
	}

	void operator()(const cv::Range& range)const
	{
		int row0 = range.start;
		int row1 = range.end;

		cv::Mat srcStripe = src.rowRange(row0, row1);
		cv::Mat dstStripe = dst.rowRange(row0, row1);

		myColumnDilate_8u(srcStripe, dstStripe, ksize.height);

	}
private:
	cv::Mat src;
	cv::Mat dst;

	cv::Size ksize;
};
void myOpDilate(cv::Mat& src, cv::Mat& dst, cv::Size ksize)
{
	//dst.create(src.size(), src.type());
	cv::Mat tmpdst(src.size(), src.type());

	cv::parallel_for_(cv::Range(0, src.rows), DilateRowRunner(src, tmpdst, ksize));
	cv::parallel_for_(cv::Range(0, src.rows), DilateColumnRunner(tmpdst, dst, ksize));
}
#endif