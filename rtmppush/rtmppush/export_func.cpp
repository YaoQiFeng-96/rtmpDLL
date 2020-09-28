#include "export_func.h"

RTMPDLL void rtmp_init()
{
	mutex_rtmpalloc.lock();
	rtmp_key = 0;
	mutex_rtmpalloc.unlock();

	mutex_rtmpmap.lock();
	rtmp_map.clear();
	mutex_rtmpmap.unlock();
	return;
}

RTMPDLL void rtmp_release_all()
{
	mutex_rtmpmap.lock();
	std::unordered_map<int64_t, std::shared_ptr<CH2642RTMP>>::iterator pos;
	for (pos = rtmp_map.begin(); pos != rtmp_map.end();)
		rtmp_map.erase(pos++);
	mutex_rtmpmap.unlock();
	return;
}

RTMPDLL int64_t rtmp_alloc(const char * url)
{
	mutex_rtmpalloc.lock();
	int64_t new_key = ++rtmp_key;
	mutex_rtmpalloc.unlock();

	if (rtmp_map.count(new_key) == 0)
	{
		auto h264 = std::make_shared<CH2642RTMP>();
		if (h264->Init(url))
		{
			mutex_rtmpmap.lock();
			rtmp_map.insert(std::make_pair(new_key, h264));
			mutex_rtmpmap.unlock();
			return new_key;
		}
		else
			return -2;
	}
	else 
		return -1;
}

RTMPDLL void rtmp_release_one(int64_t key)
{
	mutex_rtmpmap.lock();
	std::unordered_map<int64_t, std::shared_ptr<CH2642RTMP>>::iterator pos = rtmp_map.find(key);
	if (pos != rtmp_map.end())
		rtmp_map.erase(pos);
	mutex_rtmpmap.unlock();
	return;
}

RTMPDLL void rtmp_add_data(int64_t key, char * pData, int iSize)
{
	mutex_rtmpmap.lock();
	std::unordered_map<int64_t, std::shared_ptr<CH2642RTMP>>::iterator pos = rtmp_map.find(key);
	if (pos != rtmp_map.end())
	{
		auto ptr = pos->second;
		mutex_rtmpmap.unlock();
		ptr->DataPreproce(pData, iSize);
		return;
	}
	mutex_rtmpmap.unlock();
	return;
}

RTMPDLL void rtmp_set_interval(int64_t key, int interval)
{
	mutex_rtmpmap.lock();
	std::unordered_map<int64_t, std::shared_ptr<CH2642RTMP>>::iterator pos = rtmp_map.find(key);
	if (pos != rtmp_map.end())
	{
		auto ptr = pos->second;
		mutex_rtmpmap.unlock();
		ptr->SetInterval(interval);
		return;
	}
	mutex_rtmpmap.unlock();
	return;
}




