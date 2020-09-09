#include <framework/DependencyManager.h>
#include <optional_bundles/network_bundle/IConnectionService.h>
#include <optional_bundles/network_bundle/tcp/TcpHostService.h>
#include <optional_bundles/network_bundle/tcp/TcpConnectionService.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

Cppelix::TcpHostService::TcpHostService(DependencyRegister &reg, CppelixProperties props) : Service(std::move(props)), _socket(-1), _bindFd(), _priority(INTERNAL_EVENT_PRIORITY), _quit(), _listenThread() {
    reg.registerDependency<ILogger>(this, true);
}

bool Cppelix::TcpHostService::start() {
    if(getProperties()->contains("Priority")) {
        _priority = std::any_cast<uint64_t>(getProperties()->operator[]("Priority"));
    }

    _socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if(_socket == -1) {
        getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 0, "Couldn't create socket: errno = " + std::to_string(errno));
        return false;
    }

    int setting = 1;
    ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &setting, sizeof(setting));
    ::setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, &setting, sizeof(setting));

    sockaddr_in address{};
    address.sin_family = AF_INET;

    auto addressProp = getProperties()->find("Address");

    if(addressProp != end(*getProperties())) {
        auto hostname = std::any_cast<std::string>(addressProp->second);
        if(::inet_aton(hostname.c_str(), &address.sin_addr) != 0) {
            getManager()->pushEvent<RecoverableErrorEvent>(getServiceId(), 1, "inet_aton: errno = " + std::to_string(errno));
            auto hp = ::gethostbyname(hostname.c_str());
            if (hp == nullptr) {
                _socket = -1;
                close(_socket);
                getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 2, "gethostbyname: errno = " + std::to_string(errno));
                return false;
            }

            address.sin_addr = *(struct in_addr *) hp->h_addr;
        }
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }
    address.sin_port = ::htons(std::any_cast<uint16_t>((*getProperties())["Port"]));

    _bindFd = ::bind(_socket, (sockaddr *)&address, sizeof(address));

    if(_bindFd == -1) {
        _socket = -1;
        close(_socket);
        getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 3, "Couldn't bind socket: errno = " + std::to_string(errno));
        return false;
    }

    if(::listen(_socket, 10) != 0) {
        _socket = -1;
        close(_socket);
        getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 4, "Couldn't listen on socket: errno = " + std::to_string(errno));
        return false;
    }

    _listenThread = std::thread([this]{
        sockaddr_in client_addr{};

        while(!_quit.load(std::memory_order_acquire)) {
            socklen_t client_addr_size = sizeof(client_addr);
            int newConnection = ::accept(_socket, (sockaddr *) &client_addr, &client_addr_size);

            if (newConnection == -1) {
                LOG_ERROR(_logger, "New connection but accept() returned {} errno {}", newConnection, errno);
                if(errno == EINVAL) {
                    getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 4, "Accept() generated error. errno = " + std::to_string(errno));
                    break;
                }
                getManager()->pushEvent<RecoverableErrorEvent>(getServiceId(), 4, "Accept() generated error. errno = " + std::to_string(errno));
                continue;
            }

            auto ip = ::inet_ntoa(client_addr.sin_addr);
            LOG_TRACE(_logger, "new connection from {}:{}", ip, ::ntohs(client_addr.sin_port));

            auto connection = getManager()->template createServiceManager<TcpConnectionService, IConnectionService>(CppelixProperties{{"Priority", _priority.load(std::memory_order_acquire)}, {"Socket", newConnection}});

            _connections.emplace_back(connection);
        }
    });

    return true;
}

bool Cppelix::TcpHostService::stop() {
    _quit = true;

    if(_socket >= 0) {
        ::shutdown(_socket, SHUT_RDWR);
        ::close(_socket);
    }

    _listenThread.join();

    return true;
}

void Cppelix::TcpHostService::addDependencyInstance(ILogger *logger) {
    _logger = logger;
    LOG_TRACE(_logger, "Inserted logger");
}

void Cppelix::TcpHostService::removeDependencyInstance(ILogger *logger) {
    _logger = nullptr;
}

void Cppelix::TcpHostService::set_priority(uint64_t priority) {
    _priority.store(priority, std::memory_order_release);
}

uint64_t Cppelix::TcpHostService::get_priority() {
    return _priority.load(std::memory_order_acquire);
}
