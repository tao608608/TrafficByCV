#include "mytime.h"
#include "IntelligentTraffic.h"
#include "systemConf.h"
#include "VlcStreamFromCamera.h"
#include <fstream>
#include <iostream>
#include<stdio.h> 
#include <shlwapi.h>
#include "yolo_v2_class.hpp" 
#pragma comment(lib,"Shlwapi.lib")

#define TIME_INFO
#define DRAW_INFO
//#define DETECT_INFO

cv::Mat org, dst, img, tmp;
int detectNum = 0;
IntelligentTraffic traffic;
systemConf systemconf;
string Time[2];

#ifdef TIME_INFO
LONGLONG totalAvg = 0, totalMax = 0, totalMin = 0;
LONGLONG frameAvg = 0, frameMax = 0, frameMin = 0;
LONGLONG fgAvg = 0, fgMax = 0, fgMin = 0;
LONGLONG trackAvg = 0, trackMax = 0, trackMin = 0;
LONGLONG detectAvg = 0, detectMax = 0, detectMin = 0;
LONGLONG classAvg = 0, classMax = 0, classMin = 0;
LONGLONG analyAvg = 0, analyMax = 0, analyMin = 0;
bool first = false;
#endif

#ifdef PLATFORM_LINUX
/*long elapsed_ms()
{
  static struct timeval startime;
  struct timeval currentime;
  struct timezone tz;
  long r;
  
  gettimeofday(&currentime, &tz);
  r = (currentime.tv_sec - startime.tv_sec)*1000 + (currentime.tv_usec - startime.tv_usec)/1000;
  startime.tv_sec = currentime.tv_sec;
  startime.tv_usec = currentime.tv_usec;
  return r;
}*/
long GetTickCount()
{
  static struct timeval initime;
  static char started = 0;//flag indicated if has gotten a start time base
  struct timeval currentime;
  struct timezone tz;
  if (!started)
  {
   gettimeofday(&initime, &tz);
   started = 1;
  }
  gettimeofday(&currentime, &tz);
  return (currentime.tv_sec - initime.tv_sec)*1000 + (currentime.tv_usec - initime.tv_usec)/1000;
}
int _kbhit() 
{
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
//#define PRINT_ELAPSED(str) {cout << str << elapsed_ms() << "ms" << endl}
#else
//#define PRINT_ELAPSED(str) {}
#endif

void Unchar2Mat(cv::Mat &image, unsigned char*imData)
{
	for (int i = 0; i < image.rows; i++)
	{
		for (int j = 0; j < image.cols; j++)
		{
			image.at<uchar>(i, 3 * j) = imData[3 * i*image.cols + 3 * j];
			image.at<uchar>(i, 3 * j + 1) = imData[3 * i*image.cols + 3 * j + 1];
			image.at<uchar>(i, 3 * j + 2) = imData[3 * i*image.cols + 3 * j + 2];
		}
	}
}

//void CALLBACK TimerProc(void* p)
//{
//	systemconf.writeTxtandXml(traffic.eventsResults, traffic.situationResults, Time);
//	traffic.eventsResults.clear();
//}

int main()
{
	//读入xml 配置文件
	//std::string cfg_filename = "yolov3.cfg";
	//std::string weight_filename = "yolov3.weights";
	const char* cfg_filename = "yolov3-tiny.cfg";
	const char*weight_filename = "yolov3-tiny.weights";
	//Detector obj(cfg_filename, weight_filename);
	init(cfg_filename, weight_filename, 0);
	systemconf.SystemConfInit();
	systemconf.m_imgheight =1080 ;
	systemconf.m_imgwidth = 1920;
	int imageHeight = systemconf.m_imgheight;
	int imageWidth = systemconf.m_imgwidth;
	int reginNum = 4;
	vector<vector<cv::Point>>detectRegions;

	for (int i = 0; i < reginNum; i++)//获取车道
	{
		vector<cv::Point>subRegionPt(4);
		if (systemconf.m_regionOut[i].isOn)
		{
			for (int j = 0; j < 4; j++)
			{
				subRegionPt[j].x = systemconf.m_regionOut[i].m_region[j].x;
				subRegionPt[j].y = systemconf.m_regionOut[i].m_region[j].y;
			}
			detectRegions.push_back(subRegionPt);
			subRegionPt.clear();

			traffic.direct[i].ptFirst.x = systemconf.m_regionOut[i].m_directtion[0].x;
			traffic.direct[i].ptFirst.y = systemconf.m_regionOut[i].m_directtion[0].y;
			traffic.direct[i].ptEnd.x = systemconf.m_regionOut[i].m_directtion[1].x;
			traffic.direct[i].ptEnd.y = systemconf.m_regionOut[i].m_directtion[1].y;
		}
		else
		{
			detectRegions.push_back(subRegionPt);
		}
	}

	//初始化掩膜
	cv::Mat img_mask = cv::Mat::zeros(imageHeight, imageWidth, CV_8UC1);
	for (int i = 0; i < reginNum; i++)
	{
		if (systemconf.m_regionOut[i].isOn)
		{
			cv::Point maskRegion[4] = { detectRegions[i][0], detectRegions[i][1], detectRegions[i][3], detectRegions[i][2] };
			fillConvexPoly(img_mask, maskRegion, 4, cv::Scalar(255), 8, 0);
		}
	}
	//掩模外接矩形
	vector<cv::Point> cmask;
	for (int i = 0; i < imageHeight; ++i)
	{
		const uchar* maskptr = img_mask.ptr<uchar>(i);
		for (int j = 0; j < imageWidth;++j)
		{
			uchar data = maskptr[j];
			if (data == 255)
				cmask.push_back(cv::Point(j,i));
		}

	}
	/*std::stable_sort(cmask.begin(), cmask.end(), []( const cv::Point& p1, const cv::Point& p2){return p1.x < p2.x; });
	RotatedRect imgrect1(cv::minAreaRect(cmask));
	Rect imgrect = imgrect1.boundingRect();*/
	Rect imgrect = boundingRect(cmask);
	//imshow("0", img_mask);
	vector<area_direction> areas(4);
	int index_road_on;
	Point ptFirst, ptEnd;
	for (int i = 0; i < reginNum; i++)
	{
		vector<cv::Point> point(4);
		areas[i].use = systemconf.m_regionOut[i].isOn;
		if (systemconf.m_regionOut[i].isOn)
		{
			for (int j = 0; j < 4; j++)
			{
				point[j].x = systemconf.m_regionOut[i].m_region[j].x-imgrect.x;
				point[j].y = systemconf.m_regionOut[i].m_region[j].y-imgrect.y;
				areas[i].area.push_back(point[j]);
			}
			//detectRegions.push_back(point);
			point.clear();
			
			ptFirst.x = systemconf.m_regionOut[i].m_directtion[0].x - imgrect.x;
			ptFirst.y = systemconf.m_regionOut[i].m_directtion[0].y - imgrect.y;
			ptEnd.x = systemconf.m_regionOut[i].m_directtion[1].x - imgrect.x;
			ptEnd.y = systemconf.m_regionOut[i].m_directtion[1].y - imgrect.y;
			areas[i].direction.push_back(ptFirst);
			areas[i].direction.push_back(ptEnd);
			index_road_on = i;//只能记录一个开启的车道
		}
	}

	//初始化vlc
	/*char szModuleFilePath[MAX_PATH];
	char vedioPath[MAX_PATH];
	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
	strcpy(vedioPath, szModuleFilePath);
	strcat(vedioPath, "\\Data\\");
	string videoFile = "test.avi";
	strcat(vedioPath, videoFile.c_str());*/
	cv::VideoCapture capture;
	capture.open("D:\\1.avi");
	//rtsp://admin:tao608608@202.115.52.223:554/h264/ch1/main/av_stream
	//capture.open(vedioPath);
	if (!capture.isOpened())
	{
		std::cout << "read video failure" << std::endl;
		return -1;
	}
	/*CVlcStreamFromCamera vlcCam;
	bool init = vlcCam.InitilizeCameraStream();
	if (!init)
	{
	std::cout << "read video failure" << std::endl;
	}*/
	else
	{
		//初始化
		cv::Mat img_src = cv::Mat(imageHeight, imageWidth, CV_8UC3);
		cv::Mat img_save = cv::Mat(imageHeight, imageWidth, CV_8UC3);
		cv::Mat img_fg = cv::Mat(imgrect.height, imgrect.width, CV_8UC1);

		int learn = 0;
		vector<cv::Rect>rects;
		set<int> preID;
		map<int, IntelligentTraffic::targetMessage> trackRoad;
		//事件判断变量初始化
		int *carNum = new int[reginNum]();

		vector<vector<cv::Point> >carSize(reginNum);

		vector<vector<IntelligentTraffic::tarTrigger> > tarTriggerSeris(reginNum);

		int *numTh = new int[reginNum]();
		int *speed = new int[reginNum]();
		for (int i = 0; i < reginNum; i++)
		{
			numTh[i] = systemconf.m_regionOut[i].m_jamRule.m_num;
			speed[i] = systemconf.m_regionOut[i].m_jamRule.m_speed;

		}
		int isStop = 0, isReverse = 0, isDriveOut = 0, isPedestrain = 0, isLoss = 0;
		bool isJam = false;


		vector<vector<IntelligentTraffic::tarTrigger> > tarTriggerStop(reginNum);

		vector<vector<IntelligentTraffic::tarTrigger> > outMessagebyDriveOut(reginNum);

		vector<vector<IntelligentTraffic::tarTrigger> > tarTriggerMan(reginNum);

		vector<vector<IntelligentTraffic::tarTrigger> > tarTriggerLoss(reginNum);

		traffic.fGdetectInit();
		traffic.tarclassInit();

		//解析视频流串
		/*vector<string> CameraStream;
		systemconf.parseCameraListXml(CameraStream);
		int con=0;
		if (CameraStream.size()>0)
		{
		con= vlcCam.ConnectCameraStream((char*)(CameraStream[0]).c_str(), 1280, 720);
		}*/

		//读入视频流
		bool isCon;

		//启动线程
		/*Timer timer;
		int m = 10;
		int *p = &m;
		timer.registerHandler(TimerProc, p);
		timer.setInterval(30000);
		timer.Start();*/
		int i = 0;
		while (1)
		{
			/*if (i % 2 != 0||i==0)
			{
				capture >> img_src;
				i++;
				continue;
			}*/
			capture >> img_src;
			i++;
			LONGLONG framestart, framefinish;
			framestart = GetTickCount();
			if (i == 0)
			{
				capture >> img_src;
			}
			
			//resize(img_src, img_src, Size(imageHeight, imageWidth));
			framefinish = GetTickCount();
			frameMax = ((framefinish - framestart) > frameMax) ? (framefinish - framestart) : frameMax;
			if (first==false)
			{
				frameMin = framefinish - framestart;
			}
			frameMin = ((framefinish - framestart) < frameMin) ? (framefinish - framestart) : frameMin;
			frameAvg += (framefinish - framestart);
#ifdef DETECT_INFO
			cout << "frameGet over; ";
#endif
			
			/*isCon = vlcCam.GetConnectstaton();
			if (isCon)
			{
			imageData = vlcCam.QueryFrame();
			if (imageData != NULL)
			{
			Unchar2Mat(img_src, imageData);
			}
			else
			{
			continue;
			}
			}
			else
			{
			continue;
			}
			img_save=img_src.clone();*/

			if (img_src.empty())
			{
				//cv::destroyAllWindows();
				cout << "no frame" ;
				break;
			}
			//imshow("1",img_src);

			cv::Mat img_detect= img_src(imgrect);

			//前景检测
//			LONGLONG fgstart, fgfinish;
//			fgstart = GetTickCount();
			//traffic.fGDetect(img_detect, img_fg, img_mask);
//			img_fg.setTo(cv::Scalar(0));
//			traffic.fGDetect(img_detect, img_fg, maskDetect,learn);//通过前景建模 得到马路
//			//cv::imshow("img_detect", img_detect);
//			//cv::imshow("img_fg", img_fg);
//			//cv::imshow("maskDetect", maskDetect);
//			//waitKey(10);
//			fgfinish = GetTickCount();
//			fgMax = ((fgfinish - fgstart) > fgMax) ? (fgfinish - fgstart) : fgMax;
//			if (first==false)
//			{
//				fgMin = fgfinish - fgstart;
//			}
//			fgMin = ((fgfinish - fgstart)< fgMin) ? (fgfinish - fgstart) : fgMin;
//			fgAvg += (fgfinish - fgstart);
//#ifdef DETECT_INFO
//			cout << "fgDetect over; ";
//#endif
//			
//
//			learn++;
//			if (learn < 50)
//			{
//				continue;
//			}
//
//			//得到系统时间
			
			time_t t = time(0);
			char day[15], hour[4], minute[4], second[4];
			strftime(day, sizeof(day), "%Y-%m-%d", localtime(&t));
			strftime(hour, sizeof(hour), "%H", localtime(&t));
			strftime(minute, sizeof(minute), "%M", localtime(&t));
			strftime(second, sizeof(second), "%S", localtime(&t));
			ostringstream m_hour, m_mimute, m_second;
			m_hour << hour; m_mimute << minute; m_second << second;
			string Second = m_hour.str() + "-" + m_mimute.str() + "-" + m_second.str();
			Time[0].assign(day); Time[1].assign(Second);

			//目标检测
			LONGLONG Detectstart, Detectfinish;
			Detectstart = GetTickCount();//返回系统时间
			//std::vector<bbox_t> a;
			//cv::imwrite("1.jpg",img_detect);
			/*cv::Mat img_detect1 = cv::Mat(maskDetect.rows, maskDetect.cols, CV_8UC3);

			img_detect.copyTo(img_detect1);
			cv::imshow("1", maskDetect);
			waitKey(1);
			for (int i = 0; i < maskDetect.rows; ++i)
			{
				const uchar* maskptr = maskDetect.ptr<uchar>(i);
				for (int j = 0; j < maskDetect.cols; ++j)
				{
					uchar data = maskptr[j];
					if (data == 255)
					{
						img_detect1.at<uchar>(i, j) = 0;
					}
				}

			}


			imshow("2", img_detect1);
			waitKey(0);*/
			bbox_t_container a;
			int obj_num=detect_op(img_detect, a);
			//a = obj.detect(img_detect);//5（bus）或者6（car）
			cv::Rect rect;
			int nums = 0;
			rects.clear();
			IntelligentTraffic object;
			for (int i = 0; i < obj_num; i++)
			{
				Point center;
				center.x = a.candidates[i].x + a.candidates[i].w / 2;
				center.y= a.candidates[i].y + a.candidates[i].h / 2;
				bool isin = object.isPtInQuadrangle(areas[index_road_on].area, center);

				if ((a.candidates[i].obj_id != 0&&a.candidates[i].obj_id != 1 && a.candidates[i].obj_id != 2 && a.candidates[i].obj_id != 3&& a.candidates[i].obj_id != 7 && a.candidates[i].obj_id != 5)||!isin)
				//if ((a[i].obj_id != 5 && a[i].obj_id != 6) || !isin)
				//if ((a[i].obj_id != 2) || !isin)
				{
					continue;
				}
				rect.x = a.candidates[i].x;
				rect.y = a.candidates[i].y;
				rect.height = a.candidates[i].h;
				rect.width = a.candidates[i].w;
				//rectangle(img_detect, rect, Scalar(0, 0, 225), 1, 1, 0);
				rects.push_back(rect);
				nums++;
			}
			//int nums = traffic.tarDetect(img_fg, rects);//通过形态学操作后将目标以连通区域标记出
			/*cv::Mat img_f = img_fg(rects[0]);
			cv::imshow("a", img_f);
			waitKey(20);*/
			
			Detectfinish = GetTickCount();
			detectMax = ((Detectfinish - Detectstart) > detectMax) ? (Detectfinish - Detectstart) : detectMax;
			if (first==false)
			{
				detectMin = Detectfinish - Detectstart;
			}
			detectMin = ((Detectfinish - Detectstart) < detectMin) ? (Detectfinish - Detectstart) : detectMin;
			detectAvg += (Detectfinish - Detectstart);
#ifdef DETECT_INFO
			cout << "tarDetect over; ";
#endif
			
			//cout << "FrameNum:" << detectNum << endl;
			//目标跟踪
			LONGLONG Trackstart, Trackfinish;
			
			Trackstart = GetTickCount();
			/*imshow("1", img_detect);
			waitKey(5);*/
			traffic.tarTrack(img_detect, rects, trackRoad, areas);
			
			Trackfinish = GetTickCount();
			trackMax = ((Trackfinish-Trackstart) > trackMax) ? (Trackfinish-Trackstart) : trackMax;
			if (first == false)
			{
				trackMin = Trackfinish - Trackstart;
			}
			trackMin = ((Trackfinish-Trackstart) < trackMin) ? (Trackfinish-Trackstart) : trackMin;
			trackAvg += ((Trackfinish-Trackstart));
#ifdef DETECT_INFO
			cout << "tarTrack over; ";
#endif
			

			//目标分类
			LONGLONG Classstart, Classfinish;
			Classstart = GetTickCount();
			traffic.tarClass(img_detect, trackRoad);
			Classfinish = GetTickCount();
			classMax = ((Classfinish - Classstart) > classMax) ? (Classfinish - Classstart) : classMax;
			if (first == false)
			{
				classMin = Classfinish - Classstart;
			}
			classMin = ((Classfinish - Classstart) < classMin) ? (Classfinish - Classstart) : classMin;
			classAvg += (Classfinish - Classstart);
#ifdef DETECT_INFO
			cout << "tarClass over; ";
#endif
			
			

			//============================事件检测=================================
			LONGLONG analystart, analyfinish;
			analystart = GetTickCount();
			if (detectNum % 10 == 0)
			{
				//====================车道 i ===================================
				for (int i = 0; i < reginNum; i++)
				{
					if (systemconf.m_regionOut[i].isOn)
					{
						if (systemconf.m_regionOut[i].m_stopRule.m_use)//停留
						{
							isStop = traffic.stopIncidentDetect(trackRoad, areas[i].area, tarTriggerStop[i], Time, img_save);
							systemconf.saveEventsPic(isStop, systemconf.m_regionOut[i].m_stopRule.m_savePic, traffic.eventsResults);
						}

						if (systemconf.m_regionOut[i].m_reversedriveRule.m_use)//逆行
						{
							isReverse = traffic.reverseDriveIncidentDetect(trackRoad, areas[i].area, traffic.direct[i], tarTriggerSeris[i], Time, img_save);
							systemconf.saveEventsPic(isReverse, systemconf.m_regionOut[i].m_reversedriveRule.m_savePic, traffic.eventsResults);
						}

						if (systemconf.m_regionOut[i].m_pedestrianEntryRule.m_use)//行人
						{
							isPedestrain = traffic.pedestrianEntryIncidentDetect(trackRoad, areas[i].area, tarTriggerMan[i], Time, img_save);
							systemconf.saveEventsPic(isPedestrain, systemconf.m_regionOut[i].m_pedestrianEntryRule.m_savePic, traffic.eventsResults);
						}

						if (systemconf.m_regionOut[i].m_jamRule.m_use)//堵车拥挤
						{
							if (!isJam)
							{
								isJam = traffic.jamIncidentDetect(trackRoad, areas[i].area, numTh[i], speed[i], Time, img_save);
								systemconf.saveSituatPic(isJam, systemconf.m_regionOut[i].m_jamRule.m_savePic, Time, traffic.situationResults);
							}
						}

						if (systemconf.m_regionOut[i].m_driveOutOfBorderRule.m_use)//驶出道路
						{
							isDriveOut = traffic.driveOutOfBorderIncidentDetect(trackRoad, areas[i].area, outMessagebyDriveOut[i], Time, img_save);
							systemconf.saveEventsPic(isDriveOut, systemconf.m_regionOut[i].m_driveOutOfBorderRule.m_savePic, traffic.eventsResults);
						}

						if (systemconf.m_regionOut[i].m_lossRule.m_use)
						{
							isLoss = traffic.lossIncidentDetect(trackRoad, areas[i].area, tarTriggerLoss[i], Time, img_save);
							systemconf.saveEventsPic(isLoss, systemconf.m_regionOut[i].m_lossRule.m_savePic, traffic.eventsResults);
						}
					}
				}
			}
		
			//================车辆数量&拥塞检测============================
			for (int i = 0; i < reginNum; ++i)
			{
				if (systemconf.m_regionOut[i].isOn)
				{
					if (systemconf.m_regionOut[i].m_carNumRule.m_use)
					{
						traffic.carNumDetect(trackRoad, areas[i].area, preID, carNum[i], Time);
					}
					if (systemconf.m_regionOut[i].m_carSizeRule.m_use)
					{
						traffic.CarSizeDetect(trackRoad, areas[i].area, 6000, 18000, carSize[i]);
					}
					if (carNum[i] != 0)
					{
						traffic.situationResults.carNum += carNum[i];
						carNum[i] = 0;
					}
				}
			}
			//cout << "CarNum:" << traffic.situationResults.carNum << endl;

			if (isJam)
			{
				traffic.situationResults.jam = true;

			}
			analyfinish = GetTickCount();
			analyMax = ((analyfinish - analystart) > analyMax) ? (analyfinish - analystart) : analyMax;
			if (first==false)
			{
				analyMin = analyfinish - analystart;
			}
			analyMin = ((analyfinish - analystart) < analyMin) ? (analyfinish - analystart) : analyMin;
			analyAvg += (analyfinish - analystart);
#ifdef DETECT_INFO
			cout << "Analysis over; ";
#endif

			if (detectNum % 100 == 0)
			{
				systemconf.writeTxtandXml(traffic.eventsResults, traffic.situationResults, Time);
			}

			//检测帧数归零
			if (detectNum > 10000)
			{
				detectNum = 0;
			}
			detectNum++;


			//==================================画图=================================================
			//=====================车道i==========================
			for (int i = 0; i < reginNum; i++)
			{
				if (systemconf.m_regionOut[i].isOn)
				{
					if (systemconf.m_regionOut[i].m_reversedriveRule.m_draw)
					{
						char tempReverse[16] = "REVERSE DRIVE";
						for (int j = 0; j < tarTriggerSeris[i].size(); j++)
						{
							tarTriggerSeris[i][j].pos.x += imgrect.x; tarTriggerSeris[i][j].pos.y += imgrect.y;
							putText(img_src, tempReverse, tarTriggerSeris[i][j].pos, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标
						}
					}
					if (systemconf.m_regionOut[i].m_jamRule.m_draw)
					{
						char tempjam[16] = "JAM";
						if (isJam == true)
						{
							cv::Point jamPt;
							jamPt.x = (detectRegions[i][0].x + detectRegions[i][1].x + detectRegions[i][2].x + detectRegions[i][3].x) / 4;
							jamPt.y = (detectRegions[i][0].y + detectRegions[i][1].y + detectRegions[i][2].y + detectRegions[i][3].y) / 4;
							jamPt.x += imgrect.x; jamPt.y += imgrect.y;
							putText(img_src, tempjam, jamPt, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标
							isJam = false;
						}

					}
					if (systemconf.m_regionOut[i].m_stopRule.m_draw)
					{
						char tempStop[16] = "STOP";
						for (int j = 0; j < tarTriggerStop[i].size(); j++)
						{
							tarTriggerStop[i][j].pos.x += imgrect.x; tarTriggerStop[i][j].pos.y += imgrect.y;
							putText(img_src, tempStop, tarTriggerStop[i][j].pos, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标
						}
					}
					if (systemconf.m_regionOut[i].m_driveOutOfBorderRule.m_draw)
					{
						char tempDriveOut[16] = "DRIVE OUT";
						for (int j = 0; j < outMessagebyDriveOut[i].size(); j++)
						{
							outMessagebyDriveOut[i][j].pos.x += imgrect.x; outMessagebyDriveOut[i][j].pos.y += imgrect.y;
							putText(img_src, tempDriveOut, outMessagebyDriveOut[i][j].pos, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标9
						}
					}
					if (systemconf.m_regionOut[i].m_pedestrianEntryRule.m_draw)
					{
						char tempPedestrian[16] = "PEDESTRIAN";
						for (int j = 0; j < tarTriggerMan[i].size(); j++)
						{
							tarTriggerMan[i][j].pos.x += imgrect.x; tarTriggerMan[i][j].pos.y += imgrect.y;
							putText(img_src, tempPedestrian, tarTriggerMan[i][j].pos, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标
						}
					}
					if (systemconf.m_regionOut[i].m_lossRule.m_draw)
					{
						char tempLoss[16] = "Loss";
						for (int j = 0; j < tarTriggerLoss[i].size(); j++)
						{
							tarTriggerLoss[i][j].pos.x += imgrect.x; tarTriggerLoss[i][j].pos.y += imgrect.y;
							putText(img_src, tempLoss, tarTriggerLoss[i][j].pos, cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 0, 255), 3, 8);//在窗口上显示坐标
						}
					}
				}
			}

			//标注检测到的目标
			map<int, IntelligentTraffic::targetMessage> ::iterator iter = trackRoad.begin();

			//int *tracknum = new int[trackRoad.size()]();
			//for (int i = 0; i < trackRoad.size(); i++)
			//{
			//	if (tracknum[i] != trackRoad[i].trackRoad.size())
			//	{
			//		iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
			//	}
			//	tracknum[i] = trackRoad[i].trackRoad.size();
			//}
			
			for (; iter != trackRoad.end(); iter++)
			{
				if (iter->second.updata)
				{
					iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
				}
				char tmp[10];
				switch (iter->second.MatchType)
				{
				case 0:
					//iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
					rectangle(img_src, iter->second.rect, cv::Scalar(0, 0, 0), 2, 8, 0);//新目标，黑色
					_itoa_s(iter->first, tmp, 10);
					sprintf(tmp, "%d", iter->first);
					
					putText(img_src, tmp, cv::Point(iter->second.rect.x, iter->second.rect.y), 0, 1, CV_RGB(0, 0, 255), 2);
					break;
				case 1:
					//iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
					rectangle(img_src, iter->second.rect, cv::Scalar(0, 255, 0), 2, 8, 0);//跟踪成功(绿色)
					_itoa_s(iter->first, tmp, 10);
					sprintf(tmp, "%d", iter->first);
					putText(img_src, tmp, cv::Point(iter->second.rect.x, iter->second.rect.y), 0, 1, CV_RGB(0, 0, 255), 2);
					break;
				case 2:
					//iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
					rectangle(img_src, iter->second.rect, /*cv::Scalar(255, 255, 0)*/cv::Scalar(0, 0, 255), 2, 8, 0);// 预测成功(红色)
					_itoa_s(iter->first, tmp, 10);
					sprintf(tmp, "%d", iter->first);
					putText(img_src, tmp, cv::Point(iter->second.rect.x, iter->second.rect.y), 0, 1, CV_RGB(0, 0, 255), 2);
					break;
				case 3:
					//iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
					rectangle(img_src, iter->second.rect, cv::Scalar(255, 0, 0), 2, 8, 0); //检测成功(蓝色)
					_itoa_s(iter->first, tmp, 10);
					sprintf(tmp, "%d", iter->first);
					putText(img_src, tmp, cv::Point(iter->second.rect.x, iter->second.rect.y), 0, 1, CV_RGB(0, 0, 255), 2);
					break;
				case 4:
					//iter->second.rect.x += imgrect.x; iter->second.rect.y += imgrect.y;
					rectangle(img_src, iter->second.rect, cv::Scalar(255, 0, 255), 2, 8, 0);//4补充检测(洋红)
					_itoa_s(iter->first, tmp, 10);
					sprintf(tmp, "%d", iter->first);
					putText(img_src, tmp, cv::Point(iter->second.rect.x, iter->second.rect.y), 0, 1, CV_RGB(0, 0, 255), 2);
					break;
				default:
					break;
				}
				
			}

			//画出车道
			for (int i = 0; i < reginNum; i++)
			{
				//if (systemconf.m_regionOut[i].isOn)
				{
					line(img_src, detectRegions[i][0], detectRegions[i][1], cv::Scalar(255, 255, 255, 0), 4, 8, 0);
					line(img_src, detectRegions[i][2], detectRegions[i][3], cv::Scalar(0, 0, 255, 0), 4, 8, 0);
					line(img_src, detectRegions[i][0], detectRegions[i][2], cv::Scalar(255, 255, 255, 0), 4, 8, 0);
					line(img_src, detectRegions[i][1], detectRegions[i][3], cv::Scalar(255, 255, 255, 0), 4, 8, 0);
				}
			}
			
#ifdef DRAW_INFO
			//cv::namedWindow("前景掩膜", CV_WINDOW_NORMAL);
			cv::namedWindow("检测结果", 1);
			//imshow("处理区域", img_detect);
			/*imshow("前景掩膜", img_fg);*/
			imshow("检测结果", img_src);
			cv::waitKey(5);
#endif
			
			LONGLONG finish = GetTickCount();
			totalMax = ((finish - framestart) > totalMax) ? (finish - framestart) : totalMax;
			if (first==false)
			{
				totalMin = finish - framestart;
			}
			totalMin = ((finish - framestart) < totalMin) ? (finish - framestart) : totalMin;
			totalAvg = totalAvg + (finish - framestart);
#ifdef DETECT_INFO
			cout << "Completed!" << endl;
#endif
			
			first = true;
			int flag = _kbhit();
			//int b = bioskey(0) % 256;  /*这里加入对不同键盘输入的处理*/

#ifdef TIME_INFO
			if (flag!=0)
			{
				frameAvg = frameAvg / (learn-50);
				cout << "frameAvg:" << frameAvg << "ms" << " "<<"frameMax:" << frameMax << "ms" << " "<< "frameMin:" << frameMin << "ms" << endl;

				fgAvg = fgAvg / (learn - 50);
				cout << "fgAvg:" << fgAvg << "ms" <<" "<< "fgMax:" << fgMax << "ms" <<" "<< "fgMin:" << fgMin << "ms" << endl;

				traffic.erodeAvg = traffic.erodeAvg / (learn - 50);
				cout << "    erodeAvg:" << traffic.erodeAvg << "ms" <<" "<< "erodeMax:" << traffic.erodeMax << "ms" <<" "<< "erodeMin:" << traffic.erodeMin << "ms" << endl;

				traffic.dilateAvg = traffic.dilateAvg / (learn - 50);
				cout << "    dilateAvg:" << traffic.dilateAvg << "ms" <<" "<< "dilateMax:" << traffic.dilateMax << "ms" <<" "<< "dilateMin:" << traffic.dilateMin << "ms" << endl;

				traffic.labelAvg = traffic.labelAvg / (learn - 50);
				cout << "    labelAvg:" << traffic.labelAvg << "ms" <<" "<< "labelMax:" << traffic.labelMax << "ms" <<" "<< "labelMin:" << traffic.labelMin << "ms" << endl;

				detectAvg = detectAvg / (learn - 50);
				cout << "detectAvg:" << detectAvg << "ms" <<" "<< "detectMax:" << detectMax << "ms" <<" "<< "detectMin:" << detectMin << "ms" << endl;

				trackAvg = trackAvg / (learn - 50);
				cout << "trackAvg:" << trackAvg << "ms" <<" " << "trackMax:" << trackMax << "ms" <<" "<< "trackMin:" << trackMin << "ms" << endl;

				classAvg = classAvg / (learn - 50);
				cout << "classAvg:" << classAvg << "ms" <<" "<< "classMax:" << classMax << "ms" <<" "<< "classMin:" << classMin << "ms" << endl;

				analyAvg = analyAvg / (learn - 50);
				cout << "analyAvg:" << analyAvg << "ms" <<" "<< "analyMax:" << analyMax << "ms" <<" "<< "analyMin:" << analyMin << "ms" << endl;

				totalAvg = totalAvg / (learn - 50);
				cout << "totalAvg:" << totalAvg << "ms" <<" "<< "totalMax:" << totalMax << "ms" <<" "<< "totalMin:" << totalMin << "ms" << endl;
				cout << "good frameNum:" << learn - 50 << endl;
				break;
			}
#endif // TIME_INFO

		}
		//timer.Cancel();
	}
	system("pause");
}


