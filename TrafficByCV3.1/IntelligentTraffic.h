#ifndef INTELLGENTTRAFFIC_H
#define  INTELLGENTTRAFFIC_H

#pragma once
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/ml/ml.hpp"
#include "set"
#include "myBackgroundSubtractor.h"
#include "mytime.h"


using namespace cv;
using namespace std;


struct area_direction
{
	bool use;
	vector<cv::Point>  area;
	vector<cv::Point>direction;
};

class IntelligentTraffic
{
public:
	IntelligentTraffic(void);
	~IntelligentTraffic(void);
private:
	struct TrackRect
	{
		cv::Mat imgRect;//Ŀ��ͼ��
		cv::MatND NDHist;//ֱ��ͼ
		int id;
		int areaid;//Ŀ����������id
		cv::Point center;
		cv::Rect lastTrackWindow;//��һ֡Ŀ����ͼ���ϵ�λ��
		bool isUpdate;
		int leaveCount;
		cv::KalmanFilter KF;
		cv::Point predictPt;//��һ֡Ԥ�⵱ǰ֡λ��
		int predictNum;//Ԥ�����
		cv::Mat hHist;//ֱ��ͼh������
		cv::RotatedRect camshiftPos;//��һ֡���ٵ�ǰ֡λ��
		bool isTrack;
		bool isPre;
		int matchNum;//ƥ�����ͣ�1���ɹ�ƥ�䣻2����Ŀ�ꣻ3��
		int carSize;


	};
public:
	struct tarTrigger
	{
		int id;
		cv::Point pos;
	};
	struct targetMessage
	{
		vector<cv::Point> trackRoad;//��ʷ�켣
		cv::Rect rect;//��Ӿ���
		int  classNum;//����
		int carsize;//��С
		int MatchType = 0;//0��Ŀ��(��ɫ)��1���ٳɹ�(��ɫ)��2Ԥ��ɹ�(��ɫ)��3���ɹ�(��ɫ),4������(���)
		
		
		//�޸�by ����� 2016.5.11
		bool isStop;
		bool isReversedrive;
		bool isPedestrianEntry;
		bool isDriveOutOfBorder;
		bool isLoss;
		bool updata;
		targetMessage()
		{
			isStop = false;
			isReversedrive = false;
			isPedestrianEntry = false;
			isDriveOutOfBorder = false;
			isLoss = false;
			updata = false;
		}

	};
	struct EventsResults
	{
		int m_id;
		int m_events[5];
		cv::Point m_pos[5];
		string day[5], second[5];
		cv::Mat img_result;
		bool isPicSaved[5];
		EventsResults()
		{
			img_result = cv::Mat(720, 720, CV_8UC3);
			for (int i = 0; i < 5; ++i)
			{
				m_events[i] = 0;
				isPicSaved[i] = false;
			}
		}
	};
	struct SituatResults
	{
		bool jam;
		int carNum;
		cv::Mat img_result;
		SituatResults()
		{
			img_result = cv::Mat(720, 720, CV_8UC3);
			jam = false;
			carNum = 0;
		}
	};
	struct Direction
	{
		cv::Point ptFirst;
		cv::Point ptEnd;
	};
private:
	void matchTemp(cv::Mat img, cv::Mat templ, cv::Rect &ret);
	int CarCascade(cv::Mat img);
public:
	void fGdetectInit(int method = 2);
	void fGDetect(cv::Mat &inputImg, cv::Mat&outputfGImg,cv::Mat& maskImg,int learn);
	int tarDetect(cv::Mat inputMask, vector<cv::Rect>&tarRects, int niters = 3);
	void tarTrack(cv::Mat &inputImg, vector<cv::Rect>&tarRects, map<int, targetMessage> &trackRoad, vector<area_direction> &areas);
	void tarclassInit();
	void tarClass(cv::Mat &inputImg, map<int, targetMessage> &trackRoad);

	int stopIncidentDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyStop, string time[2], cv::Mat &img_Pic);
	int reverseDriveIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, Direction &direct, vector<tarTrigger>&outMessagebyReverse, string time[2], cv::Mat &img_Pic);
	int driveOutOfBorderIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyDriveOut, string time[2], cv::Mat &img_Pic);
	int pedestrianEntryIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyPedestrian, string time[2], cv::Mat &img_Pic);
	int lossIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyLoss, string time[2], cv::Mat &img_Pic);
	bool jamIncidentDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, int &numTh, int &speed, string time[2], cv::Mat &img_Pic);

	void carNumDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, set<int> &preID, int &carNum, string time[2]);
	void CarSizeDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, int areath1, int areath2, vector<cv::Point>&carSize);
	void computeConvertFactor(vector<cv::Point2f> &picPt, vector<cv::Point2f> &realPt, cv::Mat &factor);
	void computeReal(cv::Mat &factor, cv::Point2f picPt, cv::Point2f &realPt);


	bool isPtInQuadrangle(const vector<Point> &pt, const Point &center);

private:
	//CvFGDetector *m_pFG;
	//cv::Ptr<cv::BackgroundSubtractor> bg_model;
	myBackgroundSubtractorMOG2 myBgSubtractor;
	int ID=0;
	int ListLen=0;
	cv::Mat hsv, hue, mask, hist, backproj;
	cv::MatND RoiHist1;
	cv::Mat img_Gray;
	//CvSVM svm;
	//cv::Ptr<cv::ml::SVM> svm;
	vector<TrackRect> tracktarRects;


public:
	//��������¼����
	Direction direct[4];
	vector<EventsResults> eventsResults;
	SituatResults situationResults;
	LONGLONG erodeAvg , erodeMax, erodeMin , dilateAvg , dilateMax , dilateMin ;
	LONGLONG labelAvg, labelMax, labelMin;
};

#endif