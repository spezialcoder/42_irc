#include "../../server/include/mplexserver.h"
#include "../include/SrvMgr.h"

User::User(MPlexServer::Client client) : client_(client), is_authenticated_(false)
{
}

User::~User()
{
}
