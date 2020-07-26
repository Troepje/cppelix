#include "OneService.h"
#include "OtherService.h"
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
#include <framework/CommunicationChannel.h>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    std::locale::global(std::locale("en_US.UTF-8"));

    auto start = std::chrono::system_clock::now();

    CommunicationChannel channel{};

    std::thread t1([&channel](){
        DependencyManager dm{};
        channel.addManager(&dm);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto logMgr = dm.createService<IFrameworkLogger, FRAMEWORK_LOGGER_TYPE>();
        auto logAdminMgr = dm.createService<ILoggerAdmin, LoggerAdmin<LOGGER_TYPE>>();
        auto oneService = dm.createService<IOneService, OneService>(RequiredList<ILogger>, OptionalList<>);
        dm.start();
    });

    std::thread t2([&channel](){
        DependencyManager dm{};
        channel.addManager(&dm);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto logMgr = dm.createService<IFrameworkLogger, FRAMEWORK_LOGGER_TYPE>();
        auto logAdminMgr = dm.createService<ILoggerAdmin, LoggerAdmin<LOGGER_TYPE>>();
        auto otherService = dm.createService<IOtherService, OtherService>(RequiredList<ILogger>, OptionalList<>);
        dm.start();
    });

    t1.join();
    t2.join();

    auto end = std::chrono::system_clock::now();
    std::cout << fmt::format("Program ran for {:n} µs\n", std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());

    return 0;
}