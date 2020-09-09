#pragma once

#include <framework/DependencyManager.h>
#include <optional_bundles/logging_bundle/Logger.h>
#include <optional_bundles/etcd_bundle/IEtcdService.h>
#include "framework/Service.h"
#include "framework/LifecycleManager.h"

using namespace Cppelix;


struct IUsingEtcdService : virtual public IService {
    static constexpr InterfaceVersion version = InterfaceVersion{1, 0, 0};
};

class UsingEtcdService final : public IUsingEtcdService, public Service {
public:
    UsingEtcdService(DependencyRegister &reg, CppelixProperties props) : Service(std::move(props)) {
        reg.registerDependency<ILogger>(this, true);
        reg.registerDependency<IEtcdService>(this, true);
    }
    ~UsingEtcdService() final = default;

    bool start() final {
        LOG_INFO(_logger, "UsingEtcdService started");
        if(_etcd->put("test", "2")) {
            LOG_TRACE(_logger, "Succesfully put key/value into etcd");
            auto storedVal = _etcd->get("test");
            if(storedVal == "2") {
                LOG_TRACE(_logger, "Succesfully retrieved key/value into etcd");
            } else {
                LOG_ERROR(_logger, "Error retrieving key/value into etcd");
            }
        } else {
            LOG_ERROR(_logger, "Error putting key/value into etcd");
        }

        getManager()->pushEvent<QuitEvent>(getServiceId(), INTERNAL_EVENT_PRIORITY+1);
        return true;
    }

    bool stop() final {
        LOG_INFO(_logger, "UsingEtcdService stopped");
        return true;
    }

    void addDependencyInstance(ILogger *logger) {
        _logger = logger;
    }

    void removeDependencyInstance(ILogger *logger) {
        _logger = nullptr;
    }

    void addDependencyInstance(IEtcdService *etcd) {
        _etcd = etcd;
    }

    void removeDependencyInstance(IEtcdService *etcd) {
        _etcd = nullptr;
    }

private:
    ILogger *_logger{nullptr};
    IEtcdService *_etcd{nullptr};
};