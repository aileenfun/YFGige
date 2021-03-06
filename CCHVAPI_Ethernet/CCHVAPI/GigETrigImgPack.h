#pragma once
//#include "DataCapture.h"
#include <vector>
#include "GigEimgFrame.h"
#include <map>
#include <iostream>
#include <fstream>
using namespace std;
typedef
VOID
	(WINAPI * LPMV_CALLBACK2)(LPVOID lpParam, LPVOID lpUser);
class GigECDataCapture;
class GigETrigImgPack
{
private:
	
	vector<GigEimgFrame*> imgPack;
	int camSize;
	int retry;
	int alarmPeriod;
	int recved;
	UINT_PTR id;
	LPMV_CALLBACK2 h_callback;
	int lastget;
	int softtrigmode;
	int softtrig;
public:
	bool timeup;
	 static std::map<UINT_PTR, GigETrigImgPack*> m_TrigImgPackMap; //declaration
	 bool b_getallpack;
	 GigECDataCapture* this_dp;
	 LPVOID lpcb;
	 //std::ofstream savefile2;
	 GigETrigImgPack(int cs,int ap, GigECDataCapture *dp,LPMV_CALLBACK2 CallBackFunc)
	{
		recved=0;
		camSize=cs;
		alarmPeriod=ap;
		h_callback=CallBackFunc;
		this_dp=dp;
		softtrigmode=0;
		softtrig=0;
		m_TrigImgPackMap[id]=this;
		//savefile2.open("C:\\c6udp\\callbackcnt.txt", std::ios::out | std::ios::binary);

	}
	~GigETrigImgPack(void);
	void InitPack()
	{
		recved=0;
		retry=0;
		b_getallpack=false;
		timeup=false;
		for(int i=0;i<camSize;i++)
		{
			GigEimgFrame *this_imgFrame=NULL;
			imgPack.push_back(this_imgFrame);
		}
				
	}
	void add(GigEimgFrame* imgFrame);
	bool getAllPack();
	void settrigmode(int trigmode)
	{
		switch(trigmode)
		{
		case 0://free run
			softtrigmode=0;
			break;
		case 1://extern trig
			softtrigmode=0;
			resetPack();
			break;
		case 2://
			softtrigmode=1;
			break;
		}
		
	}
	int softtrigonce()
	{
		softtrig>0?0:softtrig++;
		return softtrig;
		
	}
	int softtrigtimes()
	{
		return softtrig;
	}
	void consumePack(LPMV_CALLBACK2 CallBackFunc,LPVOID lpUser)
	{
		for(int i=0;i<camSize;i++)
		{
			//savefile2 << imgPack[i]->timestamp << endl;
			if(imgPack.size()!=0)
			CallBackFunc(imgPack[i], lpcb);
		}
		
	}
	void resetPack()
	{
		recved=0;
		retry=0;
		//Sleep(1);
		for(int i=0;i<imgPack.size();i++)
		{
			if(imgPack[i]!=NULL)
			delete imgPack[i];
		}
		imgPack.clear();
	}

	
};

