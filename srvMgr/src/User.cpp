#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"

User::User(MPlexServer::Client client) : client_(client), authenticated_(false)
{
}

User::~User()
{
}
