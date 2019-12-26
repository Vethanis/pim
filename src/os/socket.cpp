#include "os/socket.h"
#include "components/system.h"

static void Init()
{
    Socket::Init();
}

static void Update()
{

}

static void Shutdown()
{
    Socket::Shutdown();
}

static constexpr System ms_system =
{
    ToGuid("Socket"),
    { 0, 0 },
    Init,
    Update,
    Shutdown,
};

static RegisterSystem ms_register(ms_system);
