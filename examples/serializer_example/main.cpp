#include "TestService.h"
#include "TestMsgJsonSerializer.h"
#include <optional_bundles/logging_bundle/LoggerAdmin.h>
#ifdef USE_SPDLOG
#include <optional_bundles/logging_bundle/SpdlogFrameworkLogger.h>
#include <optional_bundles/logging_bundle/SpdlogLogger.h>

#define FRAMEWORK_LOGGER_TYPE SpdlogFrameworkLogger
#define LOGGER_TYPE SpdlogLogger
#else
#include <optional_bundles/logging_bundle/CoutFrameworkLogger.h>
#include <optional_bundles/logging_bundle/CoutLogger.h>

#define FRAMEWORK_LOGGER_TYPE CoutFrameworkLogger
#define LOGGER_TYPE CoutLogger
#endif
#include <chrono>
#include <iostream>

int main() {
    std::locale::global(std::locale("en_US.UTF-8"));

    auto start = std::chrono::system_clock::now();
    DependencyManager dm{};
    auto logMgr = dm.createService<IFrameworkLogger, FRAMEWORK_LOGGER_TYPE>();
    auto logAdminMgr = dm.createService<ILoggerAdmin, LoggerAdmin<LOGGER_TYPE>>();
    auto serAdmin = dm.createService<ISerializationAdmin, SerializationAdmin>(RequiredList<ILogger>, OptionalList<>);
    auto testMsgSerializer = dm.createService<ISerializer, TestMsgJsonSerializer>(RequiredList<ILogger, ISerializationAdmin>, OptionalList<>);
    auto testMgr = dm.createService<ITestService, TestService>(RequiredList<ILogger, ISerializationAdmin>, OptionalList<>);
    dm.start();
    auto end = std::chrono::system_clock::now();
    std::cout << fmt::format("Program ran for {:n} µs\n", std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());

    return 0;
}