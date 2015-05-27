// Header.h
#pragma once

#include <map>
#include <set>
#include <queue>
#include <vector>

#include <string>

#include <memory>

#include <chrono>

#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

using uint = unsigned int;
using byte = unsigned char;

#define Now chrono::system_clock::now().time_since_epoch().count()

#define PI 3.14159265359f