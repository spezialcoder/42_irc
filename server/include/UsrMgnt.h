#pragma once

#include "./mplexserver.h"

/**
 * @brief Class to manage users and their capabilities
*/
class UsrMgnt : public MPlexServer::EventHandler {
public:
    UsrMgnt(MPlexServer::Server& srv);

    void    onConnect(MPlexServer::Client client) override;
    void    onDisconnect(MPlexServer::Client client) override;
    void    onMessage(MPlexServer::Message msg) override;

private:
    MPlexServer::Server& srv_instance;
};
