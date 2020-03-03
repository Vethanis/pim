#include "os/socket.h"
#include "components/system.h"

namespace SocketSystem
{
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

    static System ms_system
    {
        "Socket",
        {},
        Init,
        Update,
        Shutdown,
    };
};

