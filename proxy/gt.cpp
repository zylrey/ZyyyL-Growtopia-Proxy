#include "gt.hpp"
#include "packet.h"
#include "server.h"
#include "utils.h"


bool gt::resolving_uid2 = false;
bool gt::connecting = false;
bool gt::in_game = false;
bool gt::ghost = false;
bool gt::resolving_uid = false;
bool gt::aapbypass = false;
bool gt::noclip = false;
bool gt::noclip2 = false;
int gt::block_id = 0;
std::string gt::macaddr = "16:69:f9:t6:ga:6b" ;
string gt::item_defs_file_name = "./include/item_defs.txt";
bool gt::is_exit = false;

void gt::send_log(std::string text) {
    g_server->send(true, "action|log\nmsg| `b[Proxy Log]`w " + text, NET_MESSAGE_GAME_MESSAGE);
}
