#include <framework/DependencyManager.h>
#include <optional_bundles/network_bundle/tcp/TcpConnectionService.h>
#include <optional_bundles/network_bundle/NetworkDataEvent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>

Cppelix::TcpConnectionService::TcpConnectionService(DependencyRegister &reg, CppelixProperties props) : Service(std::move(props)), _socket(-1), _attempts(), _priority(INTERNAL_EVENT_PRIORITY),  _quit(), _listenThread() {
    reg.registerDependency<ILogger>(this, true);
}

bool Cppelix::TcpConnectionService::start() {
    if(getProperties()->contains("Priority")) {
        _priority = std::any_cast<uint64_t>(getProperties()->operator[]("Priority"));
    }


    if(getProperties()->contains("Socket")) {
        _socket = std::any_cast<int>(getProperties()->operator[]("Socket"));

        LOG_TRACE(_logger, "Starting TCP connection for existing socket");
    } else {
        if(!getProperties()->contains("Address")) {
            getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 0, "Missing \"Address\" in properties");
            return false;
        }

        if(!getProperties()->contains("Port")) {
            getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 1, "Missing \"Port\" in properties");
            return false;
        }

        // The start function possibly gets called multiple times due to trying to recover from not being able to connect
        if(_socket == -1) {
            _socket = socket(AF_INET, SOCK_STREAM, 0);
            if (_socket == -1) {
                getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 2, "Couldn't create socket: errno = " + std::to_string(errno));
                return false;
            }
        }

        int setting = 1;
        ::setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, &setting, sizeof(setting));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(std::any_cast<uint16_t>((*getProperties())["Port"]));

        int ret = inet_pton(AF_INET, std::any_cast<std::string>((*getProperties())["Address"]).c_str(), &address.sin_addr);
        if(ret == 0)
        {
            getManager()->pushEvent<UnrecoverableErrorEvent>(getServiceId(), 3, "inet_pton invalid address for given address family (has to be ipv4-valid address)");
            return false;
        }

        if(connect(_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            LOG_ERROR(_logger, "connect error {}", errno);
            if(_attempts < 5) {
                _attempts++;
                getManager()->pushEvent<StartServiceEvent>(getServiceId(), getServiceId());
            }
            return false;
        }

        auto ip = ::inet_ntoa(address.sin_addr);
        LOG_TRACE(_logger, "Starting TCP connection for {}:{}", ip, ::ntohs(address.sin_port));
    }

    _listenThread = std::thread([this] {
        while(!_quit.load(std::memory_order_acquire)) {
            std::array<char, 1024> buf;
            auto ret = recv(_socket, buf.data(), buf.size(), 0);

            if (ret == 0) {
                break;
            }

            if(ret < 0) {
                LOG_ERROR(_logger, "Error receiving from socket: {}", errno);
                getManager()->pushEvent<RecoverableErrorEvent>(getServiceId(), 4, "Error receiving from socket. errno = " + std::to_string(errno));
                continue;
            }

            getManager()->pushPrioritisedEvent<NetworkDataEvent>(getServiceId(), _priority.load(std::memory_order_acquire), std::vector<uint8_t>{buf.data(), buf.data() + ret});
        }
    });

    return true;
}

bool Cppelix::TcpConnectionService::stop() {
    _quit = true;

    if(_socket >= 0) {
        ::shutdown(_socket, SHUT_RDWR);
        ::close(_socket);
    }

    _listenThread.join();

    return true;
}

void Cppelix::TcpConnectionService::addDependencyInstance(ILogger *logger) {
    _logger = logger;
    LOG_TRACE(_logger, "Inserted logger");
}

void Cppelix::TcpConnectionService::removeDependencyInstance(ILogger *logger) {
    _logger = nullptr;
}

void Cppelix::TcpConnectionService::send(std::vector<uint8_t> &&msg) {
    size_t sent_bytes = 0;
    while(sent_bytes < msg.size()) {
        auto ret = ::send(_socket, msg.data() + sent_bytes, msg.size() - sent_bytes, 0);

        if(ret == -1) {
            throw std::runtime_error(fmt::format("Error sending: {}", errno));
        }

        sent_bytes += ret;
    }
}

void Cppelix::TcpConnectionService::set_priority(uint64_t priority) {
    _priority.store(priority, std::memory_order_release);
}

uint64_t Cppelix::TcpConnectionService::get_priority() {
    return _priority.load(std::memory_order_acquire);
}
