#include "discord.h"
#include<time.h>
#include<spdlog/spdlog.h>
#include<discord_register.h>




void init_discord() {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    Discord_Initialize("", &handlers, 1, NULL);
}


void update_rpc(const std::string& ingame)
{

    DiscordRichPresence presence;
    memset(&presence, 0, sizeof(presence));

    presence.state = "Licensed Role: Premium Proxy User";

    presence.largeImageKey = "";
    presence.largeImageText = "";
    presence.smallImageKey = "";
    presence.smallImageText = "";
    presence.startTimestamp = time(nullptr);

    presence.details = ingame.c_str();


    Discord_UpdatePresence(&presence);
}

