#pragma once

#include <framework/DependencyManager.h>
#include <optional_bundles/logging_bundle/Logger.h>
#include "framework/Service.h"
#include "framework/ServiceLifecycleManager.h"
#include "RuntimeCreatedService.h"

using namespace Cppelix;


struct ITrackerService : virtual public IService {
    static constexpr InterfaceVersion version = InterfaceVersion{1, 0, 0};
};

class TrackerService final : public ITrackerService, public Service {
public:
    ~TrackerService() final = default;
    bool start() final {
        LOG_INFO(_logger, "TrackerService started");
        _trackerRegistration = getManager()->registerDependencyTracker<IRuntimeCreatedService>(getServiceId(), this);
        return true;
    }

    bool stop() final {
        LOG_INFO(_logger, "TrackerService stopped");
        _trackerRegistration.reset(nullptr);
        return true;
    }

    void addDependencyInstance(ILogger *logger) {
        _logger = logger;
        LOG_INFO(_logger, "Inserted logger");
    }

    void removeDependencyInstance(ILogger *logger) {
        _logger = nullptr;
    }

    void handleDependencyRequest(IRuntimeCreatedService*, DependencyRequestEvent const * const evt) {
        auto scopeProp = evt->properties->find("scope");

        if(scopeProp == end(*evt->properties)) {
            LOG_ERROR(_logger, "scope missing");
            return;
        }

        auto scope = std::any_cast<std::string>(scopeProp->second);

        LOG_INFO(_logger, "Tracked IRuntimeCreatedService request for scope {}", scope);

        auto runtimeService = _scopedRuntimeServices.find(scope);

        if(runtimeService == end(_scopedRuntimeServices)) {
            _scopedRuntimeServices.emplace(scope, getManager()->createService<IRuntimeCreatedService, RuntimeCreatedService>(RequiredList<ILogger>, OptionalList<>, CppelixProperties{{"scope", scope}}));
        }
    }

    void handleDependencyUndoRequest(IRuntimeCreatedService*, DependencyUndoRequestEvent const * const evt) {
        auto scopeProp = evt->properties->find("scope");

        if(scopeProp == end(*evt->properties)) {
            LOG_ERROR(_logger, "scope missing");
            return;
        }

        auto scope = std::any_cast<std::string>(scopeProp->second);

        LOG_INFO(_logger, "Tracked IRuntimeCreatedService undo request for scope {}", scope);

        _scopedRuntimeServices.erase(scope);
    }

private:
    ILogger *_logger;
    std::unique_ptr<DependencyTrackerRegistration> _trackerRegistration;
    std::unordered_map<std::string, RuntimeCreatedService*> _scopedRuntimeServices;
};