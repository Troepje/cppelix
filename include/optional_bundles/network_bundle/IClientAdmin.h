#pragma once

#include <framework/Service.h>

namespace Cppelix {
    class IClientAdmin : public virtual IService {
    public:
        static constexpr InterfaceVersion version = InterfaceVersion{1, 0, 0};

        ~IClientAdmin() override = default;
    };
}