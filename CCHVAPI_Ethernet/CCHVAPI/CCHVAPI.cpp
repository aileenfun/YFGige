#include "stdafx.h"
#include "CCHVAPI.h"
#include "Inc/Socket/Socket.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#define _W1280
//192.168.2.11
vector<GigEcamInstance*>vec_camins;
GigEcamInstance searchCamIns;
int f_grabframe = 0;
unsigned char* grabframebuff;
unsigned char* pRBuffer;
unsigned char* pGBuffer;
unsigned char* pBBuffer;
uint32_t FrameCnt = 0;
int g_iWidth = 1920;
int g_iHeight = 1080;
MVComponent::TCP tcpSkt;
int tcpRunning = 0;
int tcpBoard = 0;
#define HOST_IP "192.168.2.12"
#define HUD_948_IP_10 "192.168.2.10"
#define DASH_948_IP_11 "192.168.2.11"
#define CAM_933_IP "192.168.2.9"
#define LABVIEW_SERVER_PORT 6340
#define HUD_948_ID 1
#define DASH_948_ID 2
#define CAM_933_ID 3
int f_sendTCP = 0;
std::vector<string> vec_ip;

int GigeSendARP(string destIP, string hostIP)
{
	DWORD dwRetVal;
	IPAddr DestIp = 0;
	IPAddr SrcIp = 0;       /* default for src ip */
	ULONG MacAddr[2];       /* for 6-byte hardware addresses */
	ULONG PhysAddrLen = 6;  /* default to length of six bytes */

	char* DestIpString = NULL;
	char* SrcIpString = NULL;

	BYTE* bPhysAddr;
	unsigned int i;
	DestIpString = (char*)destIP.c_str();
	DestIp = inet_addr(DestIpString);

	memset(&MacAddr, 0xff, sizeof(MacAddr));

	printf("Sending ARP request for IP address: %s\n", DestIpString);

	dwRetVal = SendARP(DestIp, SrcIp, &MacAddr, &PhysAddrLen);

	if (dwRetVal == NO_ERROR) {
		bPhysAddr = (BYTE*)&MacAddr;
		if (PhysAddrLen) {
			for (i = 0; i < (int)PhysAddrLen; i++) {
				if (i == (PhysAddrLen - 1))
					printf("%.2X\n", (int)bPhysAddr[i]);
				else
					printf("%.2X-", (int)bPhysAddr[i]);
			}
		}
		else
			printf
			("Warning: SendArp completed successfully, but returned length=0\n");

	}
	else {
		printf("Error: SendArp failed with error: %d", dwRetVal);
		switch (dwRetVal) {
		case ERROR_GEN_FAILURE:
			printf(" (ERROR_GEN_FAILURE)\n");
			break;
		case ERROR_INVALID_PARAMETER:
			printf(" (ERROR_INVALID_PARAMETER)\n");
			break;
		case ERROR_INVALID_USER_BUFFER:
			printf(" (ERROR_INVALID_USER_BUFFER)\n");
			break;
		case ERROR_BAD_NET_NAME:
			printf(" (ERROR_GEN_FAILURE)\n");
			break;
		case ERROR_BUFFER_OVERFLOW:
			printf(" (ERROR_BUFFER_OVERFLOW)\n");
			break;
		case ERROR_NOT_FOUND:
			printf(" (ERROR_NOT_FOUND)\n");
			break;
		default:
			printf("\n");
			break;
		}
	}
	return 1;
}
void _stdcall  Disp(LPVOID lpParam, LPVOID lpUser)
{
	GigEimgFrame* thisFrame = (GigEimgFrame*)lpParam;
	if (thisFrame == NULL)
		return;

		g_iWidth= thisFrame->m_width;
		g_iHeight = thisFrame->m_height;
		long lvbufflen = g_iWidth * g_iHeight;
		memset(grabframebuff, 0, lvbufflen);
		long idx_LV = 0;
		//RGBA
		memcpy(grabframebuff, thisFrame->imgBuf, lvbufflen);
		
		for (int i = 0; i < lvbufflen; i = i + 4)
		{//ABGR                        RGBA
			//*(grabframebuff+idx_LV+1)=*(thisFrame->imgBuf + i +2);
			//*(grabframebuff + idx_LV + 2) = *(thisFrame->imgBuf + i + 1);
			//*(grabframebuff + idx_LV + 3) = *(thisFrame->imgBuf + i + 0);
			//BGRA
			//*(grabframebuff + idx_LV) = *(thisFrame->imgBuf + i + 2);
			//*(grabframebuff + idx_LV + 1) = *(thisFrame->imgBuf + i + 1);
			//*(grabframebuff + idx_LV + 2) = *(thisFrame->imgBuf + i + 0);
			
			//RGBA
			//memcpy(grabframebuff+idx_LV,pDataBuffer + i,3);
			//idx_LV += 4;
			//ARGB
			//memcpy(grabframebuff + idx_LV + 1, thisFrame->imgBuf + i, 3);
			idx_LV += 4;
		}
		//cv::Mat frame2(g_iHeight, g_iWidth / 4, CV_8UC4, thisFrame->imgBuf, cv::Mat::AUTO_STEP);
		//cv::imshow("disp", frame2);
		//cv::waitKey(1);
		/*cv::Mat Rmat(g_iHeight/3, g_iWidth, CV_8UC1, pRBuffer);
		cv::Mat Gmat(g_iHeight/3, g_iWidth, CV_8UC1, pGBuffer);
		cv::Mat Bmat(g_iHeight/3, g_iWidth, CV_8UC1, pBBuffer);
		std::vector<cv::Mat> array_to_merge;
		array_to_merge.push_back(Bmat);
		array_to_merge.push_back(Gmat);
		array_to_merge.push_back(Rmat);
		cv::Mat color;
		cv::merge(array_to_merge, color);*/
		//cv::imwrite("IMG.jpg", color);
		f_grabframe = 2;
		
		sendImgTcp();

	
}

int GigEaddInstance(LPVOID *lpUser, LPMV_CALLBACK2 CallBackFunc, CCHCamera *info)
{
	GigEcamInstance*this_camInstance = new GigEcamInstance(lpUser, CallBackFunc, info);
	int rst = 0;
	if (this_camInstance->initEth(info)<0)
	{
		//rst=this_camInstance->connect(info);//move to start()
		return -1;
	}
	vec_camins.push_back(this_camInstance);
	return vec_camins.size();
}

int GigEstartCap(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
		
	camNum = camNum - 1;
	
	if (vec_camins[camNum]->b_connected)
	{
		return vec_camins[camNum]->start();
	}
	return -1;
}
int GigEsetMirrorType(GigEDataProcessType mirrortype, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	vec_camins[camNum]->setMirrorType(mirrortype);
	return 0;
}
int GigEstopCap(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;

	return vec_camins[camNum]->stop();
}
int GigEgetFrameCnt(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	if (vec_camins[camNum]->isRunning())
	{
		return vec_camins[camNum]->getFrameCnt();
	}
	else
	{
		return -1;
	}
}
int GigEgetDataCnt(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	if (vec_camins[camNum]->isRunning())
	{
		return vec_camins[camNum]->getDataCnt();
	}
	else
	{
		return -1;
	}
}
long GigEgetErrPackCnt(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	if (vec_camins[camNum]->isRunning())
	{
		return vec_camins[camNum]->getErrPackCnt();
	}
	else
	{
		return -1;
	}
}
int GigEsendOrder(GigEcamPropStruct camprop, int camNum, int s)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	vec_camins[camNum]->sendOrder(camprop, s);
	return 0;
}

int GigEsendProp(GigEclientPropStruct prop, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	vec_camins[camNum]->sendProp(prop);
	return 0;
}
int GigEsetTrigMode(int s, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	vec_camins[camNum]->setTrigMode(s);
	return 0;
}

int GigEcloseConnection(int &camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	vec_camins[camNum]->closeConnection();
	delete vec_camins[camNum];
	vector<GigEcamInstance*>::iterator k = vec_camins.begin() + camNum;
	vec_camins.erase(k);
	//camNum=-camNum;
	return 0;
}
int GigEsetIP(CCHCamera *devinfo, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setIP(devinfo);
}
int GigEforceIP(CCHCamera *devinfo)
{
	return searchCamIns.forceIP(devinfo);
}
int GigEsearchCamera(map_camera * camlist)
{
	//may need to restart searchCamIns
	searchCamIns.searchCamera(camlist);
	return camlist->size();
}
unsigned __int32 GigEWriteReg(unsigned __int32 addr, unsigned __int32 data, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->WriteReg(addr, data);
}
unsigned __int32 GigEReadReg(unsigned __int32 addr, uint32_t *data, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ReadReg(addr, data);
}
int GigEsendSoftTrig(int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->sendSoftTrig();
}
int GigEsetAuto(int isauto, int camNum)
{
	return vec_camins[camNum]->setAuto(isauto);
}
int GigEsetGain(uint32_t value, int isauto, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setGain(value, isauto);
}
int GigEsetExpo(uint32_t value, int isauto, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setExpo(value, isauto);
}
int GigEsetFreq(uint32_t value, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setFreq(value);
}
int GigEsetWB(uint32_t rvalue, uint32_t gvalue, uint32_t g2value, uint32_t bvalue, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setWB(rvalue, gvalue, g2value, bvalue);
}
int GigEsetCamSize(int camsize, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setCamSize(camsize);
}
int GigEgetCamSize(unsigned int *camsize, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->getCamSize(camsize);
}
int GigEsetROI(int xstart, int xend, int ystart, int yend, int enable, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setROI(xstart, xend, ystart, yend, enable);
}
int GigEsetSkip(int enable, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setSkip(enable);
}
int GigEsetBinning(int enable, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setBinning(enable);
}
int GigEsetTrigThreshold(int n,int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setTrigThreshold(n);
}
int GigEsetPWM(int perc, int freq, int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setPWM(perc,freq);
}
int GigEsetROIEn(int enable,int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setROIEn(enable);

}
int GigEsetGain_HZC(uint32_t value, int idx,int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setGain_HZC(value, idx);
}
int GigEsetResolu_HZC(int value,int camNum)
{
	if (camNum<1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	if(value>1)
		value=1;
	if(value<0)
		value=0;
	return vec_camins[camNum]->setResolu_HZC(value);
}
int GigEsetIOLength_MY(uint32_t us, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setIOLength_MY(us);
}
int GigEsetLightOn_XD(int s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setLightOn_XD(s);
}
int GigEsetLightLen_XD(uint32_t len, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->setLightLen_XD(len);
}

byte * pimagebuf = NULL;
int imgready = 0;
int board1 = 1;
#ifdef _W1280
int cs_width = 752;
int cs_height = 960;
#endif
#ifdef _W640
int cs_width = 640;
int cs_height = 480;
#endif
map_camera *cameralist;

csCallBackFuncDel Handler = 0;

void __stdcall csCallBack(LPVOID lpParam, LPVOID lpUser)
{

	GigEimgFrame *thisFrame = (GigEimgFrame*)lpParam;
	if (thisFrame->m_camNum == 0)
	{
		//memset(pimagebuf, 11, thisFrame->m_height*thisFrame->m_width);
		memcpy(pimagebuf, thisFrame->imgBuf, thisFrame->m_width*thisFrame->m_height);
		//imgready=1;
		Handler(pimagebuf);
	}

}
int csInit(csCallBackFuncDel cb, int w = 1280, int h = 1028)
{
	Handler = cb;
	cs_width = w;
	cs_height = h;
	pimagebuf = new byte[cs_width*cs_height];
	memset(pimagebuf, 0, cs_width*cs_height);
	cameralist = new map_camera();
	GigEsearchCamera(cameralist);//Ã¶¾ÙÏà»ú
	int count = 0;
	map_camera::iterator itr;
	itr = cameralist->begin();
	if (itr != cameralist->end())
	{
		CCHCamera *c0 = itr->second;
		board1 = GigEaddInstance(NULL, csCallBack, c0);
		return board1;
	}
	else
	{
		return -1;
	}
}
int csSetROI(int xstart, int xend, int ystart, int yend, int enable)
{
	GigEsetROI(xstart, xend, ystart, yend, enable, board1);

	return 1;
}
int csSetExpo(uint32_t value, int isauto)
{
	GigEsetExpo(value, isauto, board1);
	return 1;
}
int csStart()
{
	int temp = 0;
	if (board1>0)
		temp = GigEstartCap(board1);
	return temp;
}
int csStop()
{
	GigEstopCap(board1);
	return 1;
}
int csGetFrame(unsigned char * buff)
{
	//sendSoftTrig(board1,2);
	while (!imgready)
	{
		Sleep(1);
	}
	//Sleep(100);
	memcpy(buff, pimagebuf, cs_height*cs_width);
	//cv::Mat frameGray(cv_height,cv_width,CV_8UC1,pimagebuf);
	//cv::Mat frameRGB;
	//cv::cvtColor(frameGray,frameRGB,CV_BayerBG2BGR);
	imgready = 0;
	return cs_height*cs_width;
}

///////parameter setting for Laser center point
/*
Gaussian parameters:

----------------
| c  | b | c  |
----------------
| b  | a | b  |
----------------
| c  | b | c  |
----------------
*/

int csSetGaussianA(uint8_t a)
{

	return GigEWriteReg(0x33bb0100, a, board1);
}
int csSetGaussianB(uint8_t b)
{
	return GigEWriteReg(0x33bb0104, b, board1);
}
int csSetGaussianC(uint8_t c)
{
	return GigEWriteReg(0x33bb0108, c, board1);
}
int csSetMaxBrightnessThreshold(uint8_t data)
{
	return GigEWriteReg(0x33bb010c, data, board1);
}
int csSetMaxLineWidth(uint32_t data)
{
	return GigEWriteReg(0x33bb0110, data, board1);
}
int csSetMinLineWidth(uint32_t data)
{
	return GigEWriteReg(0x33bb0114, data, board1);
}


//////////////YF/////////////
int GigEScreenCapCtrl(uint32_t s, int camNum)
{

	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenCapCtrl(s);
}
int GigEScreenChannelSel(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenChannelSel(s);
}
int GigEScreenSoftTrig(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenSoftTrigCmd(s);
}
int GigEScreenSaveFrameCnt(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenSaveFrameCnt(s);
 }
int GigEScreenSendFrameCnt(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenSendFrameCnt(s);
 }

int GigEScreenRdResolution(uint32_t* s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenRdResolu(s);
 }
int GigEScreenSendOffset(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenSendOffset(s);
 }
int GigEScreenINTCtrl(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenINTCtrl(s);
 }
int GigEScreenRdLinkStatus(uint32_t* s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->ScreenRdLinkStatus(s);
 }
int GigECamOutPutMode(uint32_t s, int camNum)
{
	if (camNum < 1)return camNum;
	if (camNum > vec_camins.size())return -2;
	camNum = camNum - 1;
	return vec_camins[camNum]->CamOutPutMode(s);
 }
 int GigECamOutPutEn(uint32_t s, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamOutPutEn(s);
 }
 int GigECamOutStart(uint32_t s, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamOutStart(s);
 }
 int GigECamFrameInterval(uint32_t s, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamFrameInterval(s);
 }
 int GigECamFrameSendCnt(uint32_t s, int camNum)
 {
	 if (camNum < 1)return camNum;
	 //if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamFrameSendCnt(s);
 }
 int GigECamOutPutImgNum(uint32_t s, uint32_t img_num, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamOutImgNum(s, img_num);
 }
 int GigECamOutPutResolu(uint32_t w,uint32_t s ,int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamOutPutResolu(w,s);
 }

 int GigECamRdCamVolt(uint32_t which, uint32_t* v, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;
	 return vec_camins[camNum]->CamRdVolt(which,v);
 }
 int GigESendFile(string filenames[10], int filecnt, int camNum)
 {
	 if (camNum < 1)return camNum;
	 if (camNum > vec_camins.size())return -2;
	 camNum = camNum - 1;

	 return vec_camins[camNum]->sendFile(filenames, filecnt);
 }

 int GigEConnectIP(std::string hostIP, std::string clientIP,int height,int width)
 //int GigEConnectIP()
 {
	 unsigned long long temp = htonl(inet_addr(clientIP.c_str()));
	 CCHCamera* cam = new CCHCamera();
	 cam->hostaddr = hostIP;
	 cam->CamInfo->stGigEInfo.nCurrentIp = temp;
	 cam->str_camIP = clientIP;
	 LPVOID* lpUser = NULL;
	 int rst=GigEaddInstance(lpUser,  Disp, cam);
	 if (rst > 0)
	 {
		 if(grabframebuff==NULL)
		 grabframebuff = new unsigned char[1080 * 1920 * 4];
		 GigEScreenResSet(height, width, rst);
		 GigEstartCap(rst);
		 Sleep(2000);
		 GigEScreenStartTest(1,rst);
		 GigEScreenGrabOneFrame(1,rst);
	 }

	 return rst;
 }

 int GigEScreenStartTest(int s,int board)
 {
	 if (s == 1)
	 {
		 GigEScreenSendFrameCnt(1, board);
		 GigEScreenCapCtrl(0,board);
		 GigEScreenCapCtrl(1,board);
		 GigEScreenSendFrameCnt(1,board);
		 GigEScreenSoftTrig(1,board);
	 }
	 else
	 {
		 GigEScreenCapCtrl(0,board);
	 }
	 return 1;
 }
 int GigEScreenResSet(int height, int width, int board)
 {
	 if (height > 0)
	 {
		 unsigned long temp = height;
		 temp = temp << 16;
		 temp = temp + width;
		 GigEWriteReg(REG_SCREEN_RES_SET_EN, 1, board);
		 return GigEWriteReg(REG_SCREEN_RES_OVERRIDE, temp, board);
	 }
	 else
	 {
		 GigEWriteReg(REG_SCREEN_RES_SET_EN, 0, board);
	 }
 }
 unsigned char* GigEScreenGrabOneFrame(int Ins,int board)
 {
	// f_grabframe = 1;
	 unsigned long count = 0;
	 FrameCnt++;
	 while (count < 50)
	 {
		 count++;
#ifdef  _FAKEDATA
		 memset(grabframebuff, FrameCnt, g_iWidth * g_iHeight);
		 f_grabframe = 2;
#endif //  _FAKEDATA
		 if (f_grabframe > 1)
		 {

			 f_grabframe = 0;
			 return grabframebuff;
		 }
		 Sleep(10);
	 }
	 return NULL;
 }
int  GigEScreenGrabOneFrameDLL(int Ins,int board,int& width, int& height,unsigned char *buff)
 {
	
	unsigned char* temp=GigEScreenGrabOneFrame(Ins, board);
	
	 if (temp != NULL)
	 {
		 width = g_iWidth;
		 height = g_iHeight;
		 memcpy(buff, grabframebuff, width * height);
		/* for (int i = 0; i < width * height; i++)
		 {
			 buff[i] = grabframebuff[i];
		 }*/
	 }
	 else
	 {
		 width = 0;
		 height = 0;
	 }
	 return width;
	 
 }
 int testIPString(char* c_hostIP, char* c_clientIP)
 {
	 string hostip(c_hostIP);
	 string clientIp(c_clientIP);

	 int rst = GigEConnectIP(hostip, clientIp);
	 return rst;
 }


 static ThreadReturnType MV_STDCALL tcpCommand(LPVOID lpParam)
 {
	 const int cmdLen = 7;
	 char buff[cmdLen];
	 unsigned char* temp = NULL;

	 while (tcpRunning)
	 {
		 memset(buff, 0, cmdLen);
		// int rst= tcpSkt.ReceiveTimeout(1000,buff, cmdLen);
		 int rst = tcpSkt.Receive( buff, cmdLen);
		 if (rst <= 0)
		 {
			 Sleep(10);
			 continue;
		 }
		 char head = buff[0];
		 char board = buff[1];
		 char cmd = buff[2];
		 uint16_t param = buff[3];
		 param << 8;
		 param += buff[4];
		 uint32_t value;
		 if (head == 0x55)
		 {
			 if (board > vec_camins.size())
				 continue;
			 GigeSendARP(vec_camins[board - 1]->GigEGetCamIP());
			 if (board == 1 || board == 2)
			 {
				 tcpBoard = board;
				 switch (cmd)
				 {
				 case 1:
					 GigEScreenStartTest(1,board);
					 temp=GigEScreenGrabOneFrame(1,board);
					 if (temp == NULL)
					 {
						 GigEScreenStartTest(1, board);
					 }
					 break;
				 case 2:
					 GigEScreenStartTest(0,board);
					 break;
				 case 3:
					 if (buff[6] >= 0 || buff[6] <= 1)
					 {
						 GigEScreenChannelSel(buff[6],board);
					 }
				 case 4:
					 if (buff[6] >= 0 || buff[6] <= 1)
					 {
						 GigEScreenINTCtrl(buff[6],board);
					 }
				 case 5:

					 GigEScreenRdLinkStatus(&value,board);
					 memcpy(buff + 3, &value, 4);
					 tcpSkt.Send((char*)buff, cmdLen);
					 break;
				 case 6:
					 GigEScreenRdResolution(&value,board);
					 memcpy(buff + 3, &value, 4);
					 tcpSkt.Send((char*)buff, cmdLen);
					 break;
				 default:

					 break;
				 }
			 }//end of board 1 2
			 else if (board == 3)
			 {
				 switch (cmd)
				 {
				 case 1:
					 if (buff[6] == 1)
					 {
						 GigECamOutPutMode(0,board);
						 GigECamOutPutMode(1,board);
						 GigECamOutStart(1,board);
					 }
					 else if (buff[6] == 0)
					 {
						 GigECamOutPutMode(0,board);
					 }
					 break;
				 case 2:
					 value = buff[6];
					 GigECamOutPutMode(0,board);
					 GigECamOutPutEn(value,board);

					 break;
				 case 3:
					 break;
				 default:
					 break;
				 }
			 }
			
		 }//head==0x55
		// 
		
	 }
	 return 1;
 }
 int connServer()
 {
	 if (tcpRunning == 1)
		 return -1;

	 MVComponent::Address tcpaddr;
	 tcpaddr.SetAddressIp("127.0.0.1");
	 //tcpaddr.SetAddressIp("192.168.2.10");
	 tcpaddr.SetAddressPort(LABVIEW_SERVER_PORT);
	 if(tcpSkt.ConnectTo(tcpaddr)<0)
	 {

		 return -1;
	 }
	 tcpRunning = 1;
	 void* _pThreadTcpCmd = MV_CreateThread(MV_NULL, tcpCommand, (LPVOID)NULL);
	 int rst;
	 if (_pThreadTcpCmd == MV_NULL)
	 {
		 rst = -4;
	 }
	 return 1;
 }
 int closeConn()
 {
	 tcpSkt.Close();
	 return 1;
 }
 int connectDevice1DLL(int s,int height,int width)
 {
	 if (s == 1)
	 {

		 GigeSendARP(HUD_948_IP_10);
		 int rst = GigEConnectIP(HOST_IP, HUD_948_IP_10,height,width);
		 if (rst >0)
		 {

			 GigEScreenStartTest(1, 2);
			 GigEScreenGrabOneFrame(1, 2);

		 }
		 else
		 {
			 rst = GigEConnectIP(HOST_IP, HUD_948_IP_10,height,width);
			 if (rst >0)
			 {

				 GigEScreenStartTest(1, 2);
				 GigEScreenGrabOneFrame(1, 2);

			 }
			 else
			 {
				 return -1;
			 }
		 }
	 }
	 else if(s==2)
	 {

		 GigeSendARP(DASH_948_IP_11);
		 int rst = GigEConnectIP(HOST_IP, DASH_948_IP_11, height, width);
		 if (rst > 0)
		 {

			 GigEScreenStartTest(1, 1);
			 GigEScreenGrabOneFrame(1, 1);

		 }
		 else
		 {
			 rst = GigEConnectIP(HOST_IP, DASH_948_IP_11, height, width);
			 if (rst > 0)
			 {

				 GigEScreenStartTest(1, 1);
				 GigEScreenGrabOneFrame(1, 1);

			 }
			 else
			 {
				 return -1;
			 }
		 }

	 }
	 return 1;
 }
 int touchTest1DLL(int value, int s)
 {
	 if (s == 1)
	 {
		 return GigEScreenINTCtrl(value, 1);
	 }
	 else
	 {
		 return GigEScreenINTCtrl(value, 2);
	 }
	 
 }
 int screenTest1DLL(int s)
 {
	unsigned  char* temp = NULL;
	 if (s == 1)
	 {
		 GigeSendARP(HUD_948_IP_10);
		 GigEScreenStartTest(1, 1);
		
	 }
	 else if(s==2)
	 {
		 GigeSendARP(DASH_948_IP_11);
		 GigEScreenStartTest(1, 2);
		
	 }
	 //GigEScreenGrabOneFrame(1, 1);
	 return 1;
 }
 int disconnDevice1DLL()
 {
	 int board = 1;
	 GigEstopCap();
	 GigEcloseConnection(board);
	 return 1;
 }
 int connBoard()
 {
	 f_sendTCP = 0;
	 char buff[7] = { 0 };
	 GigeSendARP(HUD_948_IP_10);
	 int rst=GigEConnectIP(HOST_IP, HUD_948_IP_10);
	 buff[0] = 0x55;
	 buff[1] = 0x01;
	 if (rst == 1)
	 {
		 
		 GigEScreenStartTest(1, 1);
		 GigEScreenGrabOneFrame(1, 1);
		 
		 buff[6] = 0x01;
		 
	 }
	 else
	 {
		 rst = GigEConnectIP(HOST_IP, HUD_948_IP_10);

		 buff[0] = 0x55;
		 buff[1] = 0x01;
		 if (rst == 1)
		 {

			 GigEScreenStartTest(1, 1);
			 GigEScreenGrabOneFrame(1, 1);

			 buff[6] = 0x01;

		 }
		 else
		 {
			 buff[6] = 0x00;
		 }
	 }
	//tcpSkt.Send(buff, 7);
	 //Connect DASH
#ifdef DASH_948_IP_11
	 GigeSendARP(DASH_948_IP_11);
	 rst = GigEConnectIP(HOST_IP, DASH_948_IP_11);
	 buff[1] = 0x02;
	 if(rst==2)
	 {
		 f_sendTCP = 0;
		 GigEScreenStartTest(1, 2);
		 GigEScreenGrabOneFrame(1, 2);
		 buff[6] = 0x01;
	 }
	 else
	 {
		
		 rst = GigEConnectIP(HOST_IP, DASH_948_IP_11);
		 if (rst == 2)
		 {
			 f_sendTCP = 0;
			 GigEScreenStartTest(1, 2);
			 GigEScreenGrabOneFrame(1, 2);
			 buff[6] = 0x01;
		 }
		 else
		 {
			 buff[6] = 0x00;
		 }

	 }
	 //tcpSkt.Send(buff, 7);
#endif
	 Sleep(3000);
	 f_sendTCP = 1;
	 return rst;
	 
 }
 int disConnServer()
 {
	 tcpRunning = 0;
	 tcpSkt.Close();
	 return 1;
 }

 int sendImgTcp()
 {
	 if (tcpRunning == 1)
	 {
		 char buff[7];
		 buff[0] = 0x55;
		 buff[1] = tcpBoard;
		 buff[2] = 0x01;
		 buff[3] = g_iWidth >> 8;
		 buff[4] = g_iWidth & 0xff;
		 buff[5] = g_iHeight >> 8;
		 buff[6] = g_iHeight & 0xff;

		 

		 if (f_sendTCP == 1)
		 {
			 tcpSkt.Send(buff, 7);
			 tcpSkt.Send((char*)grabframebuff, g_iWidth * g_iHeight);
		 }
	 }
	 //f_grabframe = 3;
	 return 1;
 }
