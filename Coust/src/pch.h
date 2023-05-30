#pragma once

// Disable exception in stl (microsoft implementation)
#if defined(__clang__)
    #pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#endif
#define _HAS_EXCEPTIONS 0

#include <expected>

#include <format>
#include <string>
#include <fstream>
#include <iostream>
#include <string_view>

#include <random>
#include <memory>
#include <chrono>

#include <span>
#include <ranges>
#include <iterator>
#include <numeric>
#include <algorithm>
#include <functional>

#include <utility>
#include <cctype>
#include <cstring>
#include <optional>

#include <type_traits>

#include <filesystem>

#include <map>
#include <set>
#include <list>
#include <queue>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
