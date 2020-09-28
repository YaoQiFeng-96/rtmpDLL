#pragma once

#include <iostream>
#include <stdint.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <unordered_map>

#include "H2642RTMP.h"

std::atomic_int64_t rtmp_key;
std::mutex mutex_rtmpalloc;
std::unordered_map<int64_t, std::shared_ptr<CH2642RTMP>> rtmp_map;
std::mutex mutex_rtmpmap;