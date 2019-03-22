#include "systemConf.h"
#include "tinyxml.h"
#include <fstream>
#include <iostream>
#include <string>
#include <direct.h> 
#include <windows.h>

systemConf::systemConf()
{
	m_imgwidth = 1280;
	m_imgheight = 720;
	regionNum = 0;
	sys_Point initP;
	initP.x = 0;
	initP.y = 0;
	m_processImgRect.begin = initP;
	m_processImgRect.m_width = 0;
	m_processImgRect.m_height = 0;
	memset(m_regionOut, 0, sizeof(m_regionOut));
}


systemConf::~systemConf()
{
}

/*************************************************
Function: StringSplit
Description: 字符串分割
Calls:
Table Accessed:
Table Updated:
Input:
s		: 字符串
splitchar：分割字符串

Output:
vec：分割后保存的结果
Return:
Others:
*************************************************/
void systemConf::StringSplit(string s, char splitchar, vector<string>& vec)
{
	if (vec.size() > 0)//保证vec是空的  
		vec.clear();
	int length = s.length();
	int start = 0;
	for (int i = 0; i < length; i++)
	{
		if (s[i] == splitchar && i == 0)//第一个就遇到分割符  
		{
			start += 1;
		}
		else if (s[i] == splitchar)
		{
			vec.push_back(s.substr(start, i - start));
			start = i + 1;
		}
		else if (i == length - 1)//到达尾部  
		{
			vec.push_back(s.substr(start, i + 1 - start));
		}
	}
}


int systemConf::parseCameraListXml(vector<string>& cameraList)
{
	char szModuleFilePath[MAX_PATH];
	char szIniPathFileName[MAX_PATH];
	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
	strcpy(szIniPathFileName, szModuleFilePath);
	strcat(szIniPathFileName, "\\CameraList.xml");
	TiXmlDocument doc;
	std::string getValue = "";
	if (doc.LoadFile(szIniPathFileName))
	{
		TiXmlElement* rootElement = doc.RootElement();  //Root节点
		TiXmlElement* classElement = rootElement->FirstChildElement();  // Class元素
		getValue = classElement->Value();
		while (getValue != "cameraName")
		{
			if (classElement == NULL)
			{
				return 2;//不存在节点
			}
			classElement = classElement->NextSiblingElement();
			getValue = classElement->Value();
		}
		TiXmlElement* endElement = classElement->FirstChildElement();

		for (; endElement != NULL; endElement = endElement->NextSiblingElement())
		{
			getValue = endElement->Value();
			if (getValue == "cameraName")
			{
				cameraList.push_back(endElement->GetText());

			}
		}
		return 0;//获取成功
	}
	else
	{
		return 1;
	}
}


bool systemConf::SystemConfInit()
{
	using namespace std;
	char szModuleFilePath[MAX_PATH];
	char szIniPathFileName[MAX_PATH];
	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;

	// Log path file
	strcpy_s(szIniPathFileName, szModuleFilePath);
	strcat_s(szIniPathFileName, "RulesSitting.xml");

	std::string getValue = "";
	char* xmlFile = szIniPathFileName;


	TiXmlDocument doc(xmlFile);
	if (doc.LoadFile())
	{
		TiXmlElement* rootElement = doc.RootElement();  //School元素
		TiXmlElement* classElement = rootElement->FirstChildElement();  // Class元素
		const char *  getValue = classElement->Value();
		const char *  Value = "Rule";
		while (strcmp(getValue, Value))
		{
			if (classElement == NULL)
			{
				return 2;//Rule节点不存在
			}
			classElement = classElement->NextSiblingElement();
			getValue = classElement->Value();
		}
		TiXmlElement* childElement = classElement->FirstChildElement();
		if (childElement != NULL)
		{
			for (; childElement != NULL; childElement = childElement->NextSiblingElement())
			{
				getValue = childElement->Value();

				if (!strcmp(getValue, "ConfigeImage"))
				{
					TiXmlElement* endElement = childElement->FirstChildElement();
					m_imgwidth = atoi(endElement->GetText());
					endElement = endElement->NextSiblingElement();
					m_imgheight = atoi(endElement->GetText());
				}
				else if (!strcmp(getValue, "ProcessImage"))
				{
					TiXmlElement* endElement = childElement->FirstChildElement();
					char p = '-';
					std::vector<std::string> vect;
					StringSplit(endElement->GetText(), p, vect);
					vector<string>::iterator it = vect.begin();
					m_processImgRect.begin.x = atoi(it->c_str());
					it++;
					m_processImgRect.begin.y = atoi(it->c_str());
					it++;
					m_processImgRect.m_width = atoi(it->c_str());
					it++;
					m_processImgRect.m_height = atoi(it->c_str());
				}
				else if (!strcmp(getValue, "DrawRegion"))
				{
					int tempid = atoi(childElement->FirstAttribute()->Value());
					if (tempid<5 && tempid>0)
					{
						m_regionOut[tempid - 1].m_id = tempid;
						TiXmlElement* endElement = childElement->FirstChildElement();
						vector<string> vect;
						char p = '-';
						StringSplit(endElement->GetText(), p, vect);
						vector<string>::iterator it = vect.begin();
						m_regionOut[tempid - 1].m_region[0].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[0].y = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[1].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[1].y = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[2].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[2].y = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[3].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_region[3].y = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_directtion[0].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_directtion[0].y = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_directtion[1].x = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_directtion[1].y = atoi(it->c_str());
						it++;

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_stopRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_stopRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_stopRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_stopRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_stopRule.m_time = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_reversedriveRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_reversedriveRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_reversedriveRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_reversedriveRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_reversedriveRule.m_distance = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_pedestrianEntryRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_pedestrianEntryRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_pedestrianEntryRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_pedestrianEntryRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_pedestrianEntryRule.m_time = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_pedestrianEntryRule.m_distance = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_jamRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_jamRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_jamRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_jamRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_jamRule.m_num = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_jamRule.m_speed = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_driveOutOfBorderRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_driveOutOfBorderRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_driveOutOfBorderRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						//m_regionOut[tempid - 1].m_driveOutOfBorderRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_lossRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_lossRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_lossRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_lossRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_lossRule.m_width = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_lossRule.m_height = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_lossRule.m_time = atoi(it->c_str());

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_carNumRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_carNumRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_carNumRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						//m_regionOut[tempid - 1].m_carNumRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);

						vect.clear();
						endElement = endElement->NextSiblingElement();
						StringSplit(endElement->GetText(), p, vect);
						it = vect.begin();
						m_regionOut[tempid - 1].m_carSizeRule.m_use = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_carSizeRule.m_draw = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						m_regionOut[tempid - 1].m_carSizeRule.m_savePic = (atoi(it->c_str()) == 1 ? true : false);
						it++;
						/*m_regionOut[tempid - 1].m_carSizeRule.m_saveResult = (atoi(it->c_str()) == 1 ? true : false);
						it++;*/
						m_regionOut[tempid - 1].m_carSizeRule.m_width = atoi(it->c_str());
						it++;
						m_regionOut[tempid - 1].m_carSizeRule.m_height = atoi(it->c_str());

						if (m_regionOut[tempid - 1].m_stopRule.m_use && m_regionOut[tempid - 1].m_reversedriveRule.m_use && m_regionOut[tempid - 1].m_pedestrianEntryRule.m_use && m_regionOut[tempid - 1].m_driveOutOfBorderRule.m_use \
							&& m_regionOut[tempid - 1].m_jamRule.m_use && m_regionOut[tempid - 1].m_lossRule.m_use && m_regionOut[tempid - 1].m_carNumRule.m_use && m_regionOut[tempid - 1].m_carSizeRule.m_use)
						{
							m_regionOut[tempid - 1].isOn = true;
						}
						if (m_regionOut[tempid - 1].isOn == true)
						{
							regionNum++;
						}
					}
				}
			}
			return 0;
		}
		else
		{
			return 3;///ConfigeImage、ProcessImage、DrawRegion节点都不存在
		}
	}
	else
	{
		return 1;//文件不存在
	}
}

void systemConf::writeTxtandXml(vector<IntelligentTraffic::EventsResults> &eventsResults, IntelligentTraffic::SituatResults &situationResults, string time[2])
{
	ofstream outfile;
	//得到txt、xml的路径
	char szModuleFilePath[MAX_PATH];
	char txtSaveResult[MAX_PATH];
	char xmlSaveResult[MAX_PATH];
	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
	strcpy_s(txtSaveResult, szModuleFilePath);
	strcpy_s(xmlSaveResult, szModuleFilePath);
	strcat_s(txtSaveResult, "\\Data\\");
	strcat_s(xmlSaveResult, "\\Data\\");
	string tmptxt = time[0] + "AnalysisData.txt";
	string tmpxml = "AnalysisData.xml";
	strcat_s(txtSaveResult, tmptxt.c_str());
	strcat_s(xmlSaveResult, tmpxml.c_str());
	//写入txt
	outfile.open(txtSaveResult, ofstream::out | ofstream::app);
	if (!outfile)
	{
		cout << "创建文件失败！" << endl;
	}
	string tr = time[0];
	outfile << time[0] << " " << time[1] << endl;
	int numTmp = 0;
	for (int i = 0; i<eventsResults.size(); ++i)
	{
		for (int j = 0; j<5; ++j)
		{
			if (eventsResults[i].m_events[j] != 0)
			{
				numTmp++;
				outfile << "id:" << eventsResults[i].m_id << " event:" << j << " pos:" << "[" << eventsResults[i].m_pos[j].x << "," << eventsResults[i].m_pos[j].y << "];" << endl;
			}
		}
	}
	outfile << "CarNum:" << situationResults.carNum << " Jam:" << situationResults.jam << endl;
	//outfile.close();

	//写入xml
	string tempStr; ostringstream tmpStream;
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "gbk", ""); //版本 
	doc.LinkEndChild(decl);
	tempStr = "data time=\"" + time[0] + " " + time[1] + "\"";
	TiXmlElement *timeElement = new TiXmlElement(tempStr.c_str());
	doc.LinkEndChild(timeElement);
	TiXmlElement *puidElement = new TiXmlElement("PUID");
	tempStr = "00E04C742541";
	puidElement->LinkEndChild(new TiXmlText(tempStr.c_str()));
	doc.LinkEndChild(puidElement);
	TiXmlElement *eleElement = new TiXmlElement("ElementInfo");
	doc.LinkEndChild(eleElement);
	TiXmlElement *numElement = new TiXmlElement("Num");
	tmpStream << numTmp;
	tempStr = tmpStream.str();
	tmpStream.str("");
	numElement->LinkEndChild(new TiXmlText(tempStr.c_str()));
	eleElement->LinkEndChild(numElement);
	TiXmlElement *eventElement = new TiXmlElement("EventInfo");
	eleElement->LinkEndChild(eventElement);
	for (int i = 0; i<eventsResults.size(); ++i)
	{
		TiXmlElement *idElement = new TiXmlElement("ID");
		tmpStream << eventsResults[i].m_id;
		tempStr = tmpStream.str();
		tmpStream.str("");
		idElement->LinkEndChild(new TiXmlText(tempStr.c_str()));
		eventElement->LinkEndChild(idElement);
		for (int j = 0; j<5; j++)
		{
			if (eventsResults[i].m_events[j] != 0)
			{
				TiXmlElement *typeElment = new TiXmlElement("TYPE");
				tmpStream << j;
				tempStr = tmpStream.str();
				tmpStream.str("");
				typeElment->LinkEndChild(new TiXmlText(tempStr.c_str()));
				idElement->LinkEndChild(typeElment);
				TiXmlElement *posxElment = new TiXmlElement("POSX");
				tmpStream << eventsResults[i].m_pos[j].x;
				tempStr = tmpStream.str();
				tmpStream.str("");
				posxElment->LinkEndChild(new TiXmlText(tempStr.c_str()));
				idElement->LinkEndChild(posxElment);
				TiXmlElement *posyElment = new TiXmlElement("POSY");
				tmpStream << eventsResults[i].m_pos[j].y;
				tempStr = tmpStream.str();
				tmpStream.str("");
				posyElment->LinkEndChild(new TiXmlText(tempStr.c_str()));
				idElement->LinkEndChild(posyElment);
				TiXmlElement *eventtimeElment = new TiXmlElement("TIME");
				tempStr = eventsResults[i].day[j] + "-" + eventsResults[i].second[j];
				eventtimeElment->LinkEndChild(new TiXmlText(tempStr.c_str()));
				idElement->LinkEndChild(eventtimeElment);
			}
		}
	}
	TiXmlElement *carnumElement = new TiXmlElement("CarNum");
	tmpStream << situationResults.carNum;
	tempStr = tmpStream.str();
	tmpStream.str("");
	carnumElement->LinkEndChild(new TiXmlText(tempStr.c_str()));
	eleElement->LinkEndChild(carnumElement);
	TiXmlElement *jamElement = new TiXmlElement("Jam");
	tmpStream << situationResults.jam;
	tempStr = tmpStream.str();
	tmpStream.str("");
	jamElement->LinkEndChild(new TiXmlText(tempStr.c_str()));
	eleElement->LinkEndChild(jamElement);
	doc.SaveFile(xmlSaveResult);
}
void systemConf::saveEventsPic(int isHappen, bool savePic, vector<IntelligentTraffic::EventsResults> &eventsResults)
{
	switch (isHappen)
	{
	case 1:
		if (savePic)
		{
			for (int i = 0; i<eventsResults.size(); ++i)
			{
				if (!eventsResults[i].isPicSaved[i] && eventsResults[i].m_events[0] == 1)
				{
					//得到stop_pic的路径
					char szModuleFilePath[MAX_PATH];
					char picSaveResult[MAX_PATH];
					int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
					szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
					strcpy_s(picSaveResult, szModuleFilePath);
					strcat_s(picSaveResult, "Data\\Stop\\");

					cv::Mat img_Tmp(720, 720, CV_8UC3);
					resize(eventsResults[i].img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
					ostringstream id; id << eventsResults[i].m_id;
					string tmpname = id.str() + "-" + eventsResults[i].day[0] + "-" + eventsResults[i].second[0] + ".jpg";
					strcat_s(picSaveResult, tmpname.c_str());
					imwrite(picSaveResult, img_Tmp);
					eventsResults[i].isPicSaved[i]=true;
				}
			}
		}
	case  2:
		if (savePic)
		{
			for (int i = 0; i<eventsResults.size(); ++i)
			{
				if (!eventsResults[i].isPicSaved[i] && eventsResults[i].m_events[1] == 1)
				{
					//得到stop_pic的路径
					char szModuleFilePath[MAX_PATH];
					char picSaveResult[MAX_PATH];
					int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
					szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
					strcpy_s(picSaveResult, szModuleFilePath);
					strcat_s(picSaveResult, "Data\\Reverse\\");

					cv::Mat img_Tmp(720, 720, CV_8UC3);
					resize(eventsResults[i].img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
					ostringstream id; id << eventsResults[i].m_id;
					string tmpname = id.str() + "-" + eventsResults[i].day[1] + "-" + eventsResults[i].second[1] + ".jpg";
					strcat_s(picSaveResult, tmpname.c_str());
					imwrite(picSaveResult, img_Tmp);
					eventsResults[i].isPicSaved[i] = true;
				}
			}
		}
	case 3:
		if (savePic)
		{
			for (int i = 0; i<eventsResults.size(); ++i)
			{
				if (!eventsResults[i].isPicSaved[i] && eventsResults[i].m_events[2]==1)
				{
					//得到stop_pic的路径
					char szModuleFilePath[MAX_PATH];
					char picSaveResult[MAX_PATH];
					int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
					szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
					strcpy_s(picSaveResult, szModuleFilePath);
					strcat_s(picSaveResult, "Data\\DiveOut\\");

					cv::Mat img_Tmp(720, 720, CV_8UC3);
					resize(eventsResults[i].img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
					ostringstream id; id << eventsResults[i].m_id;
					string tmpname = id.str() + "-" + eventsResults[i].day[2] + "-" + eventsResults[i].second[2] + ".jpg";
					strcat_s(picSaveResult, tmpname.c_str());
					imwrite(picSaveResult, img_Tmp);
					eventsResults[i].isPicSaved[i] = true;
				}
			}
		}
	case 4:
		if (savePic)
		{
			for (int i = 0; i<eventsResults.size(); ++i)
			{
				if (!eventsResults[i].isPicSaved[i] && eventsResults[i].m_events[3]==1)
				{
					//得到stop_pic的路径
					char szModuleFilePath[MAX_PATH];
					char picSaveResult[MAX_PATH];
					int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
					szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
					strcpy_s(picSaveResult, szModuleFilePath);
					strcat_s(picSaveResult, "Data\\PedestrianEnty\\");

					cv::Mat img_Tmp(720, 720, CV_8UC3);
					resize(eventsResults[i].img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
					ostringstream id; id << eventsResults[i].m_id;
					string tmpname = id.str() + "-" + eventsResults[i].day[3] + "-" + eventsResults[i].second[3] + ".jpg";
					strcat_s(picSaveResult, tmpname.c_str());
					imwrite(picSaveResult, img_Tmp);
					eventsResults[i].isPicSaved[i] = true;
				}
			}
		}
	case 5:
		if (savePic)
		{
			for (int i = 0; i<eventsResults.size(); ++i)
			{
				if (!eventsResults[i].isPicSaved[i] && eventsResults[i].m_events[4]==1)
				{
					//得到stop_pic的路径
					char szModuleFilePath[MAX_PATH];
					char picSaveResult[MAX_PATH];
					int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
					szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
					strcpy_s(picSaveResult, szModuleFilePath);
					strcat_s(picSaveResult, "Data\\Loss\\");

					cv::Mat img_Tmp(720, 720, CV_8UC3);
					resize(eventsResults[i].img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
					ostringstream id; id << eventsResults[i].m_id;
					string tmpname = id.str() + "-" + eventsResults[i].day[4] + "-" + eventsResults[i].second[4] + ".jpg";
					strcat_s(picSaveResult, tmpname.c_str());
					imwrite(picSaveResult, img_Tmp);
					eventsResults[i].isPicSaved[i] = true;
				}
			}
		}
	}
}
void systemConf::saveSituatPic(bool isJam, bool savePic, string time[2], IntelligentTraffic::SituatResults &situationResults)
{
	if (savePic)
	{
		//得到stop_pic的路径
		char szModuleFilePath[MAX_PATH];
		char picSaveResult[MAX_PATH];
		int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH);
		szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;
		strcpy_s(picSaveResult, szModuleFilePath);
		strcat_s(picSaveResult, "Data\\Jam\\");

		cv::Mat img_Tmp(720, 720, CV_8UC3);
		resize(situationResults.img_result, img_Tmp, cv::Size(720, 720), CV_INTER_CUBIC);
		string tmpname = time[0] + "-" + time[1] + ".jpg";
		strcat_s(picSaveResult, tmpname.c_str());
		imwrite(picSaveResult, img_Tmp);
	}
}