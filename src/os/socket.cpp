#include "os/socket.h"
#include "components/system.h"

struct SocketSystem final : ISystem
{
    SocketSystem() : ISystem("Socket") {}

    void Init() final
    {
        Socket::Init();
    }
    void Update() final {}
    void Shutdown() final
    {
        Socket::Shutdown();
    }
};

static SocketSystem ms_system;
