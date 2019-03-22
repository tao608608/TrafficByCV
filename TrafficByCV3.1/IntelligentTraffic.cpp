#include "IntelligentTraffic.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "mymorph.h"
#include <shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")

# define debug
int m = 0;
bool First = false;

IntelligentTraffic::IntelligentTraffic(void)
{
	ListLen = 0;
	ID = 0;
	erodeAvg = 0; erodeMax = 0; erodeMin = 0;
	dilateAvg = 0; dilateMax = 0; dilateMin = 0;
}

IntelligentTraffic::~IntelligentTraffic(void)
{

}

void IntelligentTraffic::fGdetectInit(int method)
{
	myBgSubtractor.createmyBackgroundSubtractorMOG2(300, 20, true);
	//bg_model = cv::createBackgroundSubtractorMOG2(300, 20, false);
	//bg_model=cv::createBackgroundSubtractorKNN(500, 400, false);
	//createBackgroundSubtractorKNN().dynamicCast<BackgroundSubtractor>();
}
void IntelligentTraffic::fGDetect(cv::Mat &inputImg, cv::Mat&outputfGImg,cv::Mat& maskImg,int learn)
{

	/*if (outputfGImg.empty())
		outputfGImg.create(inputImg.size(), inputImg.type());*/
	//bg_model->apply(inputImg, outputfGImg, -1);//最后一个参数：背景学习速率

	myBgSubtractor.apply(inputImg, outputfGImg, maskImg);
	

}

//============目标检测=========
//输入：inputMask（前景掩膜）
//输出：tarRects（目标外接矩形）+ num（目标数目）
int IntelligentTraffic::tarDetect(cv::Mat inputMask, vector<cv::Rect>&tarRects, int niters)//通过形态学操作后将目标以连通区域标记出
{
	vector<cv::Rect> tarRectold = tarRects;
	tarRects.clear();

	//腐蚀+膨胀（方核）
	cv::Mat outputimg(inputMask.size(), CV_8UC1);
	//cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	//cv::Mat kernel1 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 7));

	LONGLONG edstart, edfinish;
	edstart = GetTickCount();
	//erode(inputMask, outputimg, kernel, cv::Point(-1, -1), 1);
#ifdef debug
	myOpErode(inputMask, outputimg, cv::Size(2, 2));
#else
	myOpErode(inputMask, outputimg,cv::Size(3,3));
#endif
//#ifdef debug
//	myOpErode(inputMask, outputimg, cv::Size(1, 1));
//#else
//	myOpErode(inputMask, outputimg, cv::Size(1, 1));
//#endif

	//cout << "     Erode over!" << endl;
	edfinish = GetTickCount();
	erodeMax = ((edfinish - edstart) > erodeMax) ? (edfinish - edstart) : erodeMax;
	if (First == false)
	{
		erodeMin = edfinish - edstart;
	}
	erodeMin = ((edfinish - edstart) < erodeMin) ? (edfinish - edstart) : erodeMin;
	erodeAvg += (edfinish - edstart);
	
	

    edstart = edfinish;

//#ifdef debug
//	myOpDilate(outputimg, inputMask, cv::Size(1, 1));
//#else
//	myOpDilate(outputimg, inputMask, cv::Size(1,1));
//#endif // debug

#ifdef debug
	myOpDilate(outputimg, inputMask, cv::Size(7,7));
#else
	myOpDilate(outputimg, inputMask, cv::Size(7,7));
#endif // debug
	//cout << "     Dilate over!" << endl;
	//dilate(outputimg, inputMask, kernel1, cv::Point(-1, -1), 1);
	edfinish = GetTickCount();
	dilateMax = ((edfinish - edstart) > dilateMax) ? (edfinish - edstart) : dilateMax;
	if (First == false)
	{
		dilateMin = edfinish - edstart;
	}
	dilateMin = ((edfinish - edstart) < dilateMin) ? (edfinish - edstart) : dilateMin;
	dilateAvg += (edfinish - edstart);

	cv::Mat labelImage(inputMask.size(), CV_8UC1);
	int nLabels = connectedComponents(inputMask, labelImage, 8);//标记连通区图
	LONGLONG labelstart, labelfinish;
	labelstart = GetTickCount();
	vector<vector<cv::Point> >labelPixles(nLabels);
	for (int r = 0; r < labelImage.rows; ++r){
		int *data = labelImage.ptr<int>(r);
		for (int c = 0; c < labelImage.cols; ++c){
			int label = data[c];
			
			if (label==0)
			{
				continue;
			}
			labelPixles[label - 1].push_back(cv::Point(c, r));
		}
	}	
	labelfinish = GetTickCount();
	labelMax = ((labelfinish - labelstart) > labelMax) ? (labelfinish - labelstart) : labelMax;
	if (First == false)
	{
		labelMin = labelfinish - labelstart;
	}
	labelMin = ((labelfinish - labelstart) < labelMin) ? (labelfinish - labelstart) : labelMin;
	labelAvg += (labelfinish - labelstart);
	First = true;


	int num = 0;
	for (int i = 0; i < nLabels; i++)
	{
		const vector<cv::Point>& c1 = labelPixles[i];
        int area = c1.size();
		if (area < 400)
		{
			if (area<300)
			continue;
			else
			{
				if (tarRectold.size() != 0)
				{
					/*if (m == 322)
					{
					waitKey(0);
					}*/
					cv::Rect rb = cv::boundingRect(cv::Mat(c1));
					bool judge = false;
					for (int i = 0; i < tarRectold.size(); i++)
					{
						Point center;
						center.x = tarRectold[i].x + tarRectold[i].width / 2;
						center.y= tarRectold[i].y + tarRectold[i].height / 2;
						bool t1 = rb.contains(center);
						int w = min(rb.x + rb.width, tarRectold[i].x + tarRectold[i].width)- max(rb.x, tarRectold[i].x);
						int h = min(rb.y + rb.height, tarRectold[i].y + tarRectold[i].height)- max(rb.y, tarRectold[i].y);
						bool t2 = (w*h)>(tarRectold[i].width*tarRectold[i].height/ 2);
						/*bool t2 = abs(int(((*vIter).width - tracktarRects[i].lastTrackWindow.width))) < 10;
						bool t3 = abs(int((*vIter).height - tracktarRects[i].lastTrackWindow.height))< 10;*/
						if (t1 && t2)
						{
							judge = true;
							break;
						}

					}
					if (!judge)
					{
						continue;
					}

				}
				else
				{
					continue;
				}

			}
		}
			
		cv::Rect rb = cv::boundingRect(cv::Mat(c1));
		if (float(area) / float(rb.area()) < 0.4)
			continue;
		tarRects.push_back(rb);
		num++;
	}
	vector<int> index;
	for (int i = 0; i < tarRects.size(); i++)
	{
		for (int j = 0; j < tarRects.size(); j++)
		{
			if (i == j)
			{
				continue;
			}
			else
			{
				bool t1 = abs(tarRects[i].x + tarRects[i].width)>= float(tarRects[j].x) ;
				bool t2 = abs(tarRects[i].x + tarRects[i].width)<= float(tarRects[j].x + tarRects[j].width);
				//bool t2 = float(tarRects[i].width) / float(tarRects[j].width) < 1.1;
				//bool t3 = float(tarRects[i].width) / float(tarRects[j].width) > 0.9;
				bool t3 = tarRects[i].y + tarRects[i].height - tarRects[j].y>=min(tarRects[i].height, tarRects[j].height)/4;
				bool t4 = tarRects[i].y + tarRects[i].height > tarRects[j].y;
				bool t6 = tarRects[i].y + tarRects[i].height < tarRects[j].y + tarRects[j].height;
				bool t5 = tarRects[i].y < tarRects[j].y;
				if (t1&&t3&&t2&&t4&&t5&&t6)
				{
					cv::Rect addrect(0,0,0,0);
					addrect.x = min(tarRects[i].x, tarRects[j].x);
					addrect.y = tarRects[i].y;
					addrect.width = max(tarRects[i].width, tarRects[j].width);
					addrect.height = tarRects[j].y + tarRects[j].height - tarRects[i].y;
					tarRects[i] = addrect;
					
					index.push_back(j);
				}
			}
		}
	}
	for (int i = 0; i < index.size(); i++)
	{
		tarRects.erase(tarRects.begin() + index[i]-i);
		num--;
	}
	labelPixles.clear();

	return num;

}
//辅助函数，模板匹配
void IntelligentTraffic::matchTemp(cv::Mat img, cv::Mat templ, cv::Rect &ret)
{
	cv::Mat result;
	int result_cols = img.cols - templ.cols + 1;
	int result_rows = img.rows - templ.rows + 1;
	/// 进行匹配和标准化
	matchTemplate(img, templ, result, cv::TM_CCOEFF);//模板匹配，templ为模板<img,
	normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
	/// 通过函数 minMaxLoc 定位最匹配的位置
	double minVal; double maxVal; cv::Point minLoc; cv::Point maxLoc;
	cv::Point matchLoc;
	minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
	/// 让我看看您的最终结果
	matchLoc = maxLoc;
	ret.x = matchLoc.x;
	ret.y = matchLoc.y;
	ret.width = templ.cols;
	ret.height = templ.rows;
}
//=================目标跟踪====================
//输入：目标外接矩形+原图
//输出：目标轨迹信息

// 三角形面积  
double crossArea3(const Point &pt1, const Point &pt2, const Point &pt3)
{
	/*double ax = pt2.x - pt1.x, ay = pt2.y - pt1.y, bx = pt2.x - pt3.x, by = pt2.y - pt3.y;*/
	return abs((pt1.x*pt2.y +pt1.y*pt3.x+pt2.x*pt3.y-pt3.x*pt2.y-pt1.x*pt3.y-pt2.x*pt1.y) / 2);
}

// 凸四边形面积  
double crossArea4(const Point &pt1, const Point &pt2, const Point &pt3, const Point &pt4)
{
	double S1 = crossArea3(pt1, pt2, pt3);
	double S2 = crossArea3(pt1, pt3, pt4);

	return S1 + S2;
}

bool IntelligentTraffic::isPtInQuadrangle(const vector<Point> &pt, const Point &center)
{
	cv::Point p1, p2, p3, p4;
	p1 = pt[0];
	p2 = pt[1];
	p4 = pt[2];
	p3 = pt[3];
	double S = crossArea4(p1, p2, p3, p4);

	double S1 = crossArea3(p1, p2, center);
	double S2 = crossArea3(p2, p3, center);
	double S3 = crossArea3(p3, p4, center);
	double S4 = crossArea3(p4, p1, center);

	return fabs(S - S1 - S2 - S3 - S4) <= 2;
}

//bool SVM_HOG(cv::Mat &ImgDetect, Rect &Rect)
//{
//	cv::HOGDescriptor hog(cv::Size(32, 64), cv::Size(8, 8), cv::Size(4, 4), cv::Size(4, 4), 9);
//	int DescriptorDim;
//	//加载模型文件//从XML文件读取训练好的SVM模型 
//	cv::Ptr<cv::ml::SVM> svm = cv::ml::SVM::load<cv::ml::SVM>("SVM_HOG_CAR_old.xml");
//
//	cv::Mat testImg = ImgDetect(Rect);
//	//cv::imshow("testImage", testImg);
//	//cv::waitKey(10);
//	resize(testImg, testImg, cv::Size(32, 64));
//	vector<float> descriptor;
//	//计算HOG描述子，检测窗口移动步长(4,4)  
//	hog.compute(testImg, descriptor, cv::Size(4, 4));
//	cv::Mat testFeatureMat = cv::Mat::zeros(1, 3780, CV_32FC1);//测试样本的特征向量矩阵  
//	for (int i = 0; i < descriptor.size(); i++)
//		testFeatureMat.at<float>(0, i) = descriptor[i];
//
//	//用训练好的SVM分类器对测试图片的特征向量进行分类  
//	int result = svm->predict(testFeatureMat);
//	/*cout << "分类结果：" << result << endl;*/
//	if (result < 0)
//		return false;
//	else
//		return true;
//	
//}




void IntelligentTraffic::tarTrack(cv::Mat &inputImg, vector<cv::Rect>&tarRects, map<int, targetMessage>&trackRoad, vector<area_direction> &areas)
{
	m++;
	if (m ==434)
	{
		waitKey(10);
	}
	cout << m << endl;
	map<int, IntelligentTraffic::targetMessage> ::iterator trackRoad_iter = trackRoad.begin();
	for (; trackRoad_iter != trackRoad.end(); trackRoad_iter++)
	{
		trackRoad_iter->second.updata = false;
	}

	///////////////////////////////////////////////////////////////////////////
	//////////////计算后来进行模板匹配时所需要的参数///////////////////////////
	cvtColor(inputImg, img_Gray, CV_BGR2GRAY);//对原图进行二值化
	//img_Gray = inputImg;
	int nums = tarRects.size();//得到目标数目
	cvtColor(inputImg, hsv, cv::COLOR_BGR2HSV);//转换色彩空间到hsv
	int _vmin = 10, _vmax = 256, smin = 30;//色调H、饱和度S、透明度V
	int hsize = 16;
	float hranges[] = { 0, 180 };
	const float* phranges = hranges;
	int channels[] = { 0, 1, 2 };
	const int histsize[] = { 16, 16, 16 };
	float rranges[] = { 0, 255 };
	float granges[] = { 0, 255 };
	float branges[] = { 0, 255 };
	const float *ranges[] = { rranges, granges, branges };
	inRange(hsv, cv::Scalar(0, smin, MIN(_vmin, _vmax)),
	cv::Scalar(180, 256, MAX(_vmin, _vmax)), mask);//提取图像中在阈值中间的部分（颜色块），mask输出图像
	int ch[] = { 0, 0 };//FromTo
	hue.create(hsv.size(), hsv.depth());
	mixChannels(&hsv, 1, &hue, 1, ch, 1);//取hsv第一个通道到hue
	/////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////


	//====================listLen:检测到的物体数目(第一帧之后)====================
	for (int j = 0; j<ListLen; j++)//判断目标是否被跟踪到
	{
		calcBackProject(&hue, 1, 0, tracktarRects[j].hHist, backproj, &phranges);//计算直方图的反向投影，输出backproj与待跟踪图像大小相同，每一个像素点表示该点为目标区域的概率。这个点越亮，该点属于物体的概率越大
		backproj &= mask;
		cv::Rect tempRect = tracktarRects[j].lastTrackWindow;//上一帧目标在图像上的位置
		tracktarRects[j].camshiftPos = CamShift(backproj, tempRect,
			cv::TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1));//计算上一帧跟踪当前帧位置

#ifdef debug
		bool test1 = tempRect.area() >= 1000;//不同分辨率设置不同！！！！参数
#else
		bool test1 = tempRect.area() >= 800;//不同分辨率设置不同！！！！参数
#endif // debug

		bool test2 = tracktarRects[j].camshiftPos.center.x>0;
		bool test3 = tracktarRects[j].camshiftPos.center.x<inputImg.cols - 1;
		bool test4 = tracktarRects[j].camshiftPos.center.y>0;
		bool test5 = tracktarRects[j].camshiftPos.center.y < inputImg.rows - 1;
		bool test6, test61, test7, test71;
		
		int x = areas[tracktarRects[j].areaid].direction[1].x - areas[tracktarRects[j].areaid].direction[0].x;
		int y = areas[tracktarRects[j].areaid].direction[1].y - areas[tracktarRects[j].areaid].direction[0].y;

		//test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x < tracktarRects[j].lastTrackWindow.width / 2;//物体帧与帧之间移动距离
		//test61 = -tracktarRects[j].lastTrackWindow.width / 5<tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x;//物体帧与帧之间移动距离
		//test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y < tracktarRects[j].lastTrackWindow.height / 2;//
		//test71 = -tracktarRects[j].lastTrackWindow.height / 5<tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y;//
		
		if (x>0 && y>0)
		{
			test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x < tracktarRects[j].lastTrackWindow.width / 2;//物体帧与帧之间移动距离
			test61 = -tracktarRects[j].lastTrackWindow.width / 5<tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x;//物体帧与帧之间移动距离
			test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y < tracktarRects[j].lastTrackWindow.height / 2;//
			test71 = -tracktarRects[j].lastTrackWindow.height / 5<tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y;//
		}
		else if (x>0 && y<0)
		{
			test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x < tracktarRects[j].lastTrackWindow.width / 2;//物体帧与帧之间移动距离
			test61 = -tracktarRects[j].lastTrackWindow.width / 5<tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x;//物体帧与帧之间移动距离
			test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y > -tracktarRects[j].lastTrackWindow.height / 2;//
			test71 = tracktarRects[j].lastTrackWindow.height / 5>tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y;//
		}
		else if (x < 0 && y > 0)
		{
			test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x > -tracktarRects[j].lastTrackWindow.width / 2;//物体帧与帧之间移动距离
			test61 = tracktarRects[j].lastTrackWindow.width / 5>tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x;//物体帧与帧之间移动距离
			test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y < tracktarRects[j].lastTrackWindow.height / 2;//
			test71 = -tracktarRects[j].lastTrackWindow.height / 5<tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y;//
		}
		else 
		{
			test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x > -tracktarRects[j].lastTrackWindow.width / 2;//物体帧与帧之间移动距离
			test61 = tracktarRects[j].lastTrackWindow.width / 5 > tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x;//物体帧与帧之间移动距离
			test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y > -tracktarRects[j].lastTrackWindow.height / 2;//
			test71 = tracktarRects[j].lastTrackWindow.height / 5  > tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y;//
		}
			
		
		
		bool test8 = abs(tracktarRects[j].camshiftPos.boundingRect().width - tracktarRects[j].lastTrackWindow.width*1.2) < MAX(tracktarRects[j].camshiftPos.boundingRect().width, tracktarRects[j].lastTrackWindow.width*1.2) * 3 / 4;//物体大小变化
		bool test9 = abs(tracktarRects[j].camshiftPos.boundingRect().height - tracktarRects[j].lastTrackWindow.height*1.2)<MAX(tracktarRects[j].camshiftPos.boundingRect().height, tracktarRects[j].lastTrackWindow.height*1.2) * 3 / 4;

		//bool test6 = tracktarRects[j].camshiftPos.center.x - tracktarRects[j].center.x < max(tracktarRects[j].lastTrackWindow.width, tracktarRects[j].lastTrackWindow.height) * 2;//物体帧与帧之间移动距离
		//bool test7 = tracktarRects[j].camshiftPos.center.y - tracktarRects[j].center.y < max(tracktarRects[j].lastTrackWindow.width, tracktarRects[j].lastTrackWindow.height) * 2;//
		//bool test8 = abs(tracktarRects[j].camshiftPos.boundingRect().width - tracktarRects[j].lastTrackWindow.width*1.2) < MAX(tracktarRects[j].camshiftPos.boundingRect().width, tracktarRects[j].lastTrackWindow.width*1.2) * 3 / 4;//物体大小变化
		//bool test9 = abs(tracktarRects[j].camshiftPos.boundingRect().height - tracktarRects[j].lastTrackWindow.height*1.2)<MAX(tracktarRects[j].camshiftPos.boundingRect().height, tracktarRects[j].lastTrackWindow.height*1.2) * 3 / 4;
		if (test1&&test2&&test3&&test4&&test5&&test6&&test7&&test8&&test9&&test71&&test61)
		{
			tracktarRects[j].isTrack = true;//同时满足，跟踪成功
		}
		else
		{
			tracktarRects[j].isTrack = false;
		}
		if (tracktarRects[j].predictNum>9 && abs(tracktarRects[j].center.x - tracktarRects[j].predictPt.x) < MAX(tracktarRects[j].lastTrackWindow.width, tracktarRects[j].lastTrackWindow.height) * 2 && abs(tracktarRects[j].center.y - tracktarRects[j].predictPt.y) < MAX(tracktarRects[j].lastTrackWindow.width, tracktarRects[j].lastTrackWindow.height) * 2)//预测次数&移动距离
		{
			tracktarRects[j].isPre = true;//满足上述条件，预测成功

		}
		else
		{
			tracktarRects[j].isPre = false;
		}
	}
	vector<cv::Rect>::iterator vIter;
	if (nums > 0)
	{

		//========================剔除掉临近边界点与面积小目标================================
		for (vIter = tarRects.begin(); vIter != tarRects.end();)
		{
			
#ifdef debug
			if ((*vIter).x<inputImg.cols / 15 || (*vIter).x + (*vIter).width>inputImg.cols * 14 / 15 || (*vIter).y<inputImg.rows / 15 || (*vIter).y + (*vIter).height>inputImg.rows * 14 / 15 || (*vIter).area() < 800)
#else
			if ((*vIter).x<inputImg.cols / 15 || (*vIter).x + (*vIter).width>inputImg.cols * 14 / 15 || (*vIter).y<inputImg.rows / 15 || (*vIter).y + (*vIter).height>inputImg.rows * 14 / 15 || (*vIter).area() < 800)
#endif // debug

			{
				if (ListLen != 0)
				{
					/*if (m == 322)
					{
						waitKey(0);
					}*/
					bool judge = false;
					for (int i = 0; i < ListLen; i++)
					{
						bool t1 = (*vIter).contains(tracktarRects[i].center);
						int w = min((*vIter).x + (*vIter).width, tracktarRects[i].lastTrackWindow.x + tracktarRects[i].lastTrackWindow.width)
							- max((*vIter).x, tracktarRects[i].lastTrackWindow.x);
						int h = min((*vIter).y + (*vIter).height, tracktarRects[i].lastTrackWindow.y + tracktarRects[i].lastTrackWindow.height)
							- max((*vIter).y, tracktarRects[i].lastTrackWindow.y);
						bool t6 = (w*h)>(tracktarRects[i].lastTrackWindow.width*tracktarRects[i].lastTrackWindow.height / 2);
						/*bool t2 = abs(int(((*vIter).width - tracktarRects[i].lastTrackWindow.width))) < 10;
						bool t3 = abs(int((*vIter).height - tracktarRects[i].lastTrackWindow.height))< 10;*/
						bool t4 = (*vIter).x + (*vIter).width<inputImg.cols * 2 / 3;
						bool t5 = (*vIter).y + (*vIter).height<inputImg.rows * 2 / 3;
						if (t1 && t6 && t4 && t5)
						{
							++vIter;
							judge = true;
							break;
						}
						
					}
					if (!judge)
					{
						vIter = tarRects.erase(vIter);//如果所框的矩形框不满足长宽条件，则去除
						nums--;
					}
					
				}
				else
				{
					vIter = tarRects.erase(vIter);//如果所框的矩形框不满足长宽条件，则去除
					nums--;
				}
				
			}
			else
			{
				++vIter;
			}
		}




		//========剔除大部分分裂、组合目标（每一帧新目标的分裂、组合或者目标跟踪预测失败下的分裂、组合不行）================
		for (int i = 0; i<ListLen; i++)//上一帧目标
		{
			if (nums>0)//当前帧目标
			{
				cv::Rect tempRect;
				if (tracktarRects[i].isTrack)//该目标被跟踪到
				{
					int x = (int)(tracktarRects[i].camshiftPos.center.x - tracktarRects[i].lastTrackWindow.width / 2);
					int y = (int)(tracktarRects[i].camshiftPos.center.y - tracktarRects[i].lastTrackWindow.height / 2);
					int width = tracktarRects[i].lastTrackWindow.width;
					int height = tracktarRects[i].lastTrackWindow.height;
					tempRect = cv::Rect(x, y, width, height);//上一帧跟踪当前帧位置
					for (vIter = tarRects.begin(); vIter != tarRects.end();)
					{
						if ((*vIter).x + (*vIter).width / 2 > x && (*vIter).x + (*vIter).width / 2 < x + width && (*vIter).y + (*vIter).height / 2 > y && (*vIter).y + (*vIter).height / 2 < y + height && (*vIter).area()<tempRect.area() / 2)
						{//当前目标中心点在跟踪到的目标大小内&面积小于跟踪目标        isUpdate=true 更新结构体信息（噪声）
							//分裂
							vIter = tarRects.erase(vIter);
							nums--;
						}
						else if ((int)tracktarRects[i].camshiftPos.center.x>(*vIter).x && (int)tracktarRects[i].camshiftPos.center.x<(*vIter).x + (*vIter).width && (int)tracktarRects[i].camshiftPos.center.y>(*vIter).y && (int)tracktarRects[i].camshiftPos.center.y<(*vIter).y + (*vIter).height && (*vIter).area()>tempRect.area() * 2)
						{
							//合并
							vIter = tarRects.erase(vIter);
							nums--;

						}
						else
						{
							++vIter;
						}

					}
				}
				else if (tracktarRects[i].isPre)//该目标被预测到
				{
					int x = tracktarRects[i].predictPt.x - tracktarRects[i].lastTrackWindow.width / 2;
					int y = tracktarRects[i].predictPt.y - tracktarRects[i].lastTrackWindow.height / 2;
					int width = tracktarRects[i].lastTrackWindow.width;
					int height = tracktarRects[i].lastTrackWindow.height;
					tempRect = cv::Rect(x, y, width, height);
					for (vIter = tarRects.begin(); vIter != tarRects.end();)
					{
						if ((*vIter).x + (*vIter).width / 2 > x && (*vIter).x + (*vIter).width / 2 < x + width && (*vIter).y + (*vIter).height / 2 > y && (*vIter).y + (*vIter).height / 2 < y + height && (*vIter).area()<tempRect.area() / 2)
						{
							//分裂
							vIter = tarRects.erase(vIter);
							nums--;
						}
						else if (tracktarRects[i].predictPt.x>(*vIter).x&&tracktarRects[i].predictPt.x<(*vIter).x + (*vIter).width&&tracktarRects[i].predictPt.y>(*vIter).y&&tracktarRects[i].predictPt.y<(*vIter).y + (*vIter).height && (*vIter).area()>tempRect.area() * 2)
						{
							//合并
							vIter = tarRects.erase(vIter);
							nums--;

						}
						else
						{
							++vIter;
						}

					}
				}
			}
			else
			{
				break;
			}
		}




		//===========================匹配================================
		for (int i = 0; i < nums; i++)//当前帧的目标
		{
			bool isMatched = false;
			cv::Point curCenter(tarRects[i].x + tarRects[i].width / 2, tarRects[i].y + tarRects[i].height / 2);//物体中心点
			cv::Mat imgTemp = inputImg(tarRects[i]);//物体图像
			calcHist(&imgTemp, 1, channels, cv::Mat(), RoiHist1, 3, histsize, ranges);//计算图像直方图RoiHist1
			normalize(RoiHist1, RoiHist1);//归一化
			for (int j = 0; j < ListLen; j++)//上一帧
			{
				bool isout = IntelligentTraffic::isPtInQuadrangle(areas[tracktarRects[j].areaid].area, Point(tarRects[i].x + tarRects[i].width/2 , tarRects[i].y + tarRects[i].height/2));
				if (!isout)
				{
					continue;
				}
				//匹配规则，更新List
				if (!tracktarRects[j].isUpdate)//上一帧与当前帧还未匹配到的目标进行匹配
				{

					if (tracktarRects[j].isTrack)//上一帧目标被跟踪到
					{
#ifdef debug
						//if (tarRects[i].contains(tracktarRects[j].camshiftPos.center))
						if (abs(tracktarRects[j].camshiftPos.center.x - curCenter.x) < tarRects[i].width / 2 && abs(tracktarRects[j].camshiftPos.center.y - curCenter.y) < tarRects[i].height / 2 && abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area()) < max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 4 / 5 )
#else
						if (abs(tracktarRects[j].camshiftPos.center.x - curCenter.x) < tarRects[i].width/2 && abs(tracktarRects[j].camshiftPos.center.y - curCenter.y) < tarRects[i].height/2 && abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area())<max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 3 / 5)
#endif // debug

						{
							double distance = compareHist(RoiHist1, tracktarRects[j].NDHist, CV_COMP_CORREL);//将当前位置与上一帧图片对比
							if (distance > 0.8)//越大越相似，设置参数！！！！
							{
								//匹配成功，更新数据tracktarRects
								isMatched = true;
								tracktarRects[j].center = curCenter;
								tracktarRects[j].NDHist.release();
								tracktarRects[j].NDHist = RoiHist1.clone();
								tracktarRects[j].imgRect.release();
								tracktarRects[j].imgRect = inputImg(tarRects[i]).clone();
								tracktarRects[j].lastTrackWindow = tarRects[i];//目标外接矩形
								tracktarRects[j].isUpdate = true;
								map<int, targetMessage>::iterator iter = trackRoad.find(tracktarRects[j].id);//
								if (iter != trackRoad.end()) //should like this  
								{
									iter->second.trackRoad.push_back(curCenter);
									iter->second.rect = tracktarRects[j].lastTrackWindow;//更新trackRoad
									iter->second.updata = true;
									iter->second.MatchType = 1;
								}
								cout << "track success" << tracktarRects[j].id << "CamshiftPos:[" << curCenter.x << "," << curCenter.y << "]" << endl;
							}
						}
					}
					//else if (tracktarRects[j].isPre)//如果预测到

					if ((tracktarRects[j].isTrack && !tracktarRects[j].isUpdate && tracktarRects[j].isPre) || !tracktarRects[j].isTrack &&tracktarRects[j].isPre)
					{

#ifdef debug
						//if (tarRects[i].contains(tracktarRects[j].predictPt))

						if (abs(tracktarRects[j].predictPt.x - curCenter.x) < tarRects[i].width/2&&abs(tracktarRects[j].predictPt.y - curCenter.y) < tarRects[i].height/2&&abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area()) < max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 4 / 5 )
#else
						if (abs(tracktarRects[j].predictPt.x - curCenter.x) < tarRects[i].width/2&&abs(tracktarRects[j].predictPt.y - curCenter.y) < tarRects[i].height/2&&abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area()) < max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 3 / 5)
#endif // debug

						{
							double distance = compareHist(RoiHist1, tracktarRects[j].NDHist, CV_COMP_CORREL);//将当前位置与上一帧图片对比
							if (distance>0.8)
							{
								//更新
								isMatched = true;
								tracktarRects[j].center = curCenter;
								tracktarRects[j].NDHist.release();
								tracktarRects[j].NDHist = RoiHist1.clone();
								tracktarRects[j].imgRect.release();
								tracktarRects[j].imgRect = inputImg(tarRects[i]).clone();
								tracktarRects[j].lastTrackWindow = tarRects[i];
								tracktarRects[j].isUpdate = true;
								map<int, targetMessage>::iterator iter = trackRoad.find(tracktarRects[j].id);
								if (iter != trackRoad.end()) //should like this  
								{
									iter->second.trackRoad.push_back(curCenter);
									iter->second.rect = tracktarRects[j].lastTrackWindow;
									iter->second.updata = true;
									iter->second.MatchType = 2;
								}
								cout << "track success" << tracktarRects[j].id << "PredictPos:[" << curCenter.x << "," << curCenter.y << "]" << endl;
							}
						}
					}
					//else
					if (((tracktarRects[j].isTrack || tracktarRects[j].isPre)&& !tracktarRects[j].isUpdate) || (!tracktarRects[j].isTrack&&!tracktarRects[j].isPre))
					{
#ifdef debug
						if (abs(tracktarRects[j].camshiftPos.center.x - curCenter.x) < tarRects[i].width * 2 / 3 && abs(tracktarRects[j].camshiftPos.center.y - curCenter.y) < tarRects[i].height * 2 / 3 && abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area())<max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 4 / 5)
#else
						if (abs(tracktarRects[j].camshiftPos.center.x - curCenter.x) < tarRects[i].width*2/3&&abs(tracktarRects[j].camshiftPos.center.y - curCenter.y) < tarRects[i].heigh*2/3t&&abs(tracktarRects[j].lastTrackWindow.area() - tarRects[i].area())<max(tracktarRects[j].lastTrackWindow.area(), tarRects[i].area()) * 3 / 5)
#endif // debug
						{
							double distance = compareHist(RoiHist1, tracktarRects[j].NDHist, CV_COMP_CORREL);////将当前位置与上一帧图片对比
							if (distance>0.8)
							{
								//更新
								isMatched = true;
								tracktarRects[j].center = curCenter;
								tracktarRects[j].NDHist.release();
								tracktarRects[j].NDHist = RoiHist1.clone();
								tracktarRects[j].imgRect.release();
								tracktarRects[j].imgRect = inputImg(tarRects[i]).clone();
								tracktarRects[j].lastTrackWindow = tarRects[i];
								tracktarRects[j].isUpdate = true;
								map<int, targetMessage>::iterator iter = trackRoad.find(tracktarRects[j].id);
								if (iter != trackRoad.end()) //should like this  
								{
									iter->second.trackRoad.push_back(curCenter);
									iter->second.rect = tracktarRects[j].lastTrackWindow;
									iter->second.updata = true;
									iter->second.MatchType = 3;
								}
								cout << "track success" << tracktarRects[j].id << "CurPos:[" << curCenter.x << "," << curCenter.y << "]" << endl;
							}
						}
					}
				}
				if (isMatched)//匹配成功，记录匹配类型
				{
					tracktarRects[j].matchNum = 1;//1：成功与上一帧进行匹配
					break;
				}
			}
			if (!isMatched)//新目标
			{
				//bool iscar = SVM_HOG(inputImg, tarRects[i]);
				bool iscar = 1;
#ifdef debug
				if (curCenter.x>inputImg.cols * 2 / 3 || curCenter.y>inputImg.rows * 2 / 3 || !iscar || tarRects[i].width>inputImg.cols * 2 / 3 || tarRects[i].height>inputImg.rows * 2 / 3 || tarRects[i].area()<1000)
					continue;
				else
#endif // debug
				{
					targetMessage temptarMessage;//临时变量
					temptarMessage.rect = tarRects[i];//记录外接矩形
					temptarMessage.classNum = 0;//初始化分类类别
					temptarMessage.carsize = -1;//初始化大小
					TrackRect tempRect;

					tempRect.carSize = -1;
					tempRect.matchNum = 2;//2：新目标
					tempRect.predictNum = 1;//预测次数
					tempRect.id = ID;
					tempRect.center = curCenter;//中心位置
					cout << "newTar" << ID << "curPos:[" << curCenter.x << "," << curCenter.y << "]" << endl;
					tempRect.lastTrackWindow = tarRects[i];//目标截图矩形
					tempRect.NDHist = RoiHist1.clone();//RGB直方图
					tempRect.imgRect = inputImg(tarRects[i]).clone();//目标截图
					tempRect.isUpdate = true;//？？？
					tempRect.leaveCount = 0;//消失了的帧数，目前没用
					for (int i = 0; i < 4; i++)
					{
						if (areas[i].use)
						{
							tempRect.areaid = i;							
						}
					}
					tracktarRects.push_back(tempRect);
					temptarMessage.trackRoad.push_back(curCenter);//轨迹
					trackRoad.insert(map<int, targetMessage>::value_type(ID, temptarMessage));
					map<int, targetMessage>::iterator iter = trackRoad.end();
					iter--;
					iter->second.updata = true;
					ListLen++;//目标列表++
					ID++;
					cout << "curID:" << ID-1 << endl;
					if (ID == 46)
					{
						waitKey(10);
					}
					if (ID == 1000)
						ID = 0;
				}
				
			}
		}
		//=======================上一帧跟当前帧没有匹配的目标========================
		if (tracktarRects.size() > 0)
		{
			vector<TrackRect>::iterator vtrIter;
			for (vtrIter = tracktarRects.begin(); vtrIter != tracktarRects.end();)//上一帧目标
			{
				if (!((*vtrIter).isUpdate))///是否是未匹配的上一帧目标
				{
					double distance = 0.0;
					int x = 0, y = 0, width = 0, height = 0;
					cv::Rect tempRect = cv::Rect(0, 0, 0, 0);
					cv::Rect ret(0, 0, 1, 1);
					if ((*vtrIter).isTrack)
					{
						x = (int)((*vtrIter).camshiftPos.center.x - (*vtrIter).lastTrackWindow.width / 2) < 0 ? 0 : (int)((*vtrIter).camshiftPos.center.x - (*vtrIter).lastTrackWindow.width / 2);
						y = (int)((*vtrIter).camshiftPos.center.y - (*vtrIter).lastTrackWindow.height / 2) < 0 ? 0 : (int)((*vtrIter).camshiftPos.center.y - (*vtrIter).lastTrackWindow.height / 2);
						width = x + (*vtrIter).lastTrackWindow.width < inputImg.cols ? (*vtrIter).lastTrackWindow.width : inputImg.cols - x;
						height = y + (*vtrIter).lastTrackWindow.height < inputImg.rows ? (*vtrIter).lastTrackWindow.height : inputImg.rows - y;
						if (x >= 0 && y >= 0 && width + x <= inputImg.cols&&height + y <= inputImg.rows)
						{
							tempRect = cv::Rect(x, y, width, height);
							cv::Mat imgTemp = inputImg(tempRect);
							calcHist(&imgTemp, 1, channels, cv::Mat(), RoiHist1, 3, histsize, ranges);//
							normalize(RoiHist1, RoiHist1);
							distance = compareHist(RoiHist1, (*vtrIter).NDHist, CV_COMP_CORREL);//将camshift预测位置与当前位置对比
						}
					}
					else if ((*vtrIter).isPre)//预测
					{
						x = (int)((*vtrIter).predictPt.x - (*vtrIter).lastTrackWindow.width / 2) < 0 ? 0 : (int)((*vtrIter).predictPt.x - (*vtrIter).lastTrackWindow.width / 2);
						y = (int)((*vtrIter).predictPt.y - (*vtrIter).lastTrackWindow.height / 2) < 0 ? 0 : (int)((*vtrIter).predictPt.y - (*vtrIter).lastTrackWindow.height / 2);
						width = x + (*vtrIter).lastTrackWindow.width < inputImg.cols ? (*vtrIter).lastTrackWindow.width : inputImg.cols - x;
						height = y + (*vtrIter).lastTrackWindow.height < inputImg.rows ? (*vtrIter).lastTrackWindow.height : inputImg.rows - y;
						if (x >= 0 && y >= 0 && width + x <= inputImg.cols&&height + y <= inputImg.rows)
						{
							tempRect = cv::Rect(x, y, width, height);
							cv::Mat imgTemp = inputImg(tempRect);
							calcHist(&imgTemp, 1, channels, cv::Mat(), RoiHist1, 3, histsize, ranges);//
							normalize(RoiHist1, RoiHist1);
							distance = compareHist(RoiHist1, (*vtrIter).NDHist, CV_COMP_CORREL);//将predict预测位置与当前位置对比
						}
					}
					else
					{
						cv::Mat tmpGray((*vtrIter).imgRect.rows, (*vtrIter).imgRect.cols, CV_8UC1);
						cvtColor((*vtrIter).imgRect, tmpGray, CV_BGR2GRAY);//将上一帧中vtrIter目标截图灰度化
						//x = (*vtrIter).center.x - (*vtrIter).lastTrackWindow.width * 3 / 4 < 0 ? 0 : (*vtrIter).center.x - (*vtrIter).lastTrackWindow.width * 3 / 4;//上上一帧左上角
						//width = 2 * (*vtrIter).lastTrackWindow.width + x < inputImg.cols ? 2 * (*vtrIter).lastTrackWindow.width : inputImg.cols - x;
						//y = (*vtrIter).center.y - (*vtrIter).lastTrackWindow.height * 3 / 4 < 0 ? 0 : (*vtrIter).center.y - (*vtrIter).lastTrackWindow.height * 3 / 4;
						//height = 2 * (*vtrIter).lastTrackWindow.height + y < inputImg.rows ? 2 * (*vtrIter).lastTrackWindow.height : inputImg.rows - y;
						x = (*vtrIter).center.x - (*vtrIter).lastTrackWindow.width * 5 / 2 < 0 ? 0 : (*vtrIter).center.x - (*vtrIter).lastTrackWindow.width * 5 / 2;//上上一帧左上角
						width = 5 * (*vtrIter).lastTrackWindow.width + x < inputImg.cols ? 5 * (*vtrIter).lastTrackWindow.width : inputImg.cols - x;
						y = (*vtrIter).center.y - (*vtrIter).lastTrackWindow.height * 5 / 2 < 0 ? 0 : (*vtrIter).center.y - (*vtrIter).lastTrackWindow.height * 5 / 2;
						height = 5 * (*vtrIter).lastTrackWindow.height + y < inputImg.rows ? 5 * (*vtrIter).lastTrackWindow.height : inputImg.rows - y;
						cv::Rect matchInRect(x, y, width, height);//目标上上一帧外接矩形
						cv::Mat matchInImg = img_Gray(matchInRect);//目标上上一帧截图
						matchTemp(matchInImg, tmpGray, ret);//目标上上一帧截图&上一帧截图进行模板匹配，ret为在matchInImg最匹配的矩形区域
						ret.x += x;
						ret.y += y;
						x = ret.x;
						y = ret.y;
						width = ret.width;
						height = ret.height;
						tempRect = ret;

						cv::Mat imgTemp = inputImg(ret);
						calcHist(&imgTemp, 1, channels, cv::Mat(), RoiHist1, 3, histsize, ranges);
						normalize(RoiHist1, RoiHist1);
						distance = compareHist(RoiHist1, (*vtrIter).NDHist, CV_COMP_CORREL);

					}
					bool isEdge = false;
					bool testedge;
					int direction_x = areas[(*vtrIter).areaid].direction[1].x - areas[(*vtrIter).areaid].direction[0].x;
					int direction_y = areas[(*vtrIter).areaid].direction[1].y - areas[(*vtrIter).areaid].direction[0].y;

					if (direction_x > 0 && direction_y > 0)
					{
						testedge = tempRect.x + tempRect.width > inputImg.cols * 14 / 15 || tempRect.y + tempRect.height > inputImg.rows * 14 / 15;
					}
					else if (direction_x < 0 && direction_y < 0)
					{
						testedge = tempRect.x < inputImg.cols / 15 || tempRect.y < inputImg.rows / 15;
					}
					else if (direction_x > 0 && direction_y < 0)
					{
						testedge = tempRect.x + tempRect.width > inputImg.cols * 14 / 15 || tempRect.y < inputImg.rows / 15;
					}
					else
					{
						testedge = tempRect.y + tempRect.height > inputImg.rows * 14 / 15 || tempRect.x < inputImg.cols / 15;
					}
					if (testedge)
					{
						isEdge = true;
					}
					//cv::Point center(tempRect.x + tempRect.width /2, tempRect.y + tempRect.height/2 );
					//bool isout = isPtInQuadrangle(areas[(*vtrIter).areaid].area, center);
					//if (!isout)
					//{
					//	isEdge = true;
					//}
					bool isSamllTar = false;
					if (tempRect.area() < 800)//小目标参数 ！！！
					{
						isSamllTar = true;
					}
					if (distance < 0.6||isEdge || isSamllTar )//目标消失
					{
						//删除保留信息，判断过线
						bool isvtrIter = true;//匹配到的
						map<int, targetMessage>::iterator iter1 = trackRoad.find((*vtrIter).id);
						if (iter1 != trackRoad.end()) //should like this  
						{
							if (iter1->second.trackRoad.size()>0)
							{
								map<int, targetMessage>::iterator iter = trackRoad.begin();
								while (iter != trackRoad.end()) //#1
								{
									if (iter->first == (*vtrIter).id)
									{
										trackRoad.erase(iter++);
										break;
									}
									iter++;
								}
								//删除vector
								cout << (*vtrIter).id <<"消失"<< endl;
								vtrIter = tracktarRects.erase(vtrIter);
								ListLen--;
								isvtrIter = false;
							}
						}
						if (isvtrIter)
						{
							vtrIter++;
						}
					}
					else//匹配到
					{
						cv::Point curCenter(x + width / 2, y + height / 2);
						(*vtrIter).matchNum = 3;//先未匹配成功，去伪新目标中匹配成功
						(*vtrIter).center = curCenter;
						(*vtrIter).lastTrackWindow = tempRect;
						(*vtrIter).isUpdate = true;
						map<int, targetMessage>::iterator iter = trackRoad.find((*vtrIter).id);
						if (iter != trackRoad.end()) //should like this  
						{
							iter->second.trackRoad.push_back(curCenter);
							iter->second.rect = tempRect;
							iter->second.updata = true;
							iter->second.MatchType = 4;
							cout << "track success" << (*vtrIter).id << "AddPos:[" << curCenter.x << "," << curCenter.y << "]" << endl;
						}
						vtrIter++;
					}
				}
				else
				{
					vtrIter++;
				}
			}
		}
		//===========================除去伪新目标================================
		if (tracktarRects.size() > 0)
		{
			vector<TrackRect>::iterator vtrIter;
			for (vtrIter = tracktarRects.begin(); vtrIter != tracktarRects.end();)
			{
				bool isDelete = false;
				if ((*vtrIter).matchNum == 2)//可能不是新目标,
				{
					vector<TrackRect>::iterator vtrIter1;
					for (vtrIter1 = tracktarRects.begin(); vtrIter1 != tracktarRects.end();)
					{
						if ((*vtrIter1).matchNum == 3)//跟踪的旧目标
						{
							bool test1 = (*vtrIter).center.x > (*vtrIter1).lastTrackWindow.x && (*vtrIter).center.x<(*vtrIter1).lastTrackWindow.x + (*vtrIter1).lastTrackWindow.width && (*vtrIter).center.y>(*vtrIter1).lastTrackWindow.y && (*vtrIter).center.y<(*vtrIter1).lastTrackWindow.y + (*vtrIter1).lastTrackWindow.height;
							bool test2 = (*vtrIter1).center.x>(*vtrIter).lastTrackWindow.x && (*vtrIter1).center.x<(*vtrIter).lastTrackWindow.x + (*vtrIter).lastTrackWindow.width && (*vtrIter1).center.y>(*vtrIter).lastTrackWindow.y && (*vtrIter1).center.y<(*vtrIter).lastTrackWindow.y + (*vtrIter).lastTrackWindow.height;
							if (test1 || test2)
							{
								//删除保留信息，判断过线
								map<int, targetMessage>::iterator iter1 = trackRoad.find((*vtrIter).id);
								if (iter1 != trackRoad.end()) //should like this  
								{
									if (iter1->second.trackRoad.size()>0)
									{
										map<int, targetMessage>::iterator iter = trackRoad.begin();
										while (iter != trackRoad.end()) //#1
										{
											if (iter->first == (*vtrIter).id)
											{
												trackRoad.erase(iter++);
												break;
											}
											iter++;
										}
										//删除vector
										vtrIter = tracktarRects.erase(vtrIter);
										ListLen--; ID--;
										isDelete = true;
									}
								}
								break;
							}
						}
						vtrIter1++;
					}
				}
				if (!isDelete)
				{
					vtrIter++;///
				}
			}
		}
		//============如果两个目标比较近，则保留前一个目标（大车分裂情况）（堵车情况）=======================
		if (tracktarRects.size() > 1)//除去同一目标中后面的目标
		{
			vector<TrackRect>::iterator vtrIter;
			for (vtrIter = tracktarRects.begin(); vtrIter != tracktarRects.end(); vtrIter++)
			{
				vector<TrackRect>::iterator vtrIter1;
				for (vtrIter1 = vtrIter + 1; vtrIter1 != tracktarRects.end();)
				{
					bool test1 = (*vtrIter).center.x>(*vtrIter1).lastTrackWindow.x && (*vtrIter).center.x<(*vtrIter1).lastTrackWindow.x + (*vtrIter1).lastTrackWindow.width && (*vtrIter).center.y>(*vtrIter1).lastTrackWindow.y && (*vtrIter).center.y<(*vtrIter1).lastTrackWindow.y + (*vtrIter1).lastTrackWindow.height && (*vtrIter).predictNum<9;
					bool test2 = (*vtrIter1).center.x>(*vtrIter).lastTrackWindow.x && (*vtrIter1).center.x<(*vtrIter).lastTrackWindow.x + (*vtrIter).lastTrackWindow.width && (*vtrIter1).center.y>(*vtrIter).lastTrackWindow.y && (*vtrIter1).center.y<(*vtrIter).lastTrackWindow.y + (*vtrIter).lastTrackWindow.height && (*vtrIter1).predictNum<9;
					if (test1 || test2)
					{
						//删除保留信息，判断过线
						map<int, targetMessage>::iterator iter1 = trackRoad.find((*vtrIter1).id);
						if (iter1 != trackRoad.end()) //should like this  
						{
							if (iter1->second.trackRoad.size()>0)
							{
								trackRoad.erase(iter1++);
							}
						}
						vtrIter1 = tracktarRects.erase(vtrIter1);
						ListLen--;
						ID--;
					}
					else
						vtrIter1++;
				}
			}
		}
		
		//========================更新字典==============================
		for (int i = 0; i < ListLen; i++)
		{
			tracktarRects[i].isUpdate = false;
			//做预测
			if (tracktarRects[i].predictNum == 1)//初始化，预测
			{
				tracktarRects[i].KF.init(4, 2, 0);
				tracktarRects[i].KF.transitionMatrix = (cv::Mat_<float>(4, 4) <<
					1, 0, 1, 0,
					0, 1, 0, 1, 
					0, 0, 1, 0,
					0, 0, 0, 1);//元素导入矩阵，按行;  

				//setIdentity: 缩放的单位对角矩阵;  
				//!< measurement matrix (H) 观测模型  
				setIdentity(tracktarRects[i].KF.measurementMatrix);

				//!< process noise covariance matrix (Q)  
				// wk 是过程噪声，并假定其符合均值为零，协方差矩阵为Qk(Q)的多元正态分布;  
				setIdentity(tracktarRects[i].KF.processNoiseCov, cv::Scalar::all(1e-5));

				//!< measurement noise covariance matrix (R)  
				//vk 是观测噪声，其均值为零，协方差矩阵为Rk,且服从正态分布;  
				setIdentity(tracktarRects[i].KF.measurementNoiseCov, cv::Scalar::all(1e-1));

				//!< priori error estimate covariance matrix (P'(k)): P'(k)=A*P(k-1)*At + Q)*/  A代表F: transitionMatrix  
				//预测估计协方差矩阵;  
				setIdentity(tracktarRects[i].KF.errorCovPost, cv::Scalar::all(1));
				//!< corrected state (x(k)): x(k)=x'(k)+K(k)*(z(k)-H*x'(k))  
				//initialize post state of kalman filter at random   
				randn(tracktarRects[i].KF.statePost, cv::Scalar::all(0), cv::Scalar::all(0.1));
				cv::Mat prediction = tracktarRects[i].KF.predict();
				//KALMAN 最优化估计
				cv::Mat measurement = cv::Mat::zeros(2, 1, CV_32F);
				measurement.at<float>(0) = (float)tracktarRects[i].center.x;
				measurement.at<float>(1) = (float)tracktarRects[i].center.y;//上一帧估计值 当前帧测量值 预测当前帧估计值
				tracktarRects[i].KF.correct(measurement);
				//预测
				prediction = tracktarRects[i].KF.predict();
				cv::Point predictPt = cv::Point((int)prediction.at<float>(0), (int)prediction.at<float>(1));//卡尔曼预测
				tracktarRects[i].predictPt = predictPt;
				tracktarRects[i].predictNum = 2;
				//计算hhist
				cv::Mat roi(hue, tracktarRects[i].lastTrackWindow), maskroi(mask, tracktarRects[i].lastTrackWindow);
				calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
				normalize(hist, hist, 0, 255, CV_MINMAX);
				tracktarRects[i].hHist = hist.clone();
			}
			else//预测
			{
				//KALMAN 最优化估计
				cv::Mat measurement = cv::Mat::zeros(2, 1, CV_32F);
				measurement.at<float>(0) = (float)tracktarRects[i].center.x;
				measurement.at<float>(1) = (float)tracktarRects[i].center.y;
				tracktarRects[i].KF.correct(measurement);
				//预测
				cv::Mat prediction = tracktarRects[i].KF.predict();
				cv::Point predictPt = cv::Point((int)prediction.at<float>(0), (int)prediction.at<float>(1));
				tracktarRects[i].predictPt = predictPt;
				tracktarRects[i].predictNum++;//预测次数
				//计算hhist
				cv::Mat roi(hue, tracktarRects[i].lastTrackWindow), maskroi(mask, tracktarRects[i].lastTrackWindow);
				calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
				normalize(hist, hist, 0, 255, CV_MINMAX);
				if (tracktarRects[i].matchNum != 3)
				{
					tracktarRects[i].hHist.release();
					tracktarRects[i].hHist = hist.clone();
				}

			}
		}

	}
	
}

void IntelligentTraffic::tarclassInit()
{
	//char pathName[261];
	//GetModuleFileName(NULL, pathName,260);
	//string path(pathName);
	//int pos = path.find_last_of('\\');
	//string temppath=path.substr(0,pos);
	//temppath = temppath+"\\"+"HOG_SVM_CAR_LINEAR.xml";
	//svm=new CvSVM();
	//svm->load(temppath.c_str());
}
int IntelligentTraffic::CarCascade(cv::Mat img)
{
	//检测样本    
	//Mat test=Mat(28,28,CV_8UC1);  

	//if (test.empty())
	//{
	//	return -1;
	//}

	//resize(img, test, Size(28, 28));
	//HOGDescriptor *hog=new HOGDescriptor(cvSize(28,28),cvSize(14,14),cvSize(7,7),cvSize(7,7),9);      
	//vector<float>descriptors;//存放结果       
	//hog->compute(test, descriptors, Size(1, 1), Size(0, 0)); //Hog特征计算      
	//CvMat* SVMtrainMat=cvCreateMat(1,descriptors.size(),CV_32FC1);   
	//int n=0;    
	//for(vector<float>::iterator iter=descriptors.begin();iter!=descriptors.end();iter++)    
	//{    
	//	cvmSet(SVMtrainMat,0,n,*iter);    
	//	n++;    
	//}   
	//int ret = svm->predict(SVMtrainMat);//检测结果
	////cvReleaseImage(&test);
	//cvReleaseImage(&trainTempImg);
	return 0;
}
void IntelligentTraffic::tarClass(cv::Mat &inputImg, map<int, targetMessage> &trackroad)
{
	if (trackroad.size() != 0)
	{
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.classNum == 0)
			{
				cv::Mat wrImg = inputImg(iterBegin->second.rect).clone();
				int ret = CarCascade(wrImg);
				if (ret == 0)
				{
					iterBegin->second.classNum = 2;//车
				}
				else if (iterBegin->second.rect.height / (float)iterBegin->second.rect.width >= 2)
				{
					iterBegin->second.classNum = 1;//人
				}
				else
				{
					iterBegin->second.classNum = 3;
				}
			}
		}
	}
}

int IntelligentTraffic::stopIncidentDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyStop, string time[2], cv::Mat &img_Pic)
{
	vector<tarTrigger>().swap(outMessagebyStop);
	if (trackroad.size() != 0 && detectRegion.size() == 4)
	{
		tarTrigger tarTriggerTmp;
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size() > 10)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				viterEnd--;
				vector<cv::Point>::iterator viterPre = viterEnd - 10;
				cv::Point endToPre = *viterEnd - *viterPre;
				double isInside = pointPolygonTest(detectRegion, *viterEnd, false);
				if (isInside >= 0 && iterBegin->second.classNum == 2)//进入区域
				{
					if (abs(endToPre.x) <= 5 && abs(endToPre.y) <= 5)//停止阈值
					{
						tarTriggerTmp.id = iterBegin->first;
						tarTriggerTmp.pos = *viterEnd;
						outMessagebyStop.push_back(tarTriggerTmp);
						if (!iterBegin->second.isStop)
						{
							iterBegin->second.isStop = true;
							EventsResults eventsTmp;
							int id = iterBegin->first;
							eventsTmp.m_id = id;
							eventsTmp.m_events[0] = 1;
							cv::Point pt;
							pt.x = viterEnd->x; pt.y = viterEnd->y;
							eventsTmp.m_pos[0] = pt;
							eventsTmp.day[0] = time[0];
							eventsTmp.second[0] = time[1];
							eventsTmp.img_result = img_Pic;
							eventsResults.push_back(eventsTmp);
						}
						return 1;
					}
					else return 0;
				}
			}
		}
	}

}
int IntelligentTraffic::reverseDriveIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, Direction &direct, vector<tarTrigger>&outMessagebyReverse, string time[2], cv::Mat &img_Pic)//逆行
{
	vector<tarTrigger>().swap(outMessagebyReverse);
	if (trackroad.size() > 0 && detectRegion.size() == 4)
	{
		tarTrigger tarTriggerTmp;
		cv::Point direction[2];
		direction[0].x = direct.ptFirst.x;
		direction[0].y = direct.ptFirst.y;
		direction[1].x = direct.ptEnd.x;
		direction[1].y = direct.ptEnd.y;
		cv::Point direct = direction[1] - direction[0];
		double q = sqrt((double)direct.x*(double)direct.x + (double)direct.y*(double)direct.y);
		double x = direct.x / q;
		double y = direct.y / q;
		cv::Point2d directd;
		directd.x = x;
		directd.y = y;
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size() > 10)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				cv::Point roadEnd;
				roadEnd = (*(viterEnd - 1)) + (*(viterEnd - 2)) + (*(viterEnd - 3));
				*viterEnd--;
				vector<cv::Point>::iterator viterPre = viterEnd - 10;
				cv::Point roadPre;
				roadPre = (*(viterPre)) + (*(viterPre + 1 )) + (*(viterPre + 2));

				cv::Point endToPre = roadEnd - roadPre;
				double p = sqrt((double)endToPre.x*(double)endToPre.x + (double)endToPre.y*(double)endToPre.y);
				cv::Point2d endToPred(0, 0);
				if (p != 0)
				{
					endToPred.x = (double)endToPre.x / p;
					endToPred.y = (double)endToPre.y / p;
				}
				//double isInside = pointPolygonTest(detectRegion, *viterEnd, false);
				bool isInside = IntelligentTraffic::isPtInQuadrangle(detectRegion, *viterEnd);
				//if (isInside >= 0 && iterBegin->second.classNum == 2)//进入区域
				if (isInside && iterBegin->second.classNum == 2)//进入区域
				{
					if ((abs(endToPre.x) > 4 || abs(endToPre.y) > 4) && (-directd.x*endToPred.x - directd.y*endToPred.y) > 0.5)//逆行判断
					{
						tarTriggerTmp.id = iterBegin->first;
						tarTriggerTmp.pos = *viterEnd;
						outMessagebyReverse.push_back(tarTriggerTmp);

						if (!iterBegin->second.isReversedrive)
						{
							iterBegin->second.isReversedrive = true;

							EventsResults eventsTmp;
							int id = iterBegin->first;
							eventsTmp.m_id = id;
							eventsTmp.m_events[1] = 1;
							cv::Point pt;
							pt.x = viterEnd->x; pt.y = viterEnd->y;
							eventsTmp.m_pos[1] = pt;
							eventsTmp.day[1] = time[0];
							eventsTmp.second[1] = time[1];
							eventsTmp.img_result = img_Pic;
							eventsResults.push_back(eventsTmp);
						}
						return 2;
					}
					else return 0;
				}
			}
		}
	}

}
int IntelligentTraffic::driveOutOfBorderIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyDriveOut, string time[2], cv::Mat &img_Pic)//驶出
{
	vector<tarTrigger>().swap(outMessagebyDriveOut);
	if (trackroad.size() > 0 && detectRegion.size() == 4)
	{
		tarTrigger tarTriggerTmp;
		cv::Point edge[2];
		edge[0] = detectRegion[2] - detectRegion[0];
		edge[1] = detectRegion[3] - detectRegion[1];
		if (edge[0].y < 0)
		{
			edge[0] = -edge[0];
		}
		if (edge[1].y<0)
		{
			edge[1] = -edge[1];
		}
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size()>10)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				viterEnd--;
				vector<cv::Point>::iterator viterPre = viterEnd - 10;
				cv::Point vec[2];
				bool isInside = IntelligentTraffic::isPtInQuadrangle(detectRegion, *viterEnd);

				//double isInside = pointPolygonTest(detectRegion, *viterPre, false);
				if (edge[0].y < 0)
				{
					vec[0] = *viterEnd - detectRegion[2];
				}
				else
				{
					vec[0] = *viterEnd - detectRegion[0];
				}
				if (edge[1].y < 0)
				{
					vec[1] = *viterEnd - detectRegion[3];
				}
				else
				{
					vec[1] = *viterEnd - detectRegion[1];
				}
				//if (isInside >= 0 && iterBegin->second.classNum == 2)//上一帧处于区域
				if (!isInside && iterBegin->second.classNum == 2)//上一帧处于区域
				{
					if (edge[0].x*vec[0].y - edge[0].y*vec[0].x >= 0 || edge[1].x*vec[1].y - edge[1].y*vec[1].x <= 0)//edge[0]左侧，或edge[1]右侧
					{
						tarTriggerTmp.id = iterBegin->first;
						tarTriggerTmp.pos = *viterEnd;
						outMessagebyDriveOut.push_back(tarTriggerTmp);

						if (!iterBegin->second.isDriveOutOfBorder)
						{
							iterBegin->second.isDriveOutOfBorder = true;

							EventsResults eventsTmp;
							int id = iterBegin->first;
							eventsTmp.m_id = id;
							eventsTmp.m_events[2] = 1;
							cv::Point pt;
							pt.x = viterEnd->x; pt.y = viterEnd->y;
							eventsTmp.m_pos[2] = pt;
							eventsTmp.day[2] = time[0];
							eventsTmp.second[2] = time[1];
							eventsTmp.img_result = img_Pic;
							eventsResults.push_back(eventsTmp);

							map<int, targetMessage>::iterator iter1 = trackroad.find((*iterBegin).first);
							if (iter1 != trackroad.end()) //should like this  
							{
								if (iter1->second.trackRoad.size() > 0)
								{
									trackroad.erase(iter1++);
								}
							}

						}
						return 3;
					}
					else return 0;
				}
			}
		}
	}

}
int IntelligentTraffic::pedestrianEntryIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyPedestrian, string time[2], cv::Mat &img_Pic)//行人检测
{
	vector<tarTrigger>().swap(outMessagebyPedestrian);
	if (trackroad.size() > 0 && detectRegion.size() == 4)
	{
		tarTrigger tarTriggerTmp;
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size() > 10)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				viterEnd--;
				double isInside = pointPolygonTest(detectRegion, *viterEnd, false);
				if (isInside >= 0 && iterBegin->second.classNum == 1)//进入区域
				{
					tarTriggerTmp.id = iterBegin->first;
					tarTriggerTmp.pos = *viterEnd;
					outMessagebyPedestrian.push_back(tarTriggerTmp);

					if (!iterBegin->second.isPedestrianEntry)
					{
						iterBegin->second.isPedestrianEntry = true;

						EventsResults eventsTmp;
						int id = iterBegin->first;
						eventsTmp.m_id = id;
						eventsTmp.m_events[3] = 1;
						cv::Point pt;
						pt.x = viterEnd->x; pt.y = viterEnd->y;
						eventsTmp.m_pos[3] = pt;
						eventsTmp.day[3] = time[0];
						eventsTmp.second[3] = time[1];
						eventsTmp.img_result = img_Pic;
						eventsResults.push_back(eventsTmp);
					}
					return 4;
				}
				else return 0;
			}
		}
	}

}
int IntelligentTraffic::lossIncidentDetect(map<int, targetMessage> &trackroad, vector<cv::Point> detectRegion, vector<tarTrigger>&outMessagebyLoss, string time[2], cv::Mat &img_Pic)//从道路中出现 在之前应该判断  <800
{
	vector<tarTrigger>().swap(outMessagebyLoss);
	if (trackroad.size() > 0 && detectRegion.size() == 4)
	{
		tarTrigger tarTriggerTmp;
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size() > 5)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				viterEnd--;
				double isInside = pointPolygonTest(detectRegion, *viterEnd, false);
				if (isInside >= 0 && iterBegin->second.classNum == 3)//进入区域
				{
					tarTriggerTmp.id = iterBegin->first;
					tarTriggerTmp.pos = *viterEnd;
					outMessagebyLoss.push_back(tarTriggerTmp);

					if (!iterBegin->second.isLoss)
					{
						iterBegin->second.isLoss = true;

						EventsResults eventsTmp;
						int id = iterBegin->first;
						eventsTmp.m_id = id;
						eventsTmp.m_events[4] = 1;
						cv::Point pt;
						pt.x = viterEnd->x; pt.y = viterEnd->y;
						eventsTmp.m_pos[4] = pt;
						eventsTmp.day[4] = time[0];
						eventsTmp.second[4] = time[1];
						eventsTmp.img_result = img_Pic;
						eventsResults.push_back(eventsTmp);
					}
					return 5;
				}
				else return 0;
			}
		}
	}

}
bool IntelligentTraffic::jamIncidentDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, int &numTh, int &speed, string time[7], cv::Mat &img_Pic)//拥堵
{
	int tarNum = 0;
	if (trackroad.size() != 0 && detectRegion.size() == 4)
	{
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			if (iterBegin->second.trackRoad.size() > 10)
			{
				vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
				viterEnd--;
				double isInside = 1;// pointPolygonTest(detectRegion, *viterEnd, false);
				if (isInside >= 0 && iterBegin->second.classNum == 2)//进入区域
				{
					tarNum++;
				}
			}
		}
	}
	if (tarNum > numTh)
	{
		//numTh = tarNum;//？？？？？
		speed = 0;
		situationResults.img_result = img_Pic;
		return true;
	}
	else
		return false;
}

void IntelligentTraffic::carNumDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, set<int> &preID, int &carNum, string time[7])
{
	if (trackroad.size() != 0 && detectRegion.size() == 4)
	{
		map<int, targetMessage>::iterator iterBegin;
		set<int> curID;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
			viterEnd--;
			double isInside = 1;// = pointPolygonTest(detectRegion, *viterEnd, false);
			if (isInside >= 0 && iterBegin->second.classNum == 2)//进入区域
			{
				if (preID.size() > 0)
				{
					bool ismatch = false;
					if (preID.count(iterBegin->first))
					{
						ismatch = true;
						break;
					}
					if (!ismatch)
					{
						carNum++;
					}
				}
				else
				{
					carNum++;

				}
				curID.insert(iterBegin->first);
			}
		}
		set<int> ::const_iterator iter;
		for (iter = curID.begin(); iter != curID.end(); ++iter)
		{
			if (!preID.count(*iter))
			{
				preID.insert(*iter);
			}
		}
	}
}
void IntelligentTraffic::CarSizeDetect(map<int, targetMessage>&trackroad, vector<cv::Point> detectRegion, int areath1, int areath2, vector<cv::Point>&carSize)
{
	int tarNum = 0;
	if (trackroad.size() != 0 && detectRegion.size() == 4)
	{
		map<int, targetMessage>::iterator iterBegin;
		for (iterBegin = trackroad.begin(); iterBegin != trackroad.end(); iterBegin++)
		{
			vector<cv::Point>::iterator viterEnd = iterBegin->second.trackRoad.end();
			viterEnd--;
			double isInside = 1; //= pointPolygonTest(detectRegion, *viterEnd, false);
			if (isInside >= 0 && iterBegin->second.classNum == 2 && iterBegin->second.carsize == -1)//进入区域
			{
				cv::Point temp;
				temp.x = iterBegin->first;//ID
				if ((*iterBegin).second.rect.area() < areath1)
				{
					temp.y = 1;//分类结果
					iterBegin->second.carsize = 1;
					//cout << "carID:" << iterBegin->first << "  SMALL CAR" << endl;
				}
				else if ((*iterBegin).second.rect.area() < areath2)
				{
					temp.y = 2;
					iterBegin->second.carsize = 2;
					//cout << "carID:" << iterBegin->first << "  MEDIUM CAR" << endl;
				}
				else
				{
					temp.y = 3;
					iterBegin->second.carsize = 3;
					//cout << "carID:" << iterBegin->first << "  BIG CAR" << endl;
				}
				carSize.push_back(temp);
			}

		}
	}

}

void IntelligentTraffic::computeConvertFactor(vector<cv::Point2f> &picPt, vector<cv::Point2f> &realPt, cv::Mat &factor)
{
	if (picPt.size() == realPt.size() && factor.cols == 1 && factor.rows == 8)
	{
		int Abrows = 2 * picPt.size();
		int Acols = 8;
		int bcols = 1;
		cv::Mat A = cv::Mat::zeros(Abrows, Acols, CV_32FC1);
		cv::Mat b = cv::Mat::zeros(Abrows, bcols, CV_32FC1);
		for (int i = 0; i < 2 * picPt.size(); i++)
		{
			if (i % 2 == 0)
			{
				A.at<float>(i, 0) = picPt[i / 2].x;
				A.at<float>(i, 1) = picPt[i / 2].y;
				A.at<float>(i, 2) = 1;
				A.at<float>(i, 6) = -picPt[i / 2].x*realPt[i / 2].x;
				A.at<float>(i, 7) = -picPt[i / 2].y*realPt[i / 2].x;
				b.at<float>(i, 0) = realPt[i / 2].x;
			}
			else
			{
				A.at<float>(i, 3) = picPt[i / 2].x;
				A.at<float>(i, 4) = picPt[i / 2].y;
				A.at<float>(i, 5) = 1;
				A.at<float>(i, 6) = -picPt[i / 2].x*realPt[i / 2].y;
				A.at<float>(i, 7) = -picPt[i / 2].y*realPt[i / 2].y;
				b.at<float>(i, 0) = realPt[i / 2].y;
			}
		}
		solve(A, b, factor, cv::DECOMP_LU);
	}

}

void IntelligentTraffic::computeReal(cv::Mat &factor, cv::Point2f picPt, cv::Point2f &realPt)
{
	realPt.x = (factor.at<float>(0, 0)*picPt.x + factor.at<float>(1, 0)*picPt.y + factor.at<float>(2, 0)) / (factor.at<float>(6, 0)*picPt.x + factor.at<float>(7, 0)*picPt.y + 1);
	realPt.y = (factor.at<float>(3, 0)*picPt.x + factor.at<float>(4, 0)*picPt.y + factor.at<float>(5, 0)) / (factor.at<float>(6, 0)*picPt.x + factor.at<float>(7, 0)*picPt.y + 1);
}