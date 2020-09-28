#pragma once

#include <iostream>

extern "C"
{
#include "libavformat\avformat.h"
#include "libavcodec\avcodec.h"
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avdevice.lib")


class CH2642RTMP
{
public:
	explicit CH2642RTMP();
	virtual ~CH2642RTMP();

public:
	bool Init(const char* rtmpUrl);
	void AddData(char *pData, int iSize);
	void SetInterval(int interval);
	void DataPreproce(char *pData, int iSize);

private:
	bool InitCodec();
	bool H264_decode_sps(char* bData, int Length, int &width, int &height);
	bool GetOneH264Length(int &iLength);
	void SendRtmp(char *pData, int iSize);

private:
	AVFormatContext* outputContext = nullptr;
	AVPacket *packet = nullptr;
	int64_t m_lastpts = 0;
	const char* m_rtmpUrl;

	int m_interlval = 40;

	char m_Sps[1024] = { 0 };
	int m_SpsLength = 0;

	char m_Pps[1024] = { 0 };
	int m_PpsLength = 0;

	char m_SEI[10240] = { 0 };
	int m_SEILength = 0;

	char *m_H264 = nullptr;
	const int m_H264Length = 1024 * 124;
	int m_CurrentH264Length = 0;

	char *m_SendH264 = nullptr;
	const int m_SendH264Length = 1024 * 124;

	bool m_InitCodec = false;
	bool m_InitCodecOK = false;


	char m_Cache[1024 * 1024] = { 0 };
	int m_CacheSize = 0;
};

