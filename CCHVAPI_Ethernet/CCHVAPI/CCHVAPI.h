//TODO:
//softTrig needs to be updated

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef COMPILE_API
#define CCT_API extern "C" __declspec(dllexport) 
#else 
#define CCT_API extern "C" __declspec(dllimport)
#endif

#include "resource.h"		// main symbols
#include "GigEDataCapture.h"
//#include "clientProp.h"
#include "GigEcamProp.h"
#include "DeviceGVCP.h"
#include "VirtualDevice.h"
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#define REG_SCREEN_CAP_CTRL 0x33bb0000
#define REG_SCREEN_CHANNEL_SEL 0x33bb0004
#define REG_SCREEN_SOFT_TRIG_CMD 0x33bb0008
#define REG_SCREEN_SAVE_FRAME_CNT 0x33bb000C
#define REG_SCREEN_SEND_FRAME_CNT 0x33bb0010
#define REG_SCREEN_RESOLUTION 0x33bb0014
#define REG_SCREEN_SEND_OFFSET 0x33bb0018
#define REG_SCREEN_INT_CTRL 0x33bb001C
#define REG_SCREEN_LINK_STATUS 0x33bb0020

#define REG_CAM_OUTPUT_MODE 0x33bb0040
#define REG_CAM_OUTPUT_EN 0x33bb0044
#define REG_CAM_START 0x33bb0048
#define REG_CAM_FRAME_INTERVAL 0x33bb004C
#define REG_CAM_FRAME_SEND_CNT 0x33bb0050
#define REG_CAM_0_OUTPUT_IMG_NUM 0x33bb0054//-0x33bb0060
#define REG_CAM_0_OUTPUT_RESOLU 0x33bb0064//-0x33bb0070
#define REG_CAM_0_1_VOLT 0x33bb0078
#define REG_CAM_2_3_VOLT 0x33bb007C



#define REG_SWITCH_SENDING_IMG_MODE 0x33bb0080
#define REG_SENDING_IMG_ID 0x33bb0084
#define REG_SENDING_IMG_RES 0x33bb0088
#define REG_SENDING_IMG_PIXEL_WIDTH 0x33bb008c
#define REG_SENDING_IMG_TOTAL_PACK 0x33bb0090
#define REG_SENDING_IMG_START 0x33bb0094
#define REG_SENDING_IMG_PACK_CNT 0x33bb0098
#define REG_SENDING_IMG_LOAD_FINISH 0x33bb009c
struct SendFileInfoStruct
{
	string filenames[10];
	int filecnt;
};
class GigEcamInstance  //= virtualGEVCam
{

private:
	VirtualDevice _VtDev;
	GigECDataProcess * m_pDataProcess;
	GigECDataCapture * m_pDataCapture;
	DeviceGVCP m_DeviceGVCP;
	volatile bool b_opened;
	bool b_closed;
	GigEwqueue<GigEimgFrame*>queue;
	LPVOID* lpUser;
	LPMV_CALLBACK2 cb;
	int socketSrv;
	void*   _pThreadGvsp;
	void* _pThreadSendFile;
	int width, height;
	GigEclientPropStruct *devprop;
	SendFileInfoStruct sendfileinfo;
public :
	int b_connected;
public:
	GigEcamInstance(LPVOID*lp,LPMV_CALLBACK2 CallBackFunc):m_DeviceGVCP()
	{
		m_pDataProcess=new GigECDataProcess(queue,lp);
		m_pDataCapture=new GigECDataCapture(queue,m_pDataProcess);
		b_opened=false;
		b_closed=true;
		b_connected=0;
		cb=CallBackFunc;
		socketSrv=-1;
		devprop = new GigEclientPropStruct();
		m_pDataProcess->lpcb = lp;
	}
	GigEcamInstance(LPVOID*lp,LPMV_CALLBACK2 CallBackFunc,CCHCamera *info):m_DeviceGVCP()
	{
		//m_DeviceGVCP.SetDeviceInfo(info);
		m_pDataProcess=new GigECDataProcess(queue,lp);
		m_pDataCapture=new GigECDataCapture(queue,m_pDataProcess);
		b_opened=false;
		b_closed=true;
		b_connected=0;
		cb=CallBackFunc;
		socketSrv=-1;
		devprop = new GigEclientPropStruct();
		m_pDataProcess->lpcb = lp;
	}
	GigEcamInstance() : m_DeviceGVCP()
	{
		//_VtDev.Init();
		b_opened = false;
		b_closed = true;
		b_connected = 0;
		cb = NULL;
		socketSrv = -1;
		devprop = new GigEclientPropStruct();
	}
	~GigEcamInstance()
	{
		/*if (m_pDataProcess != NULL)
		delete m_pDataProcess;
		
		if (m_pDataCapture != NULL)
		delete m_pDataCapture;*/
	}
	int initEth(CCHCamera *c,bool recv_thread=1)
	{
		if(b_connected ==1)//already opened
			return 1;
		if(b_opened)
		{
			return -2;
		}
		else
		{
			
			 if(m_DeviceGVCP.Init(c) != MV_OK)
			{
				return  -3;
			}
			 if (recv_thread)
			 {
				 _pThreadGvsp = MV_CreateThread(MV_NULL, DeviceGVCP::HandlingAckPacket, (&(this->m_DeviceGVCP)));
				 if (_pThreadGvsp == MV_NULL)
				 {
					return  -4;
				 }
			 }

				 devprop->pcIP = inet_addr(c->hostaddr.c_str());
				 uint32_t readdata;
				 int rst = ReadReg(0x0024, &readdata);//camip
				 if (rst<0)
					 if (m_DeviceGVCP.ReadRegDone() == -1)
					 {
						 return -5;
					 }
				 devprop->camIP = readdata;
				 sendProp(*devprop);
			 
		}
		b_connected = 1;
		return 1;
	}
	int updateCameraInfo()
	{
		//move connect to start.
		//connect only used to disable enthctrl(fix ip address), enable imgctrl, etc.
		//
		//
		if (b_connected <= 0)
		{
			return -1;
		}
		uint32_t readdata;
		
		int rst = ReadReg(0x33bb0010, &readdata);
		if (rst<0)//cam num
		{
			return -2;
		}

		devprop->camCnt = readdata;
		rst = ReadReg(0x33bb0014, &readdata);
		if (rst<0)//resolution
		{
			return -3;
		}

		uint32_t temp = readdata;
		devprop->width = (readdata >> 16);
		devprop->height = (temp & 0xFFFF);
		sendProp(*devprop);
		return 1;
	}
	static ThreadReturnType MV_STDCALL  sendFileProcess(void *arg)
	{
		
		GigEcamInstance* camins = (GigEcamInstance*)arg;
		for (int k = 0; k < camins->sendfileinfo.filecnt; k++)
		{
			string tempfilename(camins->sendfileinfo.filenames[k]);
			cv::Mat sendimg = cv::imread(tempfilename);
			cv::cvtColor(sendimg, sendimg, cv::COLOR_BGR2GRAY);


			char* buff = (char*)sendimg.data;
			int bufflen = sendimg.cols * sendimg.rows;
			int w = sendimg.cols;
			int h = sendimg.rows;

		JUMPOVER:
			uint32_t tempres = w;
			tempres = tempres << 16;
			tempres += h;
			int sendok = -1;
			char sendbuff[8192];
			int residue = bufflen % 8176;
			int copylen = 8176;

			camins->m_DeviceGVCP.WriteReg(REG_SWITCH_SENDING_IMG_MODE, 0);
			Sleep(1);
			camins->m_DeviceGVCP.WriteReg(REG_SWITCH_SENDING_IMG_MODE, 1);
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_ID, 0);
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_RES, tempres);
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_PIXEL_WIDTH, 2);
			int totalPack = bufflen / 8176;
			int rest = bufflen % 8176;//residue//8192-16
			if (rest > 0)
			{
				totalPack++;
			}
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_TOTAL_PACK, totalPack);
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_START, 1);
			camins->m_DeviceGVCP.WriteReg(REG_SENDING_IMG_START, 0);
			uint32_t camGetPack = 0;
			for (int i = 1; i < totalPack + 1; i++)
			{
				memset(sendbuff, 0, 8192);
				sendbuff[0] = 0x35;
				sendbuff[1] = 0xCE;
				sendbuff[14] = 0x2E;
				sendbuff[15] = 0xDE;
				if (i == totalPack)
					copylen = residue;
				memcpy(sendbuff + 16, buff + (i - 1) * 8176, copylen);
				//m_pDataCapture->sendFile(sendbuff, 8192, totalPack);
				int packOK = 0;
				int retry = 0;
				int retrylimit = 20;
				do
				{
					camins->m_pDataCapture->sendFile(sendbuff, 8192, totalPack);

					camGetPack = 0;

					while (!camGetPack)
					{
						Sleep(5);
						camins->m_DeviceGVCP.ReadReg(REG_SENDING_IMG_PACK_CNT, &camGetPack);
						retry++;
						if (retry == retrylimit)
						{
							goto JUMPOVER;
						}

					}
					//m_DeviceGVCP.WriteReg(REG_SWITCH_SENDING_IMG_MODE, 0);
					//camGetPack = camGetPack - 1;	
					uint32_t thisPackCnt = camGetPack & 0x0000ffff;
					if (thisPackCnt == i)
					{
						packOK = 1;
					}
					else
					{

						packOK = 0;
					}

					uint32_t m_thisPackLen = camGetPack;
					m_thisPackLen = m_thisPackLen & 0xffff0000;
					m_thisPackLen = m_thisPackLen >> 16;
					if (m_thisPackLen != 8192)
					{
						int tempppp = 1;
						packOK = 0;
					}
					retry++;
					if (retry == retrylimit)
					{
						goto JUMPOVER;
					}
				} while (!packOK);

				if (i == totalPack)
				{
					printf("%d", i);
				}
				//Sleep(1);
			}

			//m_DeviceGVCP.WriteReg(REG_SENDING_IMG_START, 0);

			camins->m_DeviceGVCP.WriteReg(REG_SWITCH_SENDING_IMG_MODE, 0);

			//m_pDataCapture->closeUDP();
		}
		
		return 1;
		
	}
	int sendFile(string filenames[10],int filecnt)
	{
		for (int l = 0; l < filecnt; l++)
		{
			sendfileinfo.filenames[l] = filenames[l];
		}
		sendfileinfo.filecnt = filecnt;

		updateCameraInfo();
		if (devprop == NULL)
		{
			return -1;
		}
		if (m_pDataCapture->initUDP(m_DeviceGVCP._UdpSkt,m_DeviceGVCP._From))//(&socketSrv))
		{
			_pThreadSendFile = MV_CreateThread(MV_NULL, GigEcamInstance::sendFileProcess, this);
			if (_pThreadSendFile == MV_NULL)
			{
				return  -4;
			}
		}
		else
		{
			return -2;
		}

	}
	int start()
	{

		//get height and width from reg
		//must connected
		//must know ip address from GVCP, read reg
		//send prop
		if(b_opened)
		{
			return 0;
		}
		int retry = 0;
		int rst = 0;
		while (rst <= 0&&retry<5)
		{
			rst=updateCameraInfo();
			retry++;
		}
		if (devprop == NULL)
		{
			return -1;
		}
		int port = m_pDataCapture->initUDP(m_DeviceGVCP._UdpSkt, m_DeviceGVCP._From);
		if(port>0)
		{
			m_DeviceGVCP.WriteReg(0x33bb00f8, port);
			m_pDataProcess->Open(devprop->height, devprop->width, m_pDataCapture, cb);
			m_pDataCapture->Open(devprop->height, devprop->width);
			b_opened = true;
			b_closed = false;
		}
		else
		{
			return -5;
		}
		return 1;
	}
	int stop()
	{
		//if(!b_opened)
		//	return -2;
		b_opened=false;
		m_pDataCapture->Close();
		//m_pDataCapture=NULL;
		m_pDataProcess->Close();
		//m_pDataProcess=NULL;
		return 0;
	}
	int isRunning()
	{
		bool temp=b_opened;
		return temp;
	}
	int getFrameCnt()
	{
		return m_pDataCapture->getFrameCnt();
	}
	int getDataCnt()
	{
		return m_pDataCapture->getDataCnt();
	}
	long getErrPackCnt()
	{
		return m_pDataCapture->haveerror;
	}
	int sendOrder(GigEcamPropStruct camprop,int s)
	{

		return m_pDataCapture->sendOrder(camprop,s);
	}
	int sendProp(GigEclientPropStruct prop)
	{
		m_pDataCapture->sendProp(prop);
		return 0;
	}
	int setTrigMode(int s)
	{
	
		m_DeviceGVCP.WriteReg(0x33bb0004, s);
		return m_DeviceGVCP.WriteRegDone();
	}

	int closeConnection()
	{
		m_DeviceGVCP.DeInit();
		m_pDataCapture->closeUDP();
		return 0;
	}
	int ReadReg(unsigned __int32 addr,uint32_t *data)
	{
		int rst= m_DeviceGVCP.ReadReg(addr, data);
		return rst;
	}
	unsigned __int32 WriteReg(unsigned __int32 addr,unsigned __int32 data)
	{

		return m_DeviceGVCP.WriteReg(addr, data);;
	}
	int forceIP(CCHCamera *devinfo)
	{

		m_DeviceGVCP.SetDeviceInfo(devinfo);
		return m_DeviceGVCP.ForceIP(devinfo);
	}
	int setIP(CCHCamera *devinfo)
	{
		//persistence ip ram and ee
		//subnet
		//gateway
		if (m_DeviceGVCP.WriteReg(0x064C, devinfo->CamInfo->stGigEInfo.nCurrentIp) < 0)//persistence ip
			return -1;
			Sleep(100);
			if (m_DeviceGVCP.WriteReg(0x065C, devinfo->CamInfo->stGigEInfo.nCurrentSubNetMask) < 0)//subnet
				return -2;
			Sleep(100);
			if (m_DeviceGVCP.WriteReg(0x066C, devinfo->CamInfo->stGigEInfo.nDefultGateWay) < 0)//gateway
				return -3;
			Sleep(100);
			return 1;
	
	}
	
	map_camera searchCamera(map_camera *camlist)
	{
		//m_DeviceGVCP.Discovery();
		CCHCamera*devinfo = new CCHCamera();
		devinfo->hostaddr = "0.0.0.0";
		initEth(devinfo, 0);//should not be used, just use getinterface. 
		m_DeviceGVCP.getInterface();
		*camlist = m_DeviceGVCP.cameralist;
		delete devinfo;
		return m_DeviceGVCP.cameralist;
	}
	int sendSoftTrig()
	{
		
		return m_DeviceGVCP.WriteReg(0x33bb0008, 1);;
	}
	int setAuto(int isauto)
	{
		return m_DeviceGVCP.WriteReg(0x33bb0034, isauto);
		
	}
	int setGain(uint32_t value,int isauto)
	{
		if (isauto)
			return -8;
		return m_DeviceGVCP.WriteReg(0x33bb003C,value);
		
	}
	int setExpo(uint32_t value,int isauto)
	{
		if (isauto)
			return -8;
		return m_DeviceGVCP.WriteReg(0x33bb0038,value);
		
	}
	int setFreq(uint32_t value)
	{
		return m_DeviceGVCP.WriteReg(0x33bb000C,value);

	}
	int setWB(uint32_t rvalue,uint32_t g1value,uint32_t g2value,uint32_t bvalue)
	{
		if(m_DeviceGVCP.WriteReg(0x33bb0040,g1value) \
			&&m_DeviceGVCP.WriteReg(0x33bb0044,bvalue) \
			&&m_DeviceGVCP.WriteReg(0x33bb0048,rvalue)\
		&&m_DeviceGVCP.WriteReg(0x33bb004C,g2value))
		{
			return 1;
		}
		
	}
	int setMirrorType(GigEDataProcessType mirrortype)
	{
		int temp = 0;
		switch (mirrortype)
		{
		case 0:
			temp = 0;
			break;
		case 1:
			temp = 0x4000;
			break;
		case 2:
			temp = 0x8000;
			break;
		case 3:
			temp = 0xc000;
			break;
		default:
			temp = 0;
			break;
		}
		return m_DeviceGVCP.WriteReg(0x33bb0050, temp);
		 
	}
	int setCamSize(int camsize)
	{
		m_pDataProcess->setCamSize(camsize);
		return (m_DeviceGVCP.WriteReg(0x33bb0010,camsize));
	}
	int getCamSize(unsigned int  *camsize)
	{
		 m_DeviceGVCP.ReadReg(0x33bb0010, camsize);
		 return *camsize;
	}
	int setROI(int xstart, int xend, int ystart, int yend, int enable)
	{
		if (!enable)
		{
			xstart = 0;
			xend = 1279;
			ystart = 0;
			yend = 959;
		}
		if (m_DeviceGVCP.WriteReg(0x33bb0018, ystart) &&
			m_DeviceGVCP.WriteReg(0x33bb001C, xstart) &&
			m_DeviceGVCP.WriteReg(0x33bb0020, yend) &&
			m_DeviceGVCP.WriteReg(0x33bb0024, xend)&&
			m_DeviceGVCP.WriteReg(0x33bb0028, enable))
		{
			return 1;
		}
		return -1;

	}
	int setROIEn(int enable)
	{
		return m_DeviceGVCP.WriteReg(0x33bb0028, enable);
	}
	int setBinning(int enable)
	{
		return m_DeviceGVCP.WriteReg(0x33bb002C, enable);
	}
	int setSkip(int enable)
	{
		return m_DeviceGVCP.WriteReg(0x33bb0030, enable);
	}
	int setTrigThreshold(int n)
	{
		return m_DeviceGVCP.WriteReg(0x33bb00f0, n);
	}
	int setPWM(int perc, int freq)
	{
		int datatemp = 0;
		datatemp = perc << 16;
		datatemp = datatemp + freq;
		return m_DeviceGVCP.WriteReg(0x33bb0060, datatemp);
	}
	int setGain_HZC(uint32_t value, int idx)
	{
		uint32_t addr = 0x33bb00a0;
		addr = addr + idx * 4;
		return m_DeviceGVCP.WriteReg(addr, value);
	}
	int setResolu_HZC(int value)
	{
		uint32_t addr = 0x33bb00c0;
		return m_DeviceGVCP.WriteReg(addr, value);
	}
	int setLightOn_XD(int s)
	{
		uint32_t addr = 0x33bb0070;
		return m_DeviceGVCP.WriteReg(addr, s);
	}
	int setLightLen_XD(uint32_t len)
	{
		uint32_t addr = 0x33bb0074;
		return m_DeviceGVCP.WriteReg(addr, len);
	}
	int setIOLength_MY(uint32_t us)
	{
		uint32_t addr = 0x33bb0064;
		return m_DeviceGVCP.WriteReg(addr, us);
	}
	/// <summary>
	/// YF
	/// </summary>
	int ScreenCapCtrl(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_CAP_CTRL, s);
	}
	int ScreenChannelSel(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_CHANNEL_SEL, s);
	}
	int ScreenSoftTrigCmd(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_SOFT_TRIG_CMD, s);
	}
	int ScreenSaveFrameCnt(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_SAVE_FRAME_CNT, s);
	}
	int ScreenSendFrameCnt(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_SEND_FRAME_CNT, s);
	}
	int ScreenRdResolu(uint32_t *s)
	{
		return m_DeviceGVCP.ReadReg(REG_SCREEN_RESOLUTION, s);
	}
	int ScreenSendOffset(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_SEND_OFFSET, s);
	}
	int ScreenINTCtrl(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_SCREEN_INT_CTRL, s);
	}
	int ScreenRdLinkStatus(uint32_t *s)
	{
		return m_DeviceGVCP.ReadReg(REG_SCREEN_LINK_STATUS, s);
	}

	//cam board
	int CamOutPutMode(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_CAM_OUTPUT_MODE, s);
	}
	int CamOutPutEn(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_CAM_OUTPUT_EN, s);
	}
	int CamOutStart(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_CAM_START, s);
	}
	int CamFrameInterval(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_CAM_FRAME_INTERVAL, s);
	}
	int CamFrameSendCnt(uint32_t s)
	{
		return m_DeviceGVCP.WriteReg(REG_CAM_FRAME_SEND_CNT, s);
	}
	int CamOutImgNum(uint32_t which,uint32_t s)
	{
		uint32_t addr = REG_CAM_0_OUTPUT_IMG_NUM + which * 4;

		return m_DeviceGVCP.WriteReg(addr, s);
	}
	int CamOutPutResolu(uint32_t which,uint32_t s)
	{
		uint32_t addr = REG_CAM_0_OUTPUT_RESOLU + which * 4;
		return m_DeviceGVCP.WriteReg(addr, s);
	}
	int CamRdVolt(uint32_t which, uint32_t* s)
	{
		uint32_t addr = REG_CAM_0_OUTPUT_RESOLU + which * 4;
		return m_DeviceGVCP.ReadReg(addr, s);
	}

};
CCT_API int GigEaddInstance(LPVOID *lpUser,LPMV_CALLBACK2 CallBackFunc,CCHCamera *info);
//CCT_API int initCCTAPI(int camNum);
CCT_API int GigEstartCap(int camNum=1);
CCT_API int GigEstopCap(int camNum=1);
CCT_API int GigEsetMirrorType(GigEDataProcessType mirrortype,int camNum=1);
CCT_API int GigEgetFrameCnt(int camNum=1);
CCT_API int GigEgetDataCnt(int camNum=1);
CCT_API long GigEgetErrPackCnt(int camNum=1);
CCT_API int GigEsetTrigMode(int s=0,int camNum=1);
CCT_API int GigEsendSoftTrig(int camNum=1);
CCT_API int GigEsetIP(CCHCamera *devinfo,int camNum=1);
CCT_API int GigEforceIP(CCHCamera *devinfo);
CCT_API int GigEcloseConnection(int &camNum);
CCT_API	int GigEsearchCamera(map_camera * info);
CCT_API unsigned __int32 GigEWriteReg(unsigned __int32 addr, unsigned __int32 data,int camNum=1);
CCT_API unsigned __int32 GigEReadReg(unsigned __int32 addr,uint32_t *data,int camNum=1);
CCT_API int GigEsetGain(uint32_t value,int isauto,int camNum=1);
CCT_API int GigEsetExpo(uint32_t value,int isauto,int camNum=1);
CCT_API int GigEsetFreq(uint32_t value,int camNum=1);
CCT_API int GigEsetWB(uint32_t rvalue,uint32_t gvalue,uint32_t g2value,uint32_t bvalue,int camNum=1);
CCT_API int GigEsetCamSize(int camsize,int camNum=1);
CCT_API int GigEgetCamSize(unsigned int *camsize,int camNum = 1);
CCT_API int GigEsetROI(int xstart, int xend, int ystart, int yend, int enable, int camNum = 1);
CCT_API int GigEsetROIEn(int enable, int camNum = 1);
CCT_API int GigEsetBinning(int enable,int camNum=1);
CCT_API int GigEsetSkip(int enable,int camNum=1);
CCT_API int GigEsetTrigThreshold(int n, int camNum = 1);
CCT_API int GigEsetPWM(int perc, int freq, int camNum = 1);
CCT_API int GigEsetGain_HZC(uint32_t value, int idx,int camNum);
CCT_API int GigEsetResolu_HZC(int value,int camNum=1);
CCT_API int GigEsetLightOn_XD(int s,int camNum);
CCT_API int GigEsetLightLen_XD(uint32_t len,int camNum);
CCT_API int GigEsetAuto(int isauto, int camNum);

typedef int(__stdcall *csCallBackFuncDel)(unsigned char *buff);
CCT_API int csInit(csCallBackFuncDel cb, int w, int h);
CCT_API int csSetROI(int xstart, int xend, int ystart, int yend, int enable);
CCT_API int csSetExpo(uint32_t value, int isauto);
CCT_API int csStart();
CCT_API int csStop();
CCT_API int csGetFrame(unsigned char * buff);
//#endif 
//parameter setting for laser
CCT_API int csSetGaussianA(uint8_t a);
CCT_API int csSetGaussianB(uint8_t b);
CCT_API int csSetGaussianC(uint8_t c);
CCT_API int csSetMaxBrightnessThreshold(uint8_t data);
CCT_API int csSetMaxLineWidth(uint32_t data);
CCT_API int csSetMinLineWidth(uint32_t data);
CCT_API int GigEsetIOLength_MY(uint32_t us, int camnum = 1);


/////YF////
CCT_API int GigEScreenCapCtrl(uint32_t s, int camnum = 1);
CCT_API int GigEScreenChannelSel(uint32_t s, int camnum = 1);
CCT_API int GigEScreenSoftTrig(uint32_t s, int camnum = 1);
CCT_API int GigEScreenSaveFrameCnt(uint32_t s, int camnum = 1);
CCT_API int GigEScreenSendFrameCnt(uint32_t s, int camnum = 1);
CCT_API int GigEScreenRdResolution(uint32_t* s, int camnum = 1);
CCT_API int GigEScreenSendOffset(uint32_t s, int camnum = 1);
CCT_API int GigEScreenINTCtrl(uint32_t s, int camnum = 1);
CCT_API int GigEScreenRdLinkStatus(uint32_t *s, int camnum = 1);

CCT_API int GigECamOutPutMode(uint32_t s, int camnum = 1);
CCT_API int GigECamOutPutEn(uint32_t s, int camnum = 1);
CCT_API int GigECamOutStart(uint32_t s, int camnum = 1);
CCT_API int GigECamFrameInterval(uint32_t s, int camnum = 1);
CCT_API int GigECamFrameSendCnt(uint32_t s, int camnum = 1);
CCT_API int GigECamOutPutImgNum(uint32_t s,uint32_t img_num, int camnum = 1);
CCT_API int GigECamOutPutResolu(uint32_t w,uint32_t s, int camnum = 1);
CCT_API int GigECamRdCamVolt(uint32_t which,uint32_t *v, int camnum = 1);
CCT_API int GigESendFile(string filenames[10],int filecnt,int camnum=1);
CCT_API int GigEScreenStartTest(int s);
//CCT_API int GigEConnectIP(std::string hostIP, std::string clientIP );
CCT_API  unsigned char* GigEScreenGrabOneFrame(int ins);
CCT_API int GigEConnectIP(std::string hostIP, std::string clientIP);
CCT_API int testIPString(char* hostIP,char* clientIP);
CCT_API int testDefaultIP();
CCT_API int connServer();
int sendImgTcp();