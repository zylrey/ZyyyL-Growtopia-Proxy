#include "events.h"
#include "dialog.h"
#include "gt.hpp"
#include "proton/hash.hpp"
#include "proton/rtparam.hpp"
#include "proton/variant.hpp"
#include "server.h"
#include <vector>
#include <algorithm>
#include "utils.h"
#include <thread>
#include <limits.h>
#include "HTTPRequest.hpp"
#include <mutex>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "player.h"
#include "world.h"
#include "Discord.h"
#include "discord_webhook.h"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif




namespace {
int GetWorldGemCount() {
    int total_gems = 0;
    for (auto& obj : g_server->m_world.objects) {
        if (obj.second.itemID == 112)
            total_gems += obj.second.count;
    }
    return total_gems;
}

int last_world_gem_total = 0;

std::string GetDialogField(const std::string& packet, const std::string& key) {
    const std::string token = "\n" + key + "|";
    auto pos = packet.find(token);
    if (pos == std::string::npos) {
        return "";
    }
    pos += token.size();
    const auto end_pos = packet.find('\n', pos);
    return packet.substr(pos, end_pos == std::string::npos ? std::string::npos : end_pos - pos);
}

std::string BuildPathfinderDialog() {
    const int stride = std::max(1, pathfinder_blocks_per_teleport);
    const int delay_ms = std::max(0, pathfinder_delay_ms);
    constexpr int kExampleDistance = 200;
    const auto hops_needed = static_cast<int>(std::ceil(static_cast<double>(kExampleDistance) / stride));
    const double seconds_needed = (delay_ms * hops_needed) / 1000.0;

    std::ostringstream travel_time;
    travel_time << std::fixed << std::setprecision(6) << seconds_needed;

    std::string dialog =
        "\nset_default_color|`o"
        "\nadd_label_with_icon|big|`9Pathfinder Options|left|1434|"
        "\nadd_spacer|small"
        "\nadd_textbox|To Enable Pathfind, Type /options and Select `w\"Enable Pathfinder\"``|left|"
        "\nadd_spacer|small"
        "\nadd_textbox|`oBlocks per teleport|left|"
        "\nadd_text_input|p_blocks||" + std::to_string(stride) + "|3|"
        "\nadd_textbox|`oTeleport Delay ms (1000ms = 1 second)|left|"
        "\nadd_text_input|p_delay||" + std::to_string(delay_ms) + "|4|"
        "\nadd_checkbox|p_effect|`oEnable Teleport Effect When Pathfinding|" + std::string(pathfinder_effect_enabled ? "1" : "0") + "|"
        "\nadd_spacer|small"
        "\nadd_textbox|In Theory, Traveling " + std::to_string(stride) + " Blocks per " + std::to_string(delay_ms) +
        "ms Should Travel 200 Blocks In " + travel_time.str() + "sec.|left|"
        "\nend_dialog|pathfinder|Cancel|Okey|";

    return dialog;
}

std::string BuildCommandCatalogDialog() {
    const std::string dialog = R"(
set_default_color|`o
add_label_with_icon|big|`9Proxy Command Catalog|left|6016|
add_textbox|Quick reference for popular host, visual, utility, and trick commands.|left|
add_spacer|small|
add_label_with_icon|small|`wHoster Page|left|32|
add_textbox|`o/save (pos1..pos4) `w- Save player position to numbered slots and warp back with /pos#.|left|
add_textbox|`o/warp1..4 `w- Teleport to saved host spots; /clearpos clears all.|left|
add_textbox|`o/setdrop `w- Choose a custom taxed drop amount and use /drop to place it.|left|
add_textbox|`o/tp `w- Jump between host positions; /gk kicks taxed locks; /ga pulls gas.|left|
add_spacer|small|
add_label_with_icon|small|`wVisual Page|left|6012|
add_textbox|`o/find (item) `w- Locate clothing; /clothes saves look; /maxlevel or /legend|left|
add_textbox|`o/mentor `w- Mentor title; /flag (id) changes country flag; /name (text) recolors name.|left|
add_textbox|`o/pt `w- Build platforms; /weather (id) change weather; /replace swaps dark cave with glass.|left|
add_spacer|small|
add_label_with_icon|small|`wOthers Page|left|6015|
add_textbox|`o/autofarm `w- Start farming; /fps100 boosts fps; /fakeban shows ban notice.|left|
add_textbox|`o/addpull (name) `w- Auto pull/bait; /banall or /world to manage joins; /respawn.|left|
add_textbox|`o/autocollect `w- Open autoclick/autocollect settings; /door (id) warp by door id; /back returns.|left|
add_spacer|small|
add_label_with_icon|small|`wTrick & Utility|left|2982|
add_textbox|`o/gems `w- Show punch-position gem totals; /bj toggles gem accumulation log.|left|
add_textbox|`o/spam `w- Toggle spammer, set text with /stext and delay with /sdelay.|left|
add_textbox|`o/pathfinder `w- Open pathfinder options dialog and auto travel to a tile.|left|
add_textbox|`o/drop (amount/id) `w- Quick custom drops including dls/bgls; /split divides locks.|left|
add_spacer|small|
add_textbox|`9Tip: Use /options for pathfinder settings and /proxy anytime to reopen this list.|left|
end_dialog|proxy|Close||
)");

    return dialog;
}
}

std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    size_t start = 0, end = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        token = str.substr(start, end - start);
        tokens.push_back(token);
        start = end + 1;
    }
    token = str.substr(start);
    tokens.push_back(token);

    return tokens;
}
void do_door_pass() {
    static int StartFrom = 0;
    while (bruteforcepassdoor) {
        static std::chrono::time_point<std::chrono::system_clock> timerlol = std::chrono::system_clock::now();
        if (utils::runAtInterval(timerlol, 0.190)) {
            auto local = g_server->Local_Player;
            if (g_server->m_world.connected) {
                int tileX = local.GetPos().m_x / 32;
                int tileY = local.GetPos().m_y / 32;
                if (tileX >= 0 && tileX < g_server->m_world.width && tileY >= 0 && tileY < g_server->m_world.height) {
                    Tile* tile = g_server->GetTile(tileX, tileY);
                    if (tile && (tile->header.foreground == 762 || tile->header.foreground == 8260)) {
                        StartFrom++;
                        gameupdatepacket_t packet{ 0 };
                        packet.m_type = PACKET_TILE_ACTIVATE_REQUEST;
                        packet.m_state1 = tileX;
                        packet.m_state2 = tileY;
                        g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));

                        std::string packets = "action|dialog_return\ndialog_name|password_reply\ntilex|" + std::to_string(tileX) + "|\ntiley|" + std::to_string(tileY) +
                            "|\npassword|" + std::to_string(StartFrom);
                        g_server->send(false, packets);
                    }
                    else {
                        bruteforcepassdoor = false;
                        StartFrom = 0;
                    }
                }
            }
        }
    }
}

std::chrono::steady_clock::time_point gems_last_collected_time = std::chrono::steady_clock::now();
void CheckAndSendGemsMessage(int previous_total, int current_total) {
    if (!gems_accumulating) {
        gems_accumulated_count = 0;
        last_world_gem_total = current_total;
        return;
    }

    const auto now = std::chrono::steady_clock::now();

    if (previous_total > current_total) {
        gems_accumulated_count += previous_total - current_total;
        gems_last_collected_time = now;
    }

    last_world_gem_total = current_total;

    const auto idle_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - gems_last_collected_time).count();
    if (gems_accumulated_count > 0 && idle_seconds >= 2) {
        gt::send_log("`9Collected `2+" + std::to_string(gems_accumulated_count) + " `9Gems");
        gems_accumulated_count = 0;
    }
}

void doExtract() {
    auto objmap = g_server->m_world;
    for (auto it = objmap.objects.begin(); it != objmap.objects.end(); ++it) {
        if (it->second.itemID == target_id) {
            std::string packet1 =
                "left\nadd_spacer|small|\nadd_textbox|Press on the icon to extract the item into your inventory.|left|\nadd_spacer|small|\nadd_textbox|`wItem List:|left|\nadd_label_with_icon_button_list|small|`w%s : %s|left|extractOnceObj_|itemID_itemAmount_worldObj|" + std::to_string(it->second.itemID) + ", " + std::to_string(it->second.count) + ", " + std::to_string(it->second.uid) + "\nadd_spacer|small|\nembed_data|tilex|" + std::to_string(it->second.pos.m_x / 32) + "\nembed_data|tiley|" + std::to_string(it->second.pos.m_y / 32) + "\nembed_data|startIndex|0\nembed_data|extractorID|6140\nend_dialog|extractor|";
            std::string packet2 =
                "add_label_with_icon|big|`wExtract-O-Snap``|left|6140|\nadd_spacer|small|\nadd_textbox|`2Use the Extract-O-Snap to pick out the items from the floating items in your world. |" + packet1;
            variantlist_t varlst{ "OnDialogRequest" };
            varlst[1] = packet2;
            g_server->send(true, varlst);
        }
    }
}
int qq_function(int num) {
    return num % 10;
}
int reme_function(int num) {
    int sum = 0;
    while (num > 0) {
        sum = sum + (num % 10);
        num = num / 10;
    }
    return sum;
}
string to_lower(string s) {
    for (char& c : s)
        c = tolower(c);
    return s;
}
std::vector<int> intToDigits(int num_) {
    std::vector<int> ret;
    string iStr = to_string(num_);

    for (int i = iStr.size() - 1; i >= 0; --i) {
        int units = pow(10, i);
        int digit = num_ / units % 10;
        ret.push_back(digit);
    }

    return ret;
}
static void PunchXY(int delay) {
    if (delay != 0)
        Sleep(delay); // ghetto remove
    g_server->breakBlock(lampx, lampy);
    variantlist_t varlst{ "OnParticleEffect" };
    varlst[1] = 90;
    varlst[2] = vector2_t{ static_cast<float>(lampx) + 10, static_cast<float>(lampy) + 15 };
    varlst[3] = 0;
    varlst[4] = 0;
    g_server->send(true, varlst);
    if (succeed)
        reme_mode = false;
}
static void PunchXY2(int delay) {
    if (delay != 0)
        Sleep(delay); // ghetto remove
    g_server->breakBlock(lampxq, lampyq);
    variantlist_t varlst{ "OnParticleEffect" };
    varlst[1] = 90;
    varlst[2] = vector2_t{ static_cast<float>(lampxq) + 10, static_cast<float>(lampyq) + 15 };
    varlst[3] = 0;
    varlst[4] = 0;
    g_server->send(true, varlst);
    if (succeedq)
        qeme_mode = false;
}
void do_punch_roulette() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(500, 1000);

    for (auto& tile : g_server->m_world.tiles)
    {
        if (tile.second.header.foreground == 758)
        {
            gameupdatepacket_t packet{};
            packet.m_type = PACKET_TILE_CHANGE_REQUEST;
            packet.m_int_data = 6326;
            packet.m_state1 = tile.second.pos.m_x;
            packet.m_state2 = tile.second.pos.m_y;
            packet.m_vec_x = g_server->Local_Player.pos.m_x;
            packet.m_vec_y = g_server->Local_Player.pos.m_y;

            g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));

            auto delay = std::chrono::milliseconds(dist(gen));
            std::this_thread::sleep_for(delay);
        }
    }
}
void spammer() {
    while (spam) {
        const string wordList[8] = { "`2", "`3", "`4", "`#", "`9", "`8", "`c", "`6" };
        string word = wordList[rand() % 8];
        g_server->send(false, "action|input\n|text|" + word + spam_text);
        std::this_thread::sleep_for(std::chrono::milliseconds(stoi(spam_delay)));
    }
}
void title_() {
    Sleep(10);
    if (title == "legend") {
        variantlist_t va{ "OnNameChanged" };
        va[1] = "``" + g_server->Local_Player.name + " of Legend``";
        g_server->send(true, va, g_server->Local_Player.netid, -1);
    }
    if (title == "bluename") {
        variantlist_t va{ "OnCountryState" };
        va[1] = "ae|showGuild|maxLevel";
        g_server->send(true, va, g_server->Local_Player.netid, -1);
    }
    if (title == "dr") {
        variantlist_t va{ "OnCountryState" };
        va[1] = "ae|showGuild|doctor";
        g_server->send(true, va, g_server->Local_Player.netid, -1);
    }
}
void text_color(std::string text) {
    if (text == "white") {
        textcolor = "`w";
    }
    else if (text == "lightcyan") {
        textcolor = "`1";
    }
    else if (text == "brightcyan") {
        textcolor = "`!";
    }
    else if (text == "green") {
        textcolor = "`2";
    }
    else if (text == "paleblue") {
        textcolor = "`3";
    }
    else if (text == "red") {
        textcolor = "`4";
    }
    else if (text == "waveorange") {
        textcolor = "`6";
    }
    else if (text == "lightgrey") {
        textcolor = "`7";
    }
    else if (text == "orange") {
        textcolor = "`8";
    }
    else if (text == "yellow") {
        textcolor = "`9";
    }
    else if (text == "black") {
        textcolor = "`b";
    }
    else if (text == "brightred") {
        textcolor = "`@";
    }
    else if (text == "pinkbubblegum") {
        textcolor = "`#";
    }
    else if (text == "paleyellow") {
        textcolor = "`$";
    }
    else if (text == "verypalepink") {
        textcolor = "`&";
    }
    else if (text == "dreamsicle") {
        textcolor = "`o";
    }
    else if (text == "pink") {
        textcolor = "`p";
    }
    else if (text == "darkblue") {
        textcolor = "`q";
    }
    else if (text == "mediumblue") {
        textcolor = "`e";
    }
    else if (text == "palegreen") {
        textcolor = "`r";
    }
    else if (text == "mediumgreen") {
        textcolor = "`t";
    }
    else if (text == "darkgrey") {
        textcolor = "`a";
    }
    else if (text == "mediumgrey") {
        textcolor = "`s";
    }
    else if (text == "vibrantcyan") {
        textcolor = "`c";
    }
}
void do_auto_collect() {
    if (g_server->m_world.connected) {
        auto pos2f = g_server->Local_Player.GetPos();
        for (const auto& object : g_server->m_world.objects) {
            if (utils::isInside(object.second.pos.m_x, object.second.pos.m_y, 5 * 32, pos2f.m_x, pos2f.m_y)) {
                gameupdatepacket_t packet{ 0 };
                packet.m_vec_x = object.second.pos.m_x;
                packet.m_vec_y = object.second.pos.m_y;
                packet.m_type = 11;
                packet.m_player_flags = -1;
                packet.m_int_data = object.second.uid;
                packet.m_state1 = object.second.pos.m_x + object.second.pos.m_y + 4;

                g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&packet), sizeof(gameupdatepacket_t));
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}
std::string random_string(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetLength = charset.length();

    // Initialize random seed
    std::srand(std::time(nullptr));

    std::string result;
    for (int i = 0; i < length; ++i) {
        result += charset[std::rand() % charsetLength];
    }

    return result;
}
void t_1() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    g_server->pathFindTo(86, 30);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->enterDoor(false, 86, 30);
}
void t_2() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    g_server->pathFindTo(46, 23);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    Tile* current_tile = g_server->GetTile(47, 23);
    if (current_tile != nullptr && current_tile->header.foreground == 0) {
        gt::send_log("Tile is empty");
    }
    else if (current_tile->header.foreground == 2) {
        int amount = 4;
        while (amount != 0) {
            g_server->breakBlock(47, 23);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            amount--;
        }
    }
    else if (current_tile->header.foreground == 3) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        g_server->breakBlock(47, 23);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->placeBlock(2, 47, 23);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    int amount_b = 1;
    int amount = 4;
    while (amount != 0) {
        g_server->breakBlock(47, 23);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        amount_b++;
        amount--;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    do_auto_collect();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->placeBlock(3, 47, 23);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->placeBlock(10672, 47, 23);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->breakBlock(47, 23);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    do_auto_collect();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    g_server->wearItem(48);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    g_server->send(false, "action|quit_to_exit", 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    std::string world = random_string(17) + "2";
    g_server->send(false, "action|join_request\nname|" + world, 3);
}

void doAutofarm() {
    auto& local = g_server->Local_Player;
    bool isLeft = false;
    auto tile = local.GetPos();
    static std::chrono::time_point<std::chrono::system_clock> timerlol = std::chrono::system_clock::now();
    while (auto_farm) {
        static int mfoffsets = isLeft ? -1 : 1;

        if (utils::runAtInterval(timerlol, 0.20)) {
            mfoffsets += isLeft ? -1 : 1;
            if (mfoffsets > 2 || mfoffsets < -2)
                mfoffsets = isLeft ? -1 : 1;
        }

        Tile* tilepog = g_server->GetTile(tile.m_x / 32 + mfoffsets, tile.m_y / 32);

        gameupdatepacket_t legitpacket{ 0 };
        legitpacket.m_type = PACKET_STATE;
        legitpacket.m_vec_x = local.pos.m_x;
        legitpacket.m_vec_y = local.pos.m_y;

        if (tilepog->header.background == 0 && tilepog->header.foreground == 0) {
            gameupdatepacket_t placepacket{ 0 };
            placepacket.m_type = PACKET_TILE_CHANGE_REQUEST;  // packet 3

            placepacket.m_int_data = farm_id;  // magplant remote id
            placepacket.m_vec_x = local.GetPos().m_x;
            placepacket.m_vec_y = local.GetPos().m_y;
            placepacket.m_state1 = tilepog->pos.m_x;
            placepacket.m_state2 = tilepog->pos.m_y;

            legitpacket.m_int_data = farm_id;
            legitpacket.m_state1 = tilepog->pos.m_x;
            legitpacket.m_state2 = tilepog->pos.m_y;
            legitpacket.m_packet_flags = (1 << 5) | (1 << 10) | (1 << 11);
            static std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();

            if (utils::runAtInterval(time, 0.155)) {
                g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&legitpacket), sizeof(gameupdatepacket_t));
                g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&placepacket), sizeof(gameupdatepacket_t));

            }
        }
        else {
            gameupdatepacket_t punchpacket{ 0 };
            punchpacket.m_type = PACKET_TILE_CHANGE_REQUEST;  // packet 3
            punchpacket.m_int_data = 18;                      // punch id
            punchpacket.m_vec_x = local.GetPos().m_x;
            punchpacket.m_vec_y = local.GetPos().m_y;
            punchpacket.m_state1 = tilepog->pos.m_x;
            punchpacket.m_state2 = tilepog->pos.m_y;

            legitpacket.m_int_data = 18;
            legitpacket.m_state1 = tilepog->pos.m_x;
            legitpacket.m_state2 = tilepog->pos.m_y;
            legitpacket.m_packet_flags = 2560;

            static std::chrono::time_point<std::chrono::system_clock> timepunch = std::chrono::system_clock::now();
            if (utils::runAtInterval(timepunch, 0.155)) {
                g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&legitpacket), sizeof(gameupdatepacket_t));
                g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&punchpacket), sizeof(gameupdatepacket_t));
                variantlist_t varlst{ "OnParticleEffect" };
                varlst[1] = 77;
                varlst[2] = vector2_t{ static_cast<float>(tilepog->pos.m_x * 32 + 15), static_cast<float>(tilepog->pos.m_y * 32 + 15) };
                varlst[3] = 0;
                varlst[4] = 0;
                g_server->send(true, varlst);
            }
        }
    }
}

void PlayerRemove(std::string remove) {
    rtvar rtpur = rtvar::parse(remove);
    int netidd = rtpur.get_int("netID");

    auto it = g_server->m_world.players.find(netidd);
    if (it != g_server->m_world.players.end()) {
        g_server->m_world.objects.clear();
        g_server->m_world.players.erase(it);
    }
    else {
        gt::send_log("Error: Player with netID " + std::to_string(netidd) + " not found");
    }
}
bool events::out::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    varlist.serialize_from_mem(utils::get_extended(packet));
    return false;
}
int events::out::get_punch_id(const int id_) {
    switch (id_) {
    case 138: return 1;
    case 366:
    case 1464:
        return 2;
    case 472: return 3;
    case 594:
    case 10130:
    case 5424:
    case 5456:
    case 4136:
    case 10052:
        return 4;
    case 768: return 5;
    case 900:
    case 7760:
    case 9272:
    case 5002:
    case 7758:
        return 6;
    case 910:
    case 4332:
        return 7;
    case 930:
    case 1010:
    case 6382:
        return 8;
    case 1016:
    case 6058:
        return 9;
    case 1204:
    case 9534:
        return 10;
    case 1378: return 11;
    case 1440: return 12;
    case 1484:
    case 5160:
    case 9802:
        return 13;
    case 1512:
    case 1648:
        return 14;
    case 1542: return 15;
    case 1576: return 16;
    case 1676:
    case 7504:
        return 17;
    case 1748:
    case 8006:
    case 8008:
    case 8010:
    case 8012:
        return 19;
    case 1710:
    case 4644:
    case 1714:
    case 1712:
    case 6044:
    case 1570:
        return 18;
    case 1780: return 20;
    case 1782:
    case 5156:
    case 9776:
    case 9782:
    case 9810:
        return 21;
    case 1804:
    case 5194:
    case 9784:
        return 22;
    case 1868:
    case 1998:
        return 23;
    case 1874: return 24;
    case 1946:
    case 2800:
        return 25;
    case 1952:
    case 2854:
        return 26;
    case 1956: return 27;
    case 1960: return 28;
    case 2908:
    case 6312:
    case 9496:
    case 8554:
    case 3162:
    case 9536:
    case 4956:
    case 3466:
    case 4166:
    case 4506:
    case 2952:
    case 9520:
    case 9522:
    case 8440:
    case 3932:
    case 3934:
    case 8732:
    case 3108:
    case 9766:
    case 12368:
        return 29;
    case 1980: return 30;
    case 2066:
    case 4150:
    case 11082:
    case 11080:
    case 11078:
        return 31;
    case 2212:
    case 5174:
    case 5004:
    case 5006:
    case 5008:
        return 32;
    case 2218: return 33;
    case 2220: return 34;
    case 2266: return 35;
    case 2386: return 36;
    case 2388: return 37;
    case 2450:
        return 38;
    case 2476:
    case 4208:
    case 12308:
    case 10336:
    case 9804:
        return 39;
    case 4748:
    case 4294:
        return 40;
    case 2512:
    case 9732:
    case 6338:
        return 41;
    case 2572: return 42;
    case 2592:
    case 9396:
    case 2596:
    case 9548:
    case 9812:
        return 43;
    case 2720: return 44;
    case 2752: return 45;
    case 2754: return 46;
    case 2756: return 47;
    case 2802: return 49;
    case 2866: return 50;
    case 2876: return 51;
    case 2878:
    case 2880:
        return 52;
    case 2906:
    case 4170:
    case 4278:
        return 53;
    case 2886: return 54;
    case 2890: return 55;
    case 2910: return 56;
    case 3066: return 57;
    case 3124: return 58;
    case 3168: return 59;
    case 3214:
    case 9194:
        return 60;
    case 7408:
    case 3238:
        return 61;
    case 3274: return 62;
    case 3300: return 64;
    case 3418: return 65;
    case 3476: return 66;
    case 3596: return 67;
    case 3686: return 68;
    case 3716: return 69;
    case 4290: return 71;
    case 4474: return 72;
    case 4464:
    case 9500:
        return 73;
    case 4746: return 75;
    case 4778:
    case 6026: case 7784:
        return 76;
    case 4996:
    case 3680:
    case 5176:
        return 77;
    case 4840: return 78;
    case 5206: return 79;
    case 5480:
    case 9770:
    case 9772:
        return 80;
    case 6110: return 81;
    case 6308: return 82;
    case 6310: return 83;
    case 6298: return 84;
    case 6756: return 85;
    case 7044: return 86;
    case 6892: return 87;
    case 6966: return 88;
    case 7088:
    case 11020:
        return 89;
    case 7098:
    case 9032:
        return 90;
    case 7192: return 91;
    case 7136:
    case 9738:
        return 92;
    case 3166: return 93;
    case 7216: return 94;
    case 7196:
    case 9340:
        return 95;
    case 7392:
    case 9604:
        return 96;
    case 7384: return 98;
    case 7414: return 99;
    case 7402: return 100;
    case 7424: return 101;
    case 7470: return 102;
    case 7488: return 103;
    case 7586:
    case 7646:
    case 9778:
        return 104;
    case 7650: return 105;
    case 6804:
    case 6358:
        return 106;
    case 7568:
    case 7570:
    case 7572:
    case 7574:
        return 107;
    case 7668: return 108;
    case 7660:
    case 9060:
        return 109;
    case 7584:
        return 110;
    case 7736:
    case 9116:
    case 9118:
    case 7826:
    case 7828:
    case 11440:
    case 11442:
    case 11312:
    case 7830:
    case 7832:
    case 10670:
    case 9120:
    case 9122:
    case 10680:
    case 10626:
    case 10578:
    case 10334:
    case 11380:
    case 11326:
    case 7912:
    case 11298:
    case 10498:
    case 12342:
        return 111;
    case 7836:
    case 7838:
    case 7840:
    case 7842:
        return 112;
    case 7950: return 113;
    case 8002: return 114;
    case 8022: return 116;
    case 8036: return 118;
    case 9348:
    case 8372:
        return 119;
    case 8038: return 120;
    case 8816:
    case 8818:
    case 8820:
    case 8822:
        return 128;
    case 8910: return 129;
    case 8942: return 130;
    case 8944:
    case 5276:
        return 131;
    case 8432:
    case 8434:
    case 8436:
    case 8950:
        return 132;
    case 8946: case 9576: return 133;
    case 8960: return 134;
    case 9006: return 135;
    case 9058: return 136;
    case 9082:
    case 9304:
        return 137;
    case 9066:
        return 138;
    case 9136: return 139;
    case 9138:
        return 140;
    case 9172: return 141;
    case 9254: return 143;
    case 9256: return 144;
    case 9236: return 145;
    case 9342: return 146;
    case 9542: return 147;
    case 9378: return 148;
    case 9376: return 149;
    case 9410: return 150;
    case 9462: return 151;
    case 9606:
        return 152;
    case 9716:
    case 5192:
        return 153;
    case 10048: return 167;
    case 10064: return 168;
    case 10046: return 169;
    case 10050: return 170;
    case 10128: return 171;
    case 10210:
    case 9544:
        return 172;
    case 10330: return 178;
    case 10398: return 179;
    case 10388:
    case 9524:
    case 9598:
        return 180;
    case 10442: return 184;
    case 10506: return 185;
    case 10652: return 188;
    case 10676: return 191;
    case 10694: return 193;
    case 10714: return 194;
    case 10724: return 195;
    case 10722: return 196;
    case 10754: return 197;
    case 10800: return 198;
    case 10888: return 199;
    case 10886:
    case 11308:
        return 200;
    case 10890: return 202;
    case 10922: case 9550: return 203;
    case 10990: return 205;
    case 10998: return 206;
    case 10952: return 207;
    case 11000: return 208;
    case 11006: return 209;
    case 11046: return 210;
    case 11052: return 211;
    case 10960: return 212;
    case 10956:
    case 9774:
        return 213;
    case 10958: return 214;
    case 10954: return 215;
    case 11076: return 216;
    case 11084: return 217;
    case 11118:
    case 9546:
    case 9574:
        return 218;
    case 11120: return 219;
    case 11116: return 220;
    case 11158: return 221;
    case 11162: return 222;
    case 11142: return 223;
    case 11232: return 224;
    case 11140: return 225;
    case 11248:
    case 9596:
    case 9636:
        return 226;
    case 11240: return 227;
    case 11250: return 228;
    case 11284: return 229;
    case 11292: return 231;
    case 11314: return 233;
    case 11316: return 234;
    case 11324: return 235;
    case 11354: return 236;
    case 11760:
    case 11464:
    case 11438:
    case 12230:
    case 11716:
    case 11718:
    case 11674:
    case 11630:
    case 11786:
    case 11872:
    case 11762:
    case 11994:
    case 12172:
    case 12184:
    case 11460:
    case 12014:
    case 12016:
    case 12018:
    case 12020:
    case 12022:
    case 12024:
    case 12246:
    case 12248:
    case 12176:
    case 12242:
    case 11622:
    case 12350:
    case 12300:
    case 12374:
    case 12356:
        return 237;
    case 11814:
    case 12232:
    case 12302:
        return 241;
    case 11548:
    case 11552:
        return 242;
    case 11704:
    case 11706:
        return 243;
    case 12180:
    case 12346:
    case 12344:
        return 244;
    case 11506:
    case 11508:
    case 11562:
    case 11768:
    case 11882:
    case 11720:
    case 11884:
        return 245;
    case 12432:
    case 12434:
        return 246;
    case 11818:
    case 11876:
    case 12000:
    case 12240:
    case 12642:
    case 12644:
        return 248;

    }
    return 0;
}
bool events::out::pingreply(gameupdatepacket_t* packet) {
    //since this is a pointer we do not need to copy memory manually again
    packet->m_vec2_x = 1000.f;  //gravity
    packet->m_vec2_y = 250.f;   //move speed
    packet->m_vec_x = 64.f;     //punch range
    packet->m_vec_y = 64.f;     //build range
    packet->m_jump_amount = 0;  //for example unlim jumps set it to high which causes ban
    packet->m_player_flags = 0; //effect flags. good to have as 0 if using mod noclip, or etc.
    return false;
}

bool find_command(std::string chat, std::string name) {
    bool found = chat.find("/" + name) == 0;
    if (found)
        gt::send_log("`6" + chat);
    return found;
}

bool events::out::generictext(std::string packet) {

    auto& world = g_server->m_world;
    rtvar var = rtvar::parse(packet);
    if (!var.valid())
        return false;
    if (packet.find("dialog_name|pathfinder") != std::string::npos) {
        const auto blocks_raw = GetDialogField(packet, "p_blocks");
        const auto delay_raw = GetDialogField(packet, "p_delay");

        if (!blocks_raw.empty() && utils::is_number(blocks_raw)) {
            pathfinder_blocks_per_teleport = std::max(1, std::stoi(blocks_raw));
        }

        if (!delay_raw.empty() && utils::is_number(delay_raw)) {
            pathfinder_delay_ms = std::max(0, std::stoi(delay_raw));
        }

        pathfinder_effect_enabled = packet.find("p_effect|1") != std::string::npos;

        gt::send_log("`9Pathfinder set to `3" + std::to_string(pathfinder_blocks_per_teleport) + " `9blocks with `3" + std::to_string(pathfinder_delay_ms) + "ms `9delay.");
        return true;
    }
    if (packet.find("buttonClicked|spare_btn_") != -1) {
        std::string iID = packet.substr(packet.find("buttonClicked|spare_btn_") + 24, packet.length() - packet.find("buttonClicked|spare_btn_") - 1);
        int itemID = atoi(iID.c_str());

        if (itemID != 932 && itemID != 934) {
            gameupdatepacket_t xp{ 0 };
            xp.m_type = PACKET_MODIFY_ITEM_INVENTORY;
            xp.m_count = 1;
            xp.m_int_data = itemID;
            g_server->send(true, NET_MESSAGE_GAME_PACKET, (uint8_t*)&xp, sizeof(gameupdatepacket_t));
            auto item = g_server->m_world.itemDataContainer.item_map[itemID];
        }
    }
    if (packet.find("vclothes") != -1) {
        try {
            std::string aaa = packet.substr(packet.find("hes|") + 4, packet.size());
            std::string number = aaa.c_str();
            while (!number.empty() && isspace(number[number.size() - 1]))
                number.erase(number.end() - (76 - 0x4B));
            iswear = stoi(number);
        }
        catch (exception a)
        {
            gt::send_log("`4Critical Error: `2override detected");

        }
    }
    if (packet.find("vmode") != -1) {
        try {
            std::string aaa = packet.substr(packet.find("ode|") + 4, packet.size());
            std::string number = aaa.c_str();
            while (!number.empty() && isspace(number[number.size() - 1]))
                number.erase(number.end() - (76 - 0x4B));
            wrench = stoi(number);
        }
        catch (exception a)
        {
            gt::send_log("`4Critical Error: `2override detected");

        }
    }
    if (packet.find("vblockdialog") != -1) {
        try {
            std::string aaa = packet.substr(packet.find("log|") + 4, packet.size());
            std::string number = aaa.c_str();
            while (!number.empty() && isspace(number[number.size() - 1]))
                number.erase(number.end() - (76 - 0x4B));
            block_wrench = stoi(number);
        }
        catch (exception a)
        {
            gt::send_log("`4Critical Error: `2override detected");

        }
    }
    if (wrench == true) {
        if (packet.find("action|wrench") != -1) {
            g_server->send(false, packet);

            std::string str = packet.substr(packet.find("netid|") + 6, packet.length() - packet.find("netid|") - 1);
            std::string gta5 = str.substr(0, str.find("|"));
            if (mode == "`9Pull") {
                g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + gta5 + "|\nnetID|" + gta5 + "|\nbuttonClicked|pull");
            }
            if (mode == "`9Kick") {
                g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + gta5 + "|\nnetID|" + gta5 + "|\nbuttonClicked|kick");
            }
            if (mode == "`9Ban") {
                g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + gta5 + "|\nnetID|" + gta5 + "|\nbuttonClicked|worldban");
            }
            variantlist_t varlist{ "OnTextOverlay" };
            varlist[1] = "Successfuly " + mode + "`` netID: " + gta5;
            g_server->send(true, varlist);
            return true;
        }
    }
    if (packet.find("buttonClicked|Pull") != -1) {
        mode = "`9Pull";

    }
    if (packet.find("buttonClicked|Ban") != -1) {

        mode = "`9Ban";

    }
    if (packet.find("buttonClicked|Kick") != -1) {

        mode = "`9Kick";
    }
    bool enable_command = true; 
    string reason;

    if (textcolorenable) {
        string text = "action|input\n|text|";
        if (packet.find(text) == 0 && packet != text) { 
            if (packet.find("/") != string::npos) { 
                enable_command = false;
                variantlist_t varlist{ "OnTextOverlay" };
                varlist[1] = "`0Command disabled: `4'/'`0 found in the packet.";
                g_server->send(true, varlist);
            }
            else {
                enable_command = true;
                string text2 = packet.substr(text.size());
                if (packet.find(text + textcolor) == 0) { 
                }
                else {
                    g_server->send(false, "action|input\n|text|" + textcolor + text2);
                    return true;
                }
            }
        }
        else {
            enable_command = false;
        }
    }
    else {
        enable_command = false;
    }
    if (packet.find("buttonClicked|extractOnceObj_") != -1) {
        if (g_server->m_world.connected) {
            std::string numberString = packet.substr(packet.find("extractOnceObj_") + 16);
            int extractObjectNumber = std::stoi(numberString);

            for (const auto& object : g_server->m_world.objects) {
                if (utils::isInside(object.second.pos.m_x, object.second.pos.m_y, 5 * 32, g_server->Local_Player.GetPos().m_x, g_server->Local_Player.GetPos().m_y)) {
                    gameupdatepacket_t packet{ 0 };
                    packet.m_vec_x = object.second.pos.m_x;
                    packet.m_vec_y = object.second.pos.m_y;
                    packet.m_type = 11;
                    packet.m_player_flags = -1;
                    packet.m_int_data = extractObjectNumber;

                    g_server->send(false, NET_MESSAGE_GAME_PACKET, reinterpret_cast<uint8_t*>(&packet), sizeof(gameupdatepacket_t));
                }
            }
        }
    }
    if (packet.find("buttonClicked|wblock") != -1) {
        auto round = [](double n) {
            return n - floor(n) >= 0.5 ? ceil(n) : floor(n);
        };

        std::map<int, std::map<std::string, int>> store1;

        for (auto tile : g_server->m_world.tiles) {
            int id = tile.second.header.foreground;
            if (id != 0) { 
                if (store1[tile.second.header.foreground].empty()) {
                    store1[tile.second.header.foreground] = { {"id", tile.second.header.foreground}, {"qty", 1} };
                }
                else {
                    store1[tile.second.header.foreground]["qty"] += 1;
                }
                if (store1[tile.second.header.background].empty()) {
                    store1[tile.second.header.background] = { {"id", tile.second.header.background}, {"qty", 1} };
                }
                else {
                    store1[tile.second.header.background]["qty"] += 1;
                }
            }
        }

        std::string placed_items = "add_spacer|small|";
        for (auto& tile : store1) {
            int count = round(tile.second["qty"]);
            int idplaced = floor(tile.second["id"]);
            placed_items += "\nadd_label_with_icon|small|`0" + std::to_string(count) + "``|left|" + std::to_string(idplaced);
        }

        std::string paket =
            "set_default_color|`o\nadd_label_with_icon|big|`0World blocks: `0``|left|170\n" + placed_items + "\nadd_quick_exit";
        variantlist_t liste{ "OnDialogRequest" };
        liste[1] = paket;
        g_server->send(true, liste);
        return true;
    }
    if (packet.find("buttonClicked|spare_btn_932") != -1) {
        variantlist_t varlist{ "OnSetCurrentWeather" };
        varlist[1] = 0;
        g_server->send(true, varlist);
        return true;
    }
    if (packet.find("buttonClicked|spare_btn_934") != -1) {
        variantlist_t varlist{ "OnSetCurrentWeather" };
        varlist[1] = 2;
        g_server->send(true, varlist);
        return true;
    }
    if (packet.find("buttonClicked|spare_btn_946") != -1) {
        variantlist_t varlist{ "OnSetCurrentWeather" };
        varlist[1] = 3;
        g_server->send(true, varlist);
        return true;
    }
    if (packet.find("buttonClicked|spare_btn_984") != -1) {
        variantlist_t varlist{ "OnSetCurrentWeather" };
        varlist[1] = 5;
        g_server->send(true, varlist);
        return true;
    }
    if (packet.find("buttonClicked|wfloat") != -1) {
        std::string droppedshit = "add_spacer|small|\nadd_label_with_icon_button_list|small|`w%s : %s|left|findObject_|itemID_itemAmount|";

        int itemid;
        int count = 0;
        std::vector<std::pair<int, int>> ditems;
        std::string itemlist = "";
        auto& objMap = g_server->m_world;

        if (objMap.connected) {
            std::unordered_map<uint32_t, DroppedItem> updatedObjects;

            for (auto& object : objMap.objects) {
                bool found = false;
                for (int i = 0; i < ditems.size(); i++) {
                    if (ditems[i].first == object.second.itemID) {
                        ditems[i].second += object.second.count;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ditems.push_back(std::make_pair(object.second.itemID, object.second.count));
                }
                else {
                    updatedObjects[object.first] = object.second;
                }
            }
            objMap.objects = updatedObjects;

            if (ditems.empty()) {
                gt::send_log("No dropped items found.");
                return false;
            }

            for (int i = 0; i < ditems.size(); i++) {
                itemlist += std::to_string(ditems[i].first) + "," + std::to_string(ditems[i].second) + ",";
            }
            std::string paket = "set_default_color|`o\nadd_label_with_icon|big|`wFloating Items``|left|6016|\n" + droppedshit + itemlist.substr(0, itemlist.size() - 1) + "\nadd_quick_exit|dropped||Back|\n";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        }
        else {
            gt::send_log("Not connected to the world.");
            return false;
        }
    }
    if (var.get(0).m_key == "action" && var.get(0).m_value == "input") {
        if (var.size() < 2)
            return false;
        if (var.get(1).m_values.size() < 2)
            return false;

        if (!world.connected)
            return false;
        auto chat = var.get(1).m_values[1];
        if (find_command(chat, "find ")) {
            if (iswear == true) {
                bc = "1";
            }
            else {
                bc = "0";
            }
            std::string hypercold = chat.substr(6);
            string find_list = "";

            for (int i = 0; i < g_server->m_world.itemDataContainer.item_map.size(); i++) {
                uint32_t item_id = g_server->m_world.itemDataContainer.item_map[i].itemID;
                if (to_lower(g_server->m_world.itemDataContainer.item_map[item_id].name).find(hypercold) != string::npos) {
                    if (g_server->m_world.itemDataContainer.item_map[item_id].name.find(" Seed") != std::string::npos) continue;
                    find_list += "\nadd_label_with_icon_button|small|" + to_string(item_id) + " <-> " + g_server->m_world.itemDataContainer.item_map[item_id].name + "|left|" + to_string(item_id) + "|spare_btn_" + to_string(item_id) + "|noflags|0|0|";
                }
            }

            std::string paket;
            paket =
                "\nadd_label_with_icon|big|Search Results For `2" + hypercold + "|left|6016|"
                "\nadd_textbox|`1Click on item icon to add it to your inventory|left|2480|"
                "\nadd_smalltext|`4Warning: Take off all your clothes to avoid the ban from this feature.)|left|2480|"
                "\nadd_checkbox|vclothes|`2Enable `cVisual Clothes|" +
                bc +
                "|"
                "\n" + find_list + ""
                "\nadd_quick_exit|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);

            return true;
        }
        else if (find_command(chat, "autofarm")) {
            auto_farm = !auto_farm;
            if (auto_farm) {
                gt::send_log("`9Auto Farm is Now Enabled.");
                std::thread auto_farm_thread(doAutofarm);
                auto_farm_thread.detach();
            }
            else {
                gt::send_log("`9Auto Farm is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "fd") || find_command(chat, "fastdrop")) {
            fastdrop = !fastdrop;

            if (fastdrop) {
                gt::send_log("`9Fast Drop mode is Now Enabled.");
            }
            else {
                gt::send_log("`9Fast Drop mode is Now Disabled.");
            }

            return true;
        }
        else if (find_command(chat, "ft") || find_command(chat, "fasttrash"))
        {
            fasttrash = !fasttrash;
            if (fasttrash)
                gt::send_log("`9Fast Trash mode is Now Enabled.");
            else
                gt::send_log("`9Fast Trash mode is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "sdetect")) {
            spam_detect = !spam_detect;
            if (spam_detect) {
                gt::send_log("`9Spam Detect is Now Enabled.");
            }
            else {
                gt::send_log("`9Spam Detect is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "warp"))
        {
            string name = chat.substr(6);
            gt::send_log("`7Warping to " + name);
            g_server->send(false, "action|join_request\nname|" + name + "\ninvitedWorld|0", 3);
            return true;
        }
        else if (find_command(chat, "spos1")) {
            x_pos1 = floor(g_server->Local_Player.pos.m_x / 32);
            y_pos1 = floor(g_server->Local_Player.pos.m_y / 32);
            gt::send_log("pos 1 set to `3" + to_string(y_pos1) + " `9,`3" + to_string(y_pos1));
            return true;
        }
        else if (find_command(chat, "spos2")) {
            x_pos2 = floor(g_server->Local_Player.pos.m_x / 32);
            y_pos2 = floor(g_server->Local_Player.pos.m_y / 32);
            gt::send_log("pos 2 set to `3" + to_string(x_pos2) + " `9,`3" + to_string(y_pos2));
            return true;
        }
        else if (find_command(chat, "sposback")) {
            x_posback = floor(g_server->Local_Player.pos.m_x / 32);
            y_posback = floor(g_server->Local_Player.pos.m_y / 32);
            gt::send_log("host pos set to `3" + to_string(x_posback) + " `9,`3" + to_string(y_posback));
            return true;
        }
        else if (find_command(chat, "take")) {
            if (x_posback == -1) {
                gt::send_log("`4Set Pos Back First. [/sback");
            }
            else {
                g_server->Set_Pos(x_pos2, y_pos2);
                std::this_thread::sleep_for(std::chrono::seconds(1));

                g_server->Set_Pos(x_pos1, y_pos1);
                std::this_thread::sleep_for(std::chrono::seconds(1));

                g_server->Set_Pos(x_posback, y_posback);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            return true;
        }
        if (find_command(chat, "name ")) {
            std::string name = "``" + chat.substr(6) + "``";
            variantlist_t va{ "OnNameChanged" };
            va[1] = name;
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            gt::send_log("name set to: " + name);
            return true;
        }
        else if (find_command(chat, "flag ")) {
            int flag = atoi(chat.substr(6).c_str());
            variantlist_t va{ "OnGuildDataChanged" };
            va[1] = 1;
            va[2] = 2;
            va[3] = flag;
            va[4] = 3;
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            gt::send_log("flag set to item id: " + std::to_string(flag));
            return true;
        }
        else if (find_command(chat, "skin ")) {
            int skin = atoi(chat.substr(5).c_str());
            variantlist_t va{ "OnChangeSkin" };
            va[1] = skin;
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            return true;
        }
        else if (find_command(chat, "country ")) {
            std::string country = chat.substr(9);
            country_v = country;
            gt::send_log("`9Country set to " + country + ", (Relog to game to change it successfully!)");
            variantlist_t list{ "OnReconnect" };
            g_server->send(true, list);
            return true;
        }
        else if (find_command(chat, "superpunch"))
        {
            superpunch = !superpunch;
            if (superpunch)
                gt::send_log("`9Super Punch is Now Enabled.");
            else
                gt::send_log("`9Super Punch is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "c")) {
            CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(do_auto_collect), nullptr, 0, nullptr);
            return true;
        }
        else if (find_command(chat, "spinall")) {
            CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(do_punch_roulette), nullptr, 0, nullptr);
            return true;
        }
        else if (find_command(chat, "fakeban")) {
            variantlist_t varlist{ "OnAddNotification" };
            varlist[1] = "interface/atomic_button.rttex";
            varlist[2] = "Warning from `4System``: You've been `4BANNED`` from `wGrowtopia`` for 730 days";
            varlist[3] = "audio/hub_open.wav";
            gt::send_log("`oReality flickers as you begin to wake up. (`$Ban`o mod added, `$730`o days left)");
            g_server->send(true, varlist);
            return true;
        }
        else if (find_command(chat, "warn")) {
            string warn = chat.substr(6);
            variantlist_t varlist{ "OnAddNotification" };
            varlist[1] = "interface/atomic_button.rttex";
            varlist[2] = warn;
            varlist[3] = "audio/hub_open.wav";
            g_server->send(true, varlist);
            return true;
        }
        else if (find_command(chat, "gems")) {
            int count = 0;
            for (auto& obj : g_server->m_world.objects) {
                if (obj.second.itemID == 112) {
                    count += obj.second.count;
                }
            }
            count = std::floor(count);
            variantlist_t text{ "OnTextOverlay" };
            text[1] = "`9 gems in world is : " + std::to_string(count);
            g_server->send(true, text);
            return true;
        }
        else if (find_command(chat, "pathfinder")) {
            variantlist_t dialog{ "OnDialogRequest" };
            dialog[1] = BuildPathfinderDialog();
            g_server->send(true, dialog);
            return true;
        }
        else if (find_command(chat, "bj")) {
            gems_accumulating = !gems_accumulating;
            if (gems_accumulating) {
                gems_accumulated_count = 0;
                gems_last_collected_time = std::chrono::steady_clock::now();
                last_world_gem_total = GetWorldGemCount();
                gt::send_log("Gems Checker `2Enabled.");
            }
            else {
                gt::send_log("Gems Checker `4Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "wrench")) {
            wrench_dialog = (wrench) ? "1" : "0";
            wrench_block = (block_wrench) ? "1" : "0";
            std::string paket =
                "\nset_default_color|`o"
                "\nadd_label_with_icon|big|Wrench Mode : " + mode +
                "|left|32|"
                "|left|2480|\nadd_spacer|small\n"
                "\nadd_checkbox|vmode|`9Enable Wrench|" +
                wrench_dialog +
                "|"
                "\nadd_checkbox|vblockdialog|`9Block Wrench|" +
                wrench_block +
                "|"
                "\nadd_button|Pull|`9 Set Mode To [ Pull ]``|noflags|0|0|"
                "\nadd_button|Kick|`9 Set Mode To [ Kick ]``|noflags|0|0|"
                "\nadd_button|Ban|`9 Set Mode To [ Ban ]``|noflags|0|0|"
                "\nadd_quick_exit|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        }
        else if (find_command(chat, "bgl")) {
            autobgl = !autobgl;
            if (autobgl)
                gt::send_log("`9Auto Change BGL is Now Enabled.");
            else
                gt::send_log("`9Auto Change BGL is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "world")) {
            std::string paket;
            paket =
                "\nset_default_color|`o"
                "\nadd_label_with_icon|big|`9World Options|left|5770|"
                "\nadd_spacer|small"
                "\nadd_button|killall|`9Kill All``|noflags|0|0|"
                "\nadd_button|banall|`9Ban All``|noflags|0|0|"
                "\nadd_button|pullall|`9Pull All``|noflags|0|0|"
                "\nadd_button|ubaworld|`9Unban World``|noflags|0|0|"
                "\nadd_quick_exit|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        }
        else if (find_command(chat, "gs")) {
            std::string paket;
            paket =
                "\nset_default_color|`o"
                "\nadd_label_with_icon|big|`9Growscan|left|6016|"
                "\nadd_spacer|small|"
                "\nadd_button|wfloat|`9Floating items``|noflags|0|0|"
                "\nadd_button|wblock|`9Word blocks``|noflags|0|0|"
                "\nadd_spacer|small|"
                "\nadd_quick_exit|";
            variantlist_t liste{ "OnDialogRequest" };
            liste[1] = paket;
            g_server->send(true, liste);
            return true;
        }
        else if (find_command(chat, "spam")) {
            spam = !spam;
            if (spam) {
                gt::send_log("`9Auto Spam is Now Enabled.");
                std::thread(spammer).detach();
            }
            else {
                gt::send_log("`9Auto Spam is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "stext ")) {
            std::string sp = chat.substr(6);
            spam_text = sp;
            gt::send_log("`9Spam Text Set To: " + spam_text);
            return true;
        }
        else if (find_command(chat, "sdelay ")) {
            std::string set_spam_delay_str = chat.substr(8);
            try {
                int set_spam_delay = std::stoi(set_spam_delay_str);
                spam_delay = std::to_string(set_spam_delay);
                gt::send_log("`9spam delay set to: `3" + spam_delay + " `9Seconds");
            }
            catch (const std::exception& e) {
                gt::send_log("`9Invalid spam delay value!");
            }
            return true;
        }
        else if (find_command(chat, "bluename"))
        {
            title = "bluename";
            std::string pkt = "ae|showGuild|maxLevel";
            variantlist_t agaa{ "OnCountryState" };
            agaa[1] = pkt;
            g_server->send(true, agaa, g_server->Local_Player.netid, -1);
            return true;
        }
        else if (find_command(chat, "doctor"))
        {
            title = "dr";
            std::string pkt = "ae|showGuild|doctor";
            variantlist_t va{ "OnNameChanged" };
            va[1] = "Dr." + g_server->Local_Player.name;
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            return true;
        }
        else if (find_command(chat, "legend")) {
            title = "legend";
            variantlist_t va{ "OnNameChanged" };
            va[1] = "``" + g_server->Local_Player.name + " of Legend``";
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            return true;
        }
        else if (find_command(chat, "title")) {
            variantlist_t va{ "OnNameChanged" };
            va[1] = "``" + g_server->Local_Player.name;
            g_server->send(true, va, g_server->Local_Player.netid, -1);
            return true;
        }
        else if (find_command(chat, "saveset")) {
            saveset = !saveset;
            if (saveset)
                gt::send_log("`9Clothes is saved.");
            else
                gt::send_log("`9Clothes is not saved.");
            return true;
        }
        else if (find_command(chat, "savetitle")) {
            savetitle = !savetitle;
            if (savetitle)
                gt::send_log("`9Title is saved.");
            else
                gt::send_log("`9Title is not saved.");
            return true;
        }
        else if (find_command(chat, "farmid ")) {
            std::string number_str = chat.substr(7); // Adjust the substring start index
            number_str = utils::trim(number_str); // Trim whitespace characters

            if (number_str.empty()) {
                gt::send_log("`9No number specified!");
            }
            else {
                bool validNumber = true;
                for (char c : number_str) {
                    if (!isdigit(c)) {
                        validNumber = false;
                        break;
                    }
                }

                if (validNumber) {
                    try {
                        farm_id = std::stoi(number_str);
                        gt::send_log("`9Number set to: `3" + std::to_string(farm_id));
                    }
                    catch (const std::exception& e) {
                        gt::send_log("`9Invalid number!");
                    }
                }
                else {
                    gt::send_log("`9Invalid number!");
                }
            }
            return true;
        }
        else if (find_command(chat, "sfarm")) {
            auto_stop_farm = !auto_stop_farm;
            if (auto_stop_farm)
                gt::send_log("`9Auto Stop Farm is Now Enabled.");
            else
                gt::send_log("`9Auto Stop Farm is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "reme")) {
            reme_mode = !reme_mode;
            if (reme_mode)
                gt::send_log("`9Reme Trick is Now Enabled.");
            else
                gt::send_log("`9Reme Trick is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "rpos")) {
            lampx = floor(g_server->Local_Player.pos.m_x / 32);
            lampy = floor(g_server->Local_Player.pos.m_y / 32);
            gt::send_log("pos set to `3" + to_string(lampx) + " `9,`3" + to_string(lampy));
            return true;
        }
        else if (find_command(chat, "rnumber ")) {
            std::string number_str = chat.substr(9);
            if (number_str.empty()) {
                gt::send_log("`9No number specified!");
            }
            else {
                bool validNumber = true;
                for (char c : number_str) {
                    if (!isdigit(c)) {
                        validNumber = false;
                        break;
                    }
                }

                if (validNumber) {
                    try {
                        setnumber = std::stoi(number_str);
                        gt::send_log("`9Number set to: `3" + std::to_string(setnumber));
                    }
                    catch (const std::exception& e) {
                        gt::send_log("`9Invalid number!");
                    }
                }
                else {
                    gt::send_log("`9Invalid number!");
                }
            }
            return true;
        }
        else if (find_command(chat, "rcm")) {
            succeed = !succeed;
            if (succeed)
                gt::send_log("`9Close Reme Mode When Succeed is Now Enabled.");
            else
                gt::send_log("`9Close Reme Mode When Succeed is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "qeme")) {
            qeme_mode = !qeme_mode;
            if (qeme_mode)
                gt::send_log("`9Qeme Trick is Now Enabled.");
            else
                gt::send_log("`9Qeme Trick is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "qpos")) {
            lampxq = floor(g_server->Local_Player.pos.m_x / 32);
            lampyq = floor(g_server->Local_Player.pos.m_y / 32);
            gt::send_log("pos set to `3" + to_string(lampxq) + " `9,`3" + to_string(lampyq));
            return true;
        }
        else if (find_command(chat, "qnumber ")) {
            std::string number_str = chat.substr(9);
            if (number_str.empty()) {
                gt::send_log("`9No number specified!");
            }
            else {
                bool validNumber = true;
                for (char c : number_str) {
                    if (!isdigit(c)) {
                        validNumber = false;
                        break;
                    }
                }

                if (validNumber) {
                    try {
                        setnumberq = std::stoi(number_str);
                        gt::send_log("`9Number set to: `3" + std::to_string(setnumberq));
                    }
                    catch (const std::exception& e) {
                        gt::send_log("`9Invalid number!");
                    }
                }
                else {
                    gt::send_log("`9Invalid number!");
                }
            }
            return true;
        }
        else if (find_command(chat, "qcm")) {
            succeedq = !succeedq;
            if (succeedq)
                gt::send_log("`9Close Qeme Mode When Succeed is Now Enabled.");
            else
                gt::send_log("`9Close Qeme Mode When Succeed is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "dmove")) {
            dancemove = !dancemove;
            if (dancemove)
                gt::send_log("`9Dance Move is Now Enabled.");
            else
                gt::send_log("`9Dance Move is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "npc")) {
            std::string name = "``" + chat.substr(4) + "``";  // Adjusted substring extraction
            std::string packet_1;
            std::string packet_2;
            std::string packet_3;
            std::string packet_4;
            packet_1 =
                "spawn|avatar"
                "\nnetID|9999"
                "\nuserID|99999"
                "\ncolrect|0|0|20|30";
            packet_2 =
                "\nposXY|" + std::to_string(g_server->Local_Player.pos.m_x) + "|" + std::to_string(g_server->Local_Player.pos.m_y);
            packet_3 =
                "\nname|" + name +
                "\ncountry|" + g_server->Local_Player.country;
            packet_4 =
                "\ninvis|0"
                "\nmstate|0"
                "\nsmstate|0"
                "\nonlineID|"
                "\ntype|";
            variantlist_t varlst2{ "OnSpawn" };
            varlst2[1] = packet_1 + packet_2 + packet_3 + packet_4;
            g_server->send(true, varlst2);
            variantlist_t varlst{ "OnParticleEffect" };
            varlst[1] = 90;
            varlst[2] = vector2_t{ static_cast<float>(g_server->Local_Player.pos.m_x) + 10, static_cast<float>(g_server->Local_Player.pos.m_y) + 15 };
            varlst[3] = 0;
            varlst[4] = 0;
            g_server->send(true, varlst);
            return true;
        }
        else if (find_command(chat, "invis")) {
            std::string packetC = "action|setDeath\nanimDeath|1";
            g_server->send(false, packetC);
            gameupdatepacket_t packet{ 0 };
            packet.m_packet_flags = 2308;
            g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));
            dancemove = true;
            return true;
        }
        else if (find_command(chat, "pathm")) {
            pathmarkerexploit = !pathmarkerexploit;
            if (pathmarkerexploit)
                gt::send_log("`9Pathmarker Exploit is Now Enabled.");
            else
                gt::send_log("`9Pathmarker Exploit is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "pathcworld")) {
            pathmarkerexploitlocalworld = !pathmarkerexploitlocalworld;
            if (pathmarkerexploitlocalworld)
                gt::send_log("`9On Current World is Now Enabled.");
            else
                gt::send_log("`9On Current World is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "ese")) {
            extractexploit = !extractexploit;
            if (extractexploit)
                gt::send_log("`9Extract Exploit is Now Enabled.");
            else
                gt::send_log("`9Extract Exploit is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "targetid")) {
            std::string number_str = chat.substr(9);
            number_str = utils::trim(number_str);

            if (number_str.empty()) {
                gt::send_log("`9No number specified!");
            }
            else {
                bool validNumber = true;
                for (char c : number_str) {
                    if (!isdigit(c)) {
                        validNumber = false;
                        break;
                    }
                }

                if (validNumber) {
                    try {
                        target_id = std::stoi(number_str);
                        gt::send_log("`9Item ID set to: `3" + std::to_string(target_id));
                    }
                    catch (const std::exception& e) {
                        gt::send_log("`9Invalid number!");
                        return true;
                    }
                }
                else {
                    gt::send_log("`9Invalid number!");
                    return true;
                }
            }
            return true;
        }
        else if (find_command(chat, "etake")) {
            doExtract();
            extractready = false;
            return true;
        }
        else if (find_command(chat, "fakeres")) {
            gameupdatepacket_t packet{ 0 };
            packet.m_packet_flags = 2308;
            g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));
            return true;
        }
        else if (find_command(chat, "ssup")) {
            ssup = !ssup;
            if (ssup)
            {
                g_server->sendState(g_server->Local_Player.netid, true);
                gt::send_log("`9Super Suporter Mode is Now Enabled.");
            }
            else
            {
                g_server->sendState(g_server->Local_Player.netid, true);
                gt::send_log("`9Super Suporter Mode is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "duct")) {
            duct_tape = !duct_tape;
            if (duct_tape)
            {
                g_server->sendState(g_server->Local_Player.netid, true);
                gt::send_log("`9Duct Tape Mode is Now Enabled.");
            }
            else
            {
                g_server->sendState(g_server->Local_Player.netid, true);
                gt::send_log("`9Duct Tape Mode is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "speed"))
        {
            speed = !speed;
            if (speed)
            {
                g_server->s_w(g_server->Local_Player.netid);
                gt::send_log("`9Speed Mode is Now Enabled.");
            }
            else
            {
                g_server->sendState(g_server->Local_Player.netid, -1);
                gt::send_log("`9Speed Mode is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "bdoor")) {
            bruteforcepassdoor = !bruteforcepassdoor;
            if (bruteforcepassdoor) {
                gt::send_log("`9Brute Force Passdoor is Now Enabled.");
                std::thread(do_door_pass).detach();
            }
            else {
                gt::send_log("`9Brute Force Passdoor is Now Disabled.");
            }
            return true;
        }
        else if (find_command(chat, "back")) {
            if (prev_world == "") {
                return false;
            }
            std::string joinRequestPacket = "action|join_request\nname|" + (prev_world)+"\ninvitedWorld|0";
            g_server->send(false, joinRequestPacket, 3);
            return true;
        }
        else if (find_command(chat, "textcolor")) {
            textcolorenable = !textcolorenable;
            if (textcolorenable)
                gt::send_log("`9Text Color is Now Enabled.");
            else
                gt::send_log("`9Text Color is Now Disabled.");
            return true;
        }
        else if (find_command(chat, "tcolor")) {
            std::string color_arg = chat.substr(8);
            text_color(color_arg);
            return true;
        }
        else if (find_command(chat, "role builder")) {
            variantlist_t vlist{ "OnSetRoleSkinsAndIcons" };
            vlist[1] = 1;
            vlist[2] = 1;
            g_server->send(true, vlist, g_server->Local_Player.netid);
            return true;
        }
        else if (find_command(chat, "role surgeon")) {
            variantlist_t vlist{ "OnSetRoleSkinsAndIcons" };
            vlist[1] = 2;
            vlist[2] = 2;
            g_server->send(true, vlist, g_server->Local_Player.netid);
            return true;
        }
        else if (find_command(chat, "role fisher")) {
            variantlist_t vlist{ "OnSetRoleSkinsAndIcons" };
            vlist[1] = 3;
            vlist[2] = 3;
            g_server->send(true, vlist, g_server->Local_Player.netid);
            return true;
        }
        else if (find_command(chat, "role cook")) {
            variantlist_t vlist{ "OnSetRoleSkinsAndIcons" };
            vlist[1] = 4;
            vlist[2] = 4;
            g_server->send(true, vlist, g_server->Local_Player.netid);
            return true;
        }
        else if (find_command(chat, "role farmer")) {
            variantlist_t vlist{ "OnSetRoleSkinsAndIcons" };
            vlist[1] = 5;
            vlist[2] = 5;
            g_server->send(true, vlist, g_server->Local_Player.netid);
            return true;
        }
        else if (find_command(chat, "proxy")) {
            variantlist_t dialog{ "OnDialogRequest" };
            dialog[1] = BuildCommandCatalogDialog();
            g_server->send(true, dialog);
            return true;
        }
        return false;
    }
    if (packet.find("game_version|") != -1) {
        rtvar var = rtvar::parse(packet);
        auto mac = utils::generate_mac();
        var.set("mac", mac);
        if (g_server->m_server == "213.179.209.168") {
            rtvar var1;
            using namespace httplib;
            Headers Header;
            Header.insert(std::make_pair("User-Agent", "UbiServices_SDK_2019.Release.27_PC64_unicode_static"));
            Header.insert(std::make_pair("Host", "www.growtopia1.com"));
            Client cli("https://104.125.3.135");
            cli.set_default_headers(Header);
            cli.enable_server_certificate_verification(false);
            cli.set_connection_timeout(2, 0);
            auto res = cli.Post("/growtopia/server_data.php");
            if (res.error() == Error::Success)
                var1 = rtvar::parse({ res->body });
            else
            {
                Client cli("http://api.surferstealer.com");
                auto resSurfer = cli.Get("/system/growtopiaapi?CanAccessBeta=1");
                if (resSurfer.error() == Error::Success)
                    var1 = rtvar::parse({ resSurfer->body });
            }
            g_server->meta = (var1.find("meta") ? var1.get("meta") : (g_server->meta = var1.get("meta")));
        }
        var.set("meta", g_server->meta);

        var.set("country", country_v);
        packet = var.serialize();
        gt::in_game = false;
        PRINTS("Spoofing login info\n");
        g_server->send(false, packet);
        update_rpc("Currently In Game.");
        return true;
    }
    return false;
}

bool events::out::gamemessage(std::string packet) {
    if (packet == "action|quit") {
        g_server->quit();
        return true;
    }

    return false;
}


bool events::out::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;

    g_server->Local_Player.pos = vector2_t{ packet->m_vec_x, packet->m_vec_y };

    if (dancemove) {
        packet->m_jump_amount = 0;
        packet->m_packet_flags = 16 | 8;
        packet->m_vec2_x = 0;
        packet->m_vec2_y = 0;
    }
    if (superpunch) {
        packet->m_packet_flags = 8390688;
        packet->m_state1 = packet->m_state1;
        packet->m_state2 = packet->m_state2;
        packet->m_vec_x = packet->m_vec_x;
        packet->m_vec_y = packet->m_vec_y;
    }
    packet->m_packet_flags &= ~(1 << 11);

    return false;
}

bool events::out::tile_change_request(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;

#ifdef _WIN32
    const bool shift_down = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
#else
    const bool shift_down = false;
#endif

    if (!shift_down)
        return false;

    const int target_x = static_cast<int>(packet->m_state1);
    const int target_y = static_cast<int>(packet->m_state2);

    if (target_x < 0 || target_y < 0 || target_x >= g_server->m_world.width || target_y >= g_server->m_world.height)
        return false;

    g_server->pathFindTo(target_x, target_y);
    return true;
}

bool events::in::variantlist(gameupdatepacket_t* packet) {
    variantlist_t varlist{};
    auto extended = utils::get_extended(packet);
    extended += 4;
    varlist.serialize_from_mem(extended);
    auto& func = varlist[0].get_string();

    if (func.find("OnSuperMainStartAcceptLogon") != -1)
        gt::in_game = true;
    switch (hs::hash32(func.c_str())) {
    case fnv32("OnSetRoleSkinsAndIcons"): {
    } break;
    case fnv32("OnRequestWorldSelectMenu"): {
        auto& world = g_server->m_world;
        world.players.clear();
        world.lastDroppedUid = 0;
        world.objects.clear();
        world.connected = false;
        world.name = "EXIT";
    } break;
    case fnv32("OnMagicCompassTrackingItemIDChanged"): {
        g_server->send(false, "action|setSkin\ncolor|1685231359");
        return true;
    }
    case fnv32("OnChangeSkin"): {
        if (saveset) {
            g_server->sendState(g_server->Local_Player.netid, true);
            variantlist_t liste{ "OnSetClothing" };
            liste[1] = vector3_t{ hair,  shirt,  pants };
            liste[2] = vector3_t{ shoe,  face,  hand };
            liste[3] = vector3_t{ back,  mask,  neck };
            liste[4] = 1685231359;
            liste[5] = vector3_t{ ances , 1.f, 0.f };
            g_server->send(true, liste, g_server->Local_Player.netid, -1);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (savetitle) {
            title_();
            return true;
        }
        break;
    }
    case fnv32("OnSendToServer"): g_server->redirect_server(varlist); return true;

    case fnv32("OnConsoleMessage"): {
        auto& wry = varlist[1].get_string();
        if (wry.find("other people here,") != -1) {
            std::string requested_world = g_server->m_world.name;
            if (requested_world != last_world) {
                prev_world = last_world;
                last_world = requested_world;
            }
            std::transform(last_world.begin(), last_world.end(), last_world.begin(), ::toupper);
            std::transform(prev_world.begin(), prev_world.end(), prev_world.begin(), ::toupper);
            if (!prev_world.empty()) {
                gt::send_log("`9Last Joined World: `3" + prev_world);
            }
        }
        if (wry.find("```wTUTORIAL_1") != -1) {
            CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(t_1), nullptr, 0, nullptr);
        }
        else if (wry.find("```wTUTORIAL_2") != -1) {
            CreateThread(nullptr, 0, LPTHREAD_START_ROUTINE(t_2), nullptr, 0, nullptr);
        }
        if (spam_detect && wry.find("`6>>`4Spam detected!") != -1) {
            spam = false;
        }
        if (wry.find(" Wrench yourself to accept.") != std::string::npos) {
            g_server->send(false, "action|wrench\n|netid|" + std::to_string(g_server->Local_Player.netid));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(g_server->Local_Player.netid) + "|\nbuttonClicked|acceptlock");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            g_server->send(false, "action|dialog_return\ndialog_name|acceptaccess");
        }
        varlist[1] = " `b[ Rbsh#1718 ]`` " + varlist[1].get_string();
        g_server->send(true, varlist);
        return true;
    }break;
    case fnv32("OnTalkBubble"): {
        auto& wry = varlist[2].get_string();
        auto netid = varlist[1].get_int32();
        if (netid == g_server->Local_Player.netid) {
            if (wry.find("spun the wheel and got") != -1) {
                int playerx = g_server->Local_Player.pos.m_x / 32;
                int playery = g_server->Local_Player.pos.m_y / 32;
                std::string growidlocal = g_server->Local_Player.name;

                std::string growid = growidlocal.substr(2).substr(0, growidlocal.length() - 4);
                string replace = wry.substr((35 + growid.length()));

                size_t i = 0;
                for (; i < replace.length(); i++) {
                    if (isdigit(replace[i]))
                        break;
                }
                replace = replace.substr(i, replace.length() - i);
                int idx = atoi(replace.c_str());
                vector<int> total = intToDigits(idx);
                int toplam = 0;
                for (int item : total) {
                    toplam += item;
                }
                int lastdigit = idx % 10;

                int qq_mode = qq_function(idx);
                int reme_mode_2 = reme_function(idx);

                if (reme_mode) {
                    if (toplam >= setnumber && toplam != 11) {
                        if (utils::isInside(lampx, lampy, 4, playerx, playery)) {
                            gt::send_log("`cPunching to `9X:" + to_string(lampx) + " Y:" + to_string(lampy));
                            std::thread a(PunchXY, delay);
                            a.detach();
                        }
                        else {
                            gt::send_log("`4You're not in range of lamp!");
                        }
                    }
                    else if (toplam == 0) {
                        if (utils::isInside(lampx, lampy, 4, playerx, playery)) {
                            gt::send_log("`cPunching to `9X:" + to_string(lampx) + " Y:" + to_string(lampy));
                            std::thread a(PunchXY, delay);
                            a.detach();
                        }
                        else {
                            gt::send_log("`4You're not in range of lamp!");
                        }
                    }
                }
                else if (qeme_mode && reme_mode == false) {
                    if (lastdigit >= setnumber || lastdigit == 0) {
                        if (idx != 0) {
                            if (utils::isInside(lampxq, lampyq, 4, playerx, playery)) {
                                gt::send_log("`cPunching to `9X:" + to_string(lampxq) + " Y:" + to_string(lampyq));
                                std::thread a(PunchXY2, delayq);
                                a.detach();
                            }
                            else {
                                gt::send_log("`4You're not in range of lamp!");
                            }
                        }
                    }
                }
                if (packet->m_int_data == 1800) {
                    varlist[2] = "`0[`2REAL`0]`w " + wry + " (`9QQ: " + std::to_string(qq_mode) + "`0) (`9REME: " + std::to_string(reme_mode_2) + "`0)";
                }
                else {
                    varlist[2] = "`0[`4FAKE`0]`w " + wry.substr(wry.find("the wheel and got"));
                }
                g_server->send(true, varlist);
                return true;
            }
            g_server->send(true, varlist);
            return true;
        }
        if (wry.find("`7[```4MWAHAHAHA!! FIRE FIRE FIRE") != -1) {
            g_server->send(false, "action|wrench\n|netid|" + std::to_string(netid));
            g_server->send(false, "action|dialog_return\ndialog_name|popup\nnetID|" + std::to_string(netid) + "|\nnetID|" + std::to_string(netid) + "|\nbuttonClicked|worldban");
            return true;
        }
    } break;
    case fnv32("OnDialogRequest"): {
        auto& content = varlist[1].get_string();
        if (fasttrash) {
            std::string itemid = content.substr(content.find("embed_data|itemID|") + 18, content.length() - content.find("embed_data|itemID|") - 1);
            std::string count = content.substr(content.find("you have ") + 9, content.length() - content.find("you have ") - 1);
            std::string s = count;
            std::string delimiter = ")";
            std::string token = s.substr(0, s.find(delimiter));
            if (content.find("embed_data|itemID|") != -1) {
                if (content.find("Trash") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|trash_item\nitemID|" + itemid + "|\ncount|" + token);
                    return true;
                }
            }
        }
        if (pathmarkerexploit) {
            if (content.find("would you like to write on this") != -1 && pathmarkerexploitlocalworld) {
                for (auto& tile : g_server->m_world.tiles)
                {
                    if (tile.second.header.foreground == 1684)
                    {
                        std::string packet = "action|dialog_return\ndialog_name|sign_edit\ntilex|" + to_string(tile.second.pos.m_x) + "|\ntiley|" + to_string(tile.second.pos.m_y) + "|\nsign_text|" + pathmarkertxt;
                        g_server->send(false, packet);
                        g_server->Join_World(g_server->m_world.name + "|" + pathmarkertxt);
                        return true;
                    }
                }
            }
        }
        if (extractexploit && content.find("set_default_color|`o") != -1)
        {
            if (content.find("left|6140|") != -1)
            {
                extractready = true;
            }
        }
        if (content.find("add_label_with_icon|big|`wThe Growtopia Gazette") != std::string::npos) {
            g_server->send(false, "action|dialog_return\ndialog_name|gazette");
            return true;
        }
        if (autobgl) {
            if (content.find("Dial a number to call somebody in Growtopia.") != std::string::npos) {
                int x = std::stoi(content.substr(content.find("embed_data|tilex|") + 17, content.find_first_of('|', content.find("embed_data|tilex|")) - content.find("embed_data|tilex|") - 17));
                int y = std::stoi(content.substr(content.find("embed_data|tiley|") + 17, content.find_first_of('|', content.find("embed_data|tiley|")) - content.find("embed_data|tiley|") - 17));
                g_server->send(false, "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(x) + "|\ntiley|" + std::to_string(y) + "|\nnum|-2|\ndial|53785");
                return true;
            }
            if (autobgl) {
                if (content.find("embed_data|num|53785") != std::string::npos) {
                    int x = std::stoi(content.substr(content.find("embed_data|tilex|") + 17, content.find_first_of('|', content.find("embed_data|tilex|")) - content.find("embed_data|tilex|") - 17));
                    int y = std::stoi(content.substr(content.find("embed_data|tiley|") + 17, content.find_first_of('|', content.find("embed_data|tiley|")) - content.find("embed_data|tiley|") - 17));
                    g_server->send(false, "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(x) + "|\ntiley|" + std::to_string(y) + "|\nnum|53785|\nbuttonClicked|chc5");
                    return true;
                }
            }
        }
        if (autobgl) {
            if (content.find("Excellent! I'm happy to sell you a Blue Gem Lock in exchange for 100 Diamond Lock") != std::string::npos) {
                int x = std::stoi(content.substr(content.find("embed_data|tilex|") + 17, content.find_first_of('|', content.find("embed_data|tilex|")) - content.find("embed_data|tilex|") - 17));
                int y = std::stoi(content.substr(content.find("embed_data|tiley|") + 17, content.find_first_of('|', content.find("embed_data|tiley|")) - content.find("embed_data|tiley|") - 17));
                g_server->send(false, "action|dialog_return\ndialog_name|phonecall\ntilex|" + std::to_string(x) + "|\ntiley|" + std::to_string(y) + "|\nnum|-34|\nbuttonClicked|chc0");
                return true;
            }
        }
        if (block_wrench) {
            if (content.find("add_button|report_player|`wReport Player``|noflags|0|0|") != std::string::npos) {
                if (content.find("embed_data|netID") != std::string::npos) {
                    return true;
                }
            }
        }
        if (fastdrop) {
            std::string itemid = content.substr(content.find("embed_data|itemID|") + 18, content.length() - content.find("embed_data|itemID|") - 1);
            std::string count = content.substr(content.find("count||") + 7, content.length() - content.find("count||") - 1);
            if (content.find("embed_data|itemID|") != -1) {
                if (content.find("Drop") != -1) {
                    g_server->send(false, "action|dialog_return\ndialog_name|drop_item\nitemID|" + itemid + "|\ncount|" + count);
                    return true;
                }
            }
        }
    }break;
    case fnv32("OnGuildDataChanged"): {
        if (varlist[3].get_int32() == 5956) {
            variantlist_t varlst2{ "OnAddNotification" };
            varlst2[1] = "interface/seth.rttex";
            varlst2[2] = "`#Retard mod detected";
            varlst2[3] = "audio/already_used.wav";
            varlst2[4] = 0;
            g_server->send(true, varlst2);
            gt::send_log("Mododo");
            g_server->send(false, "action|quit_to_exit", 3);
        }
        else if (varlist[3].get_int32() == 276) {
            variantlist_t varlst{ "OnAddNotification" };
            varlst[1] = "interface/seth.rttex";
            varlst[2] = "`4GUARDIAN `7IS IN THIS WORLD";
            varlst[3] = "audio/already_used.wav";
            varlst[4] = 0;
            g_server->send(true, varlst);
            gt::send_log("`4Guardian`7 Entered The World.");
            g_server->send(false, "action|quit_to_exit", 3);
        }
    } break;
    case fnv32("OnRemove"): {
        std::string remove = varlist[1].get_string();
        PlayerRemove(remove);
    }break;
    case fnv32("OnTextOverlay"): {
        auto overlay = varlist.get(1).get_string();
        if (auto_stop_farm) {
            if (overlay.find("You were pulled by") != -1) {
                auto_farm = false;
                g_server->send(false, "action|input\n|text|?");
                auto_stop_farm = false;
            }
        }
    }break;
    case fnv32("OnSpawn"): {
        std::string mem = varlist.get(1).get_string();
        rtvar var = rtvar::parse(mem);
        auto name = var.find("name");
        auto netid = var.find("netID");
        auto onlineid = var.find("onlineID");
        if (name && netid && onlineid) {
            Player ply{};
            ply.mod = false;
            ply.invis = false;
            ply.name = name->m_value;
            ply.country = var.get("country");
            auto pos = var.find("posXY");
            if (pos && pos->m_values.size() >= 2) {
                auto x = atoi(pos->m_values[0].c_str());
                auto y = atoi(pos->m_values[1].c_str());
                ply.pos = vector2_t{ float(x), float(y) };
            }
            ply.userid = var.get_int("userID");
            ply.netid = var.get_int("netID");
            if (mem.find("type|local") != -1) {
                var.find("mstate")->m_values[0] = "1"; //Mod Zoom

                g_server->Local_Player.country = ply.country;
                g_server->Local_Player.invis = ply.invis;
                g_server->Local_Player.mod = ply.mod;
                g_server->Local_Player.name = ply.name;
                g_server->Local_Player.netid = ply.netid;
                g_server->Local_Player.userid = ply.userid;
                g_server->Local_Player.pos = ply.pos;
                g_server->Local_Player.state = 48;
            }
            else g_server->m_world.players[ply.netid] = ply;
            auto str = var.serialize();
            utils::replace(str, "onlineID", "onlineID|");
            varlist[1] = str;
            g_server->send(true, varlist, -1, -1);
            return true;
        }
    } break;
    }
    return false;
}

bool events::in::generictext(std::string packet) {
    return false;
}

bool events::in::gamemessage(std::string packet) {

    if (gt::resolving_uid2) {
        if (packet.find("PERSON IGNORED") != -1) {
            g_server->send(false, "action|dialog_return\ndialog_name|friends_guilds\nbuttonClicked|showfriend");
            g_server->send(false, "action|dialog_return\ndialog_name|friends\nbuttonClicked|friend_all");
        }
        else if (packet.find("Nobody is currently online with the name") != -1) {
            gt::resolving_uid2 = false;
            gt::send_log("Target is offline, cant find uid.");
        }
        else if (packet.find("Clever perhaps") != -1) {
            gt::resolving_uid2 = false;
            gt::send_log("Target is a moderator, can't ignore them.");
        }
    }
    return false;
}

void mapDataThread(gameupdatepacket_t* packet) {
    g_server->m_world.objects.clear();
    g_server->m_world.LoadFromMem(packet);
}

bool events::in::sendmapdata(gameupdatepacket_t* packet) {
    try {
        thread thr(mapDataThread, packet);
        thr.detach();
    }
    catch (exception e) {

    }
    g_server->m_world.connected = true;
    return false;
}



bool events::in::state(gameupdatepacket_t* packet) {
    if (!g_server->m_world.connected)
        return false;

    if (packet->m_player_flags == -1)
        return false;

    for (int i = 0; i < g_server->m_world.players.size(); i++) {
        if (g_server->m_world.players[i].netid == packet->m_player_flags) {
            g_server->m_world.players[i].pos.m_x = packet->m_vec_x;
            g_server->m_world.players[i].pos.m_y = packet->m_vec_y;
            break;
        }
    }
    return false;
}


enum {
    AUTH_SUCCESS = 0,
    AUTH_BANNED = 1,
    AUTH_SUSPENDED = 2,
    AUTH_OLDDATE = 3,
    AUTH_MAX_ATTEMPTS = 4,
    AUTH_INVALID_ACC = 5,
    AUTH_RELOG = 6,
};

bool events::in::tracking(std::string packet) {
    rtvar var = rtvar::parse(packet);
    std::string event = var.get("eventName");

    if (event == "102_PLAYER.AUTHENTICATION") {
        int autherror = var.get_int("Authentication_error");
        int auth = var.get_int("Authenticated");

        if (autherror == 0 && auth == 1) {
        }
        else {
            if (autherror == 14) { // Suspended
                authfail = AUTH_SUSPENDED;
                gt::send_log("`4Suspended");
            }
            else if (autherror == 15) { // Banned
                authfail = AUTH_BANNED;
                gt::send_log("Banned.");
            }
            else if (autherror == 28) { // Old version
                authfail = AUTH_OLDDATE;
                gt::send_log("`4Old Version.");
            }
            else if (autherror == 35) { // Wrong password
                authfail = AUTH_INVALID_ACC;
                gt::send_log("`4Wrong Password.");
            }
            else if (autherror == 27) { // Blocked
                authfail = AUTH_MAX_ATTEMPTS;
                gt::send_log("`4Blocked.");
            }
            else {
                // Handle other authentication errors
                authfail = autherror;
            }
        }
    }

    if (var.validate_int("Worldlock_balance")) {
        g_server->Local_Player.wl_balance = var.get_int("Worldlock_balance");
    }
    if (var.validate_int("Gems_balance")) {
        g_server->Local_Player.gems_balance = var.get_int("Gems_balance");
    }
    if (var.validate_int("Level")) {
        g_server->Local_Player.level = var.get_int("Level");
    }
    if (var.validate_int("Awesomeness")) {
        g_server->Local_Player.awesomeness = var.get_int("Awesomeness");
    }

    return true;
}


bool events::in::OnChangeObject(gameupdatepacket_t* packet) {
    const int gem_total_before_change = GetWorldGemCount();

    if (packet->m_player_flags == -1) {
        DroppedItem obj;
        obj.itemID = packet->m_int_data;
        obj.pos.m_x = packet->m_vec_x;
        obj.pos.m_y = packet->m_vec_y;
        obj.count = static_cast<uint8_t>(packet->m_struct_flags);
        obj.flags = packet->m_packet_flags;
        obj.uid = ++g_server->m_world.lastDroppedUid;

        const auto key = HashCoord(obj.pos.m_x, obj.pos.m_y);
        g_server->m_world.objects[key] = obj;
    }
    else if (packet->m_player_flags == -3) {
        for (auto& obj : g_server->m_world.objects) {
            if (obj.second.itemID == packet->m_int_data && obj.second.pos.m_x == packet->m_vec_x && obj.second.pos.m_y == packet->m_vec_y) {
                obj.second.count = static_cast<uint8_t>(packet->m_struct_flags);
                break;
            }
        }
    }
    else if (packet->m_player_flags > 0) {
        for (auto it = g_server->m_world.objects.begin(); it != g_server->m_world.objects.end(); ++it) {
            if (it->second.uid == packet->m_int_data || (it->second.itemID == packet->m_int_data && it->second.pos.m_x == packet->m_vec_x && it->second.pos.m_y == packet->m_vec_y)) {
                if (packet->m_player_flags == g_server->Local_Player.netid && it->second.itemID == 112) {
                    gems += it->second.count;
                }
                g_server->m_world.objects.erase(it);
                break;
            }
        }
    }
    else {
#ifdef _CONSOLE
        std::cout << "object update unhandled netid: " << packet->m_int_data << std::endl;
#endif
    }
    CheckAndSendGemsMessage(gem_total_before_change, GetWorldGemCount());
    return false;
}



bool events::in::onTileChangeRequest(gameupdatepacket_t* packet) {
    auto tile = g_server->m_world.tiles.find(HashCoord(packet->m_state1, packet->m_state2));

    if (packet->m_int_data == 18) {
        if (tile->second.header.foreground != 0) tile->second.header.foreground = 0;
        else tile->second.header.background = 0;
    }
    else {
        auto item = g_server->m_world.itemDataContainer.item_map.find(packet->m_int_data);
        if (item->second.actionType == 18)
            tile->second.header.background = packet->m_int_data;
        else if (item->second.actionType == 19) {
            SeedData* seedData = new SeedData(item->second.growTime);
            seedData->count = packet->m_count;
            tile->second.header.foreground = packet->m_int_data;
            tile->second.header.extraData.reset(seedData);
        }
        else tile->second.header.foreground = packet->m_int_data;

        if (packet->m_player_flags == g_server->Local_Player.netid) {
            auto s_items_ptr = &g_server->Local_Player.inventory.items;
            auto it = s_items_ptr->find(packet->m_int_data);
            if (it != s_items_ptr->end()) {
                if (it->second.count > 1)
                    it->second.count -= 1;
                else s_items_ptr->erase(packet->m_int_data);
            }
        }
    }
    return false;
}