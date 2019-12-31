#include "os/socket.h"
#include "components/system.h"

static void SocketUpdate() {}

static constexpr System ms_system =
{
    ToGuid("Socket"),
    { 0, 0 },
    Socket::Init,
    SocketUpdate,
    Socket::Shutdown,
};

static RegisterSystem ms_register(ms_system);
