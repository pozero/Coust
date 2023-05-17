#pragma once

#include "utils/Enums.h"
#include "utils/Log.h"
#include "utils/Assert.h"
#include "utils/allocators/SmartPtr.h"

#include "core/Application.h"
#include "core/Logger.h"
#include "core/Memory.h"

#if defined(COUST_TEST)
    #include "test/RunTest.h"
#endif

#include "core/EntryPoint.h"
