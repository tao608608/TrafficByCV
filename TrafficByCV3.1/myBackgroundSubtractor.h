#ifndef MYBACKGOUNDSUBTRACTOR_H
#define MYBACKGOUNDSUBTRACTOR_H

#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"


static const int defaultHistory2 = 900;//ѧϰ���ʣ�alpha=1/history
static const float defaultVarThreshold2 = 4.0f*4.0f;//���Ͼ�������ֵ
static const int defaultNMixtures2 = 5; //��ʼ����˹ģ�͸���
static const float defaultBackgroundRatio2 = 0.9f; //Ȩֵ�͵�����ֵ
static const float defaultVarThresholdGen2 = 3.0f*3.0f;
static const float defaultVarInit2 = 15.0f; //����ĳ�ʼ��ֵ
static const float defaultVarMax2 = 5 * defaultVarInit2;
static const float defaultVarMin2 = 4.0f;
static const float defaultfCT2 = 0.05f; //���͸��Ӷ����鳣�� ��0�������٣�
static const unsigned char defaultnShadowDetection2 = (unsigned char)0; //��Ӱ��⣬����ֵ
static const float defaultfTau = 0.5f; //��Ӱ����ֵ

struct GMM
{
	float weight;
	float variance;
};
CV_INLINE bool mydetectShadowGMM(const float* data, int nchannels, int nmodes,
	const GMM* gmm, const float* mean, float Tb, float TB, float tau)
{
	float tWeight = 0;

	// ������б����Ϊ������ģ��
	for (int mode = 0; mode < nmodes; mode++, mean += nchannels)
	{
		GMM g = gmm[mode];

		float numerator = 0.0f;
		float denominator = 0.0f;
		for (int c = 0; c < nchannels; c++)
		{
			numerator += data[c] * mean[c];
			denominator += mean[c] * mean[c];
		}

		// ������Ϊ��
		if (denominator == 0)
			return false;

		// if tau < a < 1 ,�����ɫʧ��
		if (numerator <= denominator && numerator >= tau*denominator)
		{
			float a = numerator / denominator;
			float dist2a = 0.0f;

			for (int c = 0; c < nchannels; c++)
			{
				float dD = a*mean[c] - data[c];
				dist2a += dD*dD;
			}

			if (dist2a < Tb*g.variance*a*a)
				return true;
		};

		tWeight += g.weight;
		if (tWeight > TB)
			return false;
	};
	return false;
}
class myMOG2Invoker : public cv::ParallelLoopBody
{
public:
	myMOG2Invoker(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat& _imgmask,
		GMM* _gmm, float* _mean,
		uchar* _modesUsed,
		int _nmixtures, float _alphaT,
		float _Tb, float _TB, float _Tg,
		float _varInit, float _varMin, float _varMax,
		float _prune, float _tau, bool _detectShadows,
		uchar _shadowVal)
	{
		src = &_src;
		dst = &_dst;
		imgmask = &_imgmask;
		gmm0 = _gmm;
		mean0 = _mean;
		modesUsed0 = _modesUsed;
		nmixtures = _nmixtures;
		alphaT = _alphaT;
		Tb = _Tb;
		TB = _TB;
		Tg = _Tg;
		varInit = _varInit;
		varMin = MIN(_varMin, _varMax);
		varMax = MAX(_varMin, _varMax);
		prune = _prune;
		tau = _tau;
		detectShadows = _detectShadows;
		shadowVal = _shadowVal;
	}

	void operator()(const cv::Range& range) const
	{
		int y0 = range.start, y1 = range.end;
		int ncols = src->cols, nchannels = src->channels();
		cv::AutoBuffer<float> buf(src->cols*nchannels);
		float alpha1 = 1.f - alphaT;
		float dData[CV_CN_MAX];

		for (int y = y0; y < y1; y++)
		{
			const float* data = buf;
			if (src->depth() != CV_32F)
				src->row(y).convertTo(cv::Mat(1, ncols, CV_32FC(nchannels), (void*)data), CV_32F);
			else
				data = src->ptr<float>(y);

			float* mean = mean0 + ncols*nmixtures*nchannels*y;//ÿ�����ص�ÿ��ͨ����n����ֵ
			GMM* gmm = gmm0 + ncols*nmixtures*y;//ÿ��������n��ģ��
			uchar* modesUsed = modesUsed0 + ncols*y;//ģʽ����ʹ����� ����
			uchar* mask = dst->ptr(y);
			const uchar* maskptr = imgmask->ptr<uchar>(y);

			for (int x = 0; x < ncols; x++, data += nchannels, gmm += nmixtures, mean += nmixtures*nchannels)
			{
				if (maskptr[x] == 0)
					continue;
				//������벢������Ҫ���ף�
				bool background = false;

				//internal:
				bool fitsPDF = false;
				int nmodes = modesUsed[x], nNewModes = nmodes;//��ǰģ�͸���
				float totalWeight = 0.f;

				float* mean_m = mean;//��ֵ


				for (int mode = 0; mode < nmodes; mode++, mean_m += nchannels)
				{
					float weight = alpha1*gmm[mode].weight + prune;//ƥ�䣬prune=-learningRate*fCT  w=(1-alpha)*w
					int swap_count = 0;

					//δƥ��
					if (!fitsPDF)
					{
						//������Ƿ�����ʣ���ģʽ
						float var = gmm[mode].variance;

						//�������
						float dist2;

						if (nchannels == 3)
						{
							dData[0] = mean_m[0] - data[0];
							dData[1] = mean_m[1] - data[1];
							dData[2] = mean_m[2] - data[2];
							dist2 = dData[0] * dData[0] + dData[1] * dData[1] + dData[2] * dData[2];
						}
						else
						{
							dist2 = 0.f;
							for (int c = 0; c < nchannels; c++)
							{
								dData[c] = mean_m[c] - data[c];
								dist2 += dData[c] * dData[c];
							}
						}

						//background�� Tb>Tg
						if (totalWeight < TB && dist2 < Tb*var)
							background = true;

						if (dist2 < Tg*var)
						{
							//���ڸ�ģ��
							fitsPDF = true;

							// ���·ֲ�

							//����Ȩֵ
							weight += alphaT;//�ӵ�β�͵�alpha w=(1-alpha)*w+alpha*o
							float k = alphaT / weight;//������ѧϰ����  

							//���¾�ֵ
							for (int c = 0; c < nchannels; c++)
								mean_m[c] -= k*dData[c];//u=u+(data-u)*k*o

							//���·���
							float varnew = var + k*(dist2 - var);
							//��������
							varnew = MAX(varnew, varMin);
							varnew = MIN(varnew, varMax);
							gmm[mode].variance = varnew;

							//��Ȩֵ����
							for (int i = mode; i > 0; i--)
							{
								if (weight < gmm[i - 1].weight)
									break;

								swap_count++;
								std::swap(gmm[i], gmm[i - 1]);
								for (int c = 0; c < nchannels; c++)
									std::swap(mean[i*nchannels + c], mean[(i - 1)*nchannels + c]);
							}
						}
					}

					//���prune
					if (weight < -prune)
					{
						weight = 0.0;
						nmodes--;
					}

					gmm[mode - swap_count].weight = weight;//����Ȩֵ
					totalWeight += weight;
				}

				//��������ģ�ͣ���һ��Ȩֵ
				totalWeight = 1.f / totalWeight;
				for (int mode = 0; mode < nmodes; mode++)
				{
					gmm[mode].weight *= totalWeight;
				}

				nmodes = nNewModes;

				//�����µ�ģ�ͣ������Ҫ�����˳�
				if (!fitsPDF && alphaT > 0.f)
				{
					//�滻Ȩֵ��С�Ļ����¼�һ��
					int mode = nmodes == nmixtures ? nmixtures - 1 : nmodes++;

					if (nmodes == 1)
						gmm[mode].weight = 1.f;
					else
					{
						gmm[mode].weight = alphaT;

						//��һ������Ȩֵ
						for (int i = 0; i < nmodes - 1; i++)
							gmm[i].weight *= alpha1;
					}

					//��ʼ��
					for (int c = 0; c < nchannels; c++)
						mean[mode*nchannels + c] = data[c];

					gmm[mode].variance = varInit;

					//Ȩֵ����
					for (int i = nmodes - 1; i > 0; i--)
					{
						if (alphaT < gmm[i - 1].weight)
							break;

						std::swap(gmm[i], gmm[i - 1]);
						for (int c = 0; c < nchannels; c++)
							std::swap(mean[i*nchannels + c], mean[(i - 1)*nchannels + c]);
					}
				}

				//����ģʽ������
				modesUsed[x] = uchar(nmodes);
				mask[x] = background ? 0 :
					detectShadows && mydetectShadowGMM(data, nchannels, nmodes, gmm, mean, Tb, TB, tau) ?
				shadowVal : 255;
			}
		}
	}

	const cv::Mat* src;
	cv::Mat* dst;
	const cv::Mat* imgmask;
	GMM* gmm0;
	float* mean0;
	uchar* modesUsed0;

	int nmixtures;
	float alphaT, Tb, TB, Tg;
	float varInit, varMin, varMax, prune, tau;

	bool detectShadows;
	uchar shadowVal;
};
class myBackgroundSubtractorMOG2
{
public:
	myBackgroundSubtractorMOG2()
	{
		frameSize = cv::Size(0, 0);
		frameType = 0;

		nframes = 0;
		history = defaultHistory2;
		varThreshold = defaultVarThreshold2;
		bShadowDetection = 1;

		nmixtures = defaultNMixtures2;
		backgroundRatio = defaultBackgroundRatio2;
		fVarInit = defaultVarInit2;
		fVarMax = defaultVarMax2;
		fVarMin = defaultVarMin2;

		varThresholdGen = defaultVarThresholdGen2;
		fCT = defaultfCT2;
		nShadowDetection = defaultnShadowDetection2;
		fTau = defaultfTau;
	}
	void createmyBackgroundSubtractorMOG2(int _history, float _varThreshold, bool _bShadowDetection = true)
	{
		frameSize = cv::Size(0, 0);
		frameType = 0;

		nframes = 0;
		history = _history > 0 ? _history : defaultHistory2;
		varThreshold = (_varThreshold > 0) ? _varThreshold : defaultVarThreshold2;
		bShadowDetection = _bShadowDetection;

		nmixtures = defaultNMixtures2;
		backgroundRatio = defaultBackgroundRatio2;
		fVarInit = defaultVarInit2;
		fVarMax = defaultVarMax2;
		fVarMin = defaultVarMin2;

		varThresholdGen = defaultVarThresholdGen2;
		fCT = defaultfCT2;
		nShadowDetection = defaultnShadowDetection2;
		fTau = defaultfTau;
	}
	~myBackgroundSubtractorMOG2() {}
	void apply(cv::Mat& image, cv::Mat& fgmask, cv::Mat& imgmask, double learningRate = -1)
	{
		bool needToInitialize = nframes == 0 || learningRate >= 1 || image.size() != frameSize || image.type() != frameType;

		if (needToInitialize)
			initialize(image.size(), image.type());

		fgmask.create(image.size(), CV_8U);

		++nframes;
		learningRate = learningRate >= 0 && nframes > 1 ? learningRate : 1. / std::min(2 * nframes, history);
		CV_Assert(learningRate >= 0);
		parallel_for_(cv::Range(0, image.rows),
			myMOG2Invoker(image, fgmask, imgmask,
			bgmodel.ptr<GMM>(),
			(float*)(bgmodel.ptr() + sizeof(GMM)*nmixtures*image.rows*image.cols),
			bgmodelUsedModes.ptr(), nmixtures, (float)learningRate,
			(float)varThreshold,
			backgroundRatio, varThresholdGen,
			fVarInit, fVarMin, fVarMax, float(-learningRate*fCT), fTau,
			bShadowDetection, nShadowDetection),
			image.total() / (double)(1 << 16));
	}
	void initialize(cv::Size _frameSize, int _frameType)
	{
		frameSize = _frameSize;
		frameType = _frameType;
		nframes = 0;

		int nchannels = CV_MAT_CN(frameType);
		CV_Assert(nchannels <= CV_CN_MAX);
		CV_Assert(nmixtures <= 255);
		{
			bgmodel.create(1, frameSize.height*frameSize.width*nmixtures*(2 + nchannels), CV_32F);
			//��ʼ������ģ��Ϊ��
			bgmodelUsedModes.create(frameSize, CV_8U);
			bgmodelUsedModes = cv::Scalar::all(0);
		}
	}
private:
	cv::Size frameSize;
	int frameType;
	cv::Mat bgmodel;
	cv::Mat bgmodelUsedModes;//ÿ���������õ�ģ����Ŀ

	int nframes;
	int history;
	int nmixtures;//���ģ����
	double varThreshold;//���Ͼ�������ֵ


	float backgroundRatio; //Ȩֵ�͵�����ֵ
	float varThresholdGen;

	float fVarInit;
	float fVarMin;
	float fVarMax;

	float fCT;//����ģ����Ŀ�Ĳ���

	//��Ӱ������
	bool bShadowDetection;
	unsigned char nShadowDetection;
	float fTau;
};
#endif
