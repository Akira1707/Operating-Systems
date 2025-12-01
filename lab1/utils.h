#pragma once
#include <string>
#include <syslog.h>
#include <cstdarg>
#include <chrono>
#include <ctime>

std::string timestamp_now();
void safe_syslog(int priority, const char* fmt, ...);