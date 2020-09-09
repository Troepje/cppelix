#pragma once

#include <framework/DependencyManager.h>
#include <optional_bundles/logging_bundle/Logger.h>
#include "framework/Service.h"
#include "optional_bundles/serialization_bundle/SerializationAdmin.h"
#include "framework/LifecycleManager.h"
#include "TestMsg.h"

using namespace Cppelix;


struct ITestService : virtual public IService {
    static constexpr InterfaceVersion version = InterfaceVersion{1, 0, 0};
};

class TestService final : public ITestService, public Service {
public:
    TestService(DependencyRegister &reg, CppelixProperties props) : Service(std::move(props)) {
        reg.registerDependency<ILogger>(this, true);
        reg.registerDependency<ISerializationAdmin>(this, true);
    }
    ~TestService() final = default;

    bool start() final {
        LOG_INFO(_logger, "TestService started with dependency");
        _doWorkRegistration = getManager()->registerEventCompletionCallbacks<DoWorkEvent>(getServiceId(), this);
        getManager()->pushEvent<DoWorkEvent>(getServiceId());
        return true;
    }

    bool stop() final {
        LOG_INFO(_logger, "TestService stopped with dependency");
        return true;
    }

    void addDependencyInstance(ILogger *logger) {
        _logger = logger;
        LOG_TRACE(_logger, "Inserted logger");
    }

    void removeDependencyInstance(ILogger *logger) {
        _logger = nullptr;
    }

    void addDependencyInstance(ISerializationAdmin *serializationAdmin) {
        _serializationAdmin = serializationAdmin;
        LOG_INFO(_logger, "Inserted serializationAdmin");
    }

    void removeDependencyInstance(ISerializationAdmin *serializationAdmin) {
        _serializationAdmin = nullptr;
        LOG_INFO(_logger, "Removed serializationAdmin");
    }

    void handleCompletion(DoWorkEvent const * const evt) {
        TestMsg msg{20, "five hundred"};
        auto res = _serializationAdmin->serialize<TestMsg>(msg);
        auto msg2 = _serializationAdmin->deserialize<TestMsg>(std::move(res));
        if(msg2->id != msg.id || msg2->val != msg.val) {
            LOG_ERROR(_logger, "serde incorrect!");
        } else {
            LOG_ERROR(_logger, "serde correct!");
        }
        getManager()->pushEvent<QuitEvent>(getServiceId());
    }

    void handleError(DoWorkEvent const * const evt) {
        LOG_ERROR(_logger, "Error handling DoWorkEvent");
    }

private:
    ILogger *_logger;
    ISerializationAdmin *_serializationAdmin;
    std::unique_ptr<EventCompletionHandlerRegistration> _doWorkRegistration;
};