#pragma once
//#include "DataCapture.h"
#include  <string>

struct GigEclientPropStruct
{
	int camCnt;
	DWORD pcIP;
	DWORD camIP;
	int packetSize;
	int interval_time;
	unsigned long long MACaddr;
	int width;
	int height;
	std::string str_pcIP;
	std::string str_camIP;
};

class GigEclientProp
{
public:
	GigEclientProp(void);
	~GigEclientProp(void);
	int initBuffer();
	int getCamCnt();
	int setCamCnt(int camCnt);
	int setpcIP(DWORD IP);
	DWORD getpcIP();
	int setcamIP(DWORD IP);
	DWORD getcamIP();
	int getBuffer(char * destbuffer);
	int setClientProp(GigEclientPropStruct prop);
	int getClientProp(GigEclientPropStruct *prop);
	GigEclientPropStruct memClientProp;
	
private:
	char *sendbuffer;
	
		
	int camCnt;
	DWORD pcIP;
	DWORD camIP;
	int packetSize;
	int interval_time;
	unsigned long MACaddr;
	std::string str_pcIP;
	std::string str_camIP;
};

