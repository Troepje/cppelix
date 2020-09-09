#include "UsingEtcdService.h"
#include <optional_bundles/logging_bundle/LoggerAdmin.h>
#include <optional_bundles/etcd_bundle/EtcdService.h>
#include <framework/CommunicationChannel.h>
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
#include <string>

int main() {
    using namespace std::string_literals;
    std::locale::global(std::locale("en_US.UTF-8"));

    auto start = std::chrono::system_clock::now();

    CommunicationChannel channel{};
    DependencyManager dmOne{};
    DependencyManager dmTwo{};
    channel.addManager(&dmOne);
    channel.addManager(&dmTwo);

    std::thread t1([&dmOne] {
        auto logMgr = dmOne.createServiceManager<FRAMEWORK_LOGGER_TYPE, IFrameworkLogger>();
        logMgr->setLogLevel(LogLevel::INFO);
#ifdef USE_SPDLOG
        dmOne.createServiceManager<SpdlogSharedService, ISpdlogSharedService>();
#endif
        dmOne.createServiceManager<LoggerAdmin<LOGGER_TYPE>, ILoggerAdmin>();
        dmOne.createServiceManager<EtcdService, IEtcdService>(CppelixProperties{{"EtcdAddress", "localhost:2379"s}});
        dmOne.createServiceManager<UsingEtcdService, IUsingEtcdService>();
        dmOne.start();
    });

    std::thread t2([&dmTwo] {
        auto logMgr = dmTwo.createServiceManager<FRAMEWORK_LOGGER_TYPE, IFrameworkLogger>();
        logMgr->setLogLevel(LogLevel::INFO);
#ifdef USE_SPDLOG
        dmTwo.createServiceManager<SpdlogSharedService, ISpdlogSharedService>();
#endif
        dmTwo.createServiceManager<LoggerAdmin<LOGGER_TYPE>, ILoggerAdmin>();
        dmTwo.createServiceManager<EtcdService, IEtcdService>(CppelixProperties{{"EtcdAddress", "localhost:2379"s}});
        dmTwo.createServiceManager<UsingEtcdService, IUsingEtcdService>();
        dmTwo.start();
    });

    t1.join();
    t2.join();

    auto end = std::chrono::system_clock::now();
    std::cout << fmt::format("Program ran for {:L} µs\n", std::chrono::duration_cast<std::chrono::microseconds>(end-start).count());

    return 0;
}