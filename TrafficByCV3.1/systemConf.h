#pragma once

#include "vector"
#include "IntelligentTraffic.h"
using namespace std;


class systemConf
{
public:
	systemConf();
	~systemConf();
	bool SystemConfInit();
public:
	struct sys_Point
	{
		int x;
		int y;
	};
	struct sys_Rect
	{
		sys_Point begin;
		int m_width;
		int m_height;
	};

	struct stopRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_time;
	};
	struct reversedriveRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_distance;
	};
	struct pedestrianEntryRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_time;
		int m_distance;
	};
	struct jamRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_num;
		int m_speed;
	};
	struct driveOutOfBorderRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
	};
	struct lossRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_width;
		int m_height;
		int m_time;
	};

	struct carNumRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
	};
	struct carSizeRule
	{
		bool m_use;
		bool m_draw;
		bool m_savePic;
		//bool m_saveResult;
		int m_width;
		int m_height;
	};

	struct region
	{
		int m_id;
		sys_Point m_region[4];
		sys_Point m_directtion[2];
		stopRule m_stopRule;
		reversedriveRule m_reversedriveRule;
		pedestrianEntryRule m_pedestrianEntryRule;
		jamRule m_jamRule;
		driveOutOfBorderRule m_driveOutOfBorderRule;
		lossRule m_lossRule;
		carNumRule m_carNumRule;
		carSizeRule m_carSizeRule;
		bool isOn;
		region()
		{
			isOn = false;
		}
	};

public:
	int m_imgwidth;
	int m_imgheight;
	int regionNum;
	sys_Rect m_processImgRect;
	region m_regionOut[4];
public:
	void StringSplit(string s, char splitchar, vector<string>& vec);
	int parseCameraListXml(vector<string>& cameraList);
	void writeTxtandXml(vector<IntelligentTraffic::EventsResults> &eventsResults, IntelligentTraffic::SituatResults &situationResults, string time[2]);
	void saveEventsPic(int isHappen, bool savePic, vector<IntelligentTraffic::EventsResults> &eventsResults);
	void saveSituatPic(bool isJam, bool savePic, string time[2], IntelligentTraffic::SituatResults &situationResults);
};

