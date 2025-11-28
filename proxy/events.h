#pragma once
#include "enet/include/enet.h"
#include <string>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <thread>
#include "proton/vector.hpp"
#include "proton/variant.hpp"
#include <mutex>
#include <queue>
#include "world.h"

//return value: true - dont send original packet, false - send original packet
namespace events {

    namespace out {
        bool variantlist(gameupdatepacket_t* packet);
        bool pingreply(gameupdatepacket_t* packet);
        bool worldoptions(std::string option);
        bool generictext(std::string packet);
        bool gamemessage(std::string packet);
        bool state(gameupdatepacket_t* packet);
        bool tile_change_request(gameupdatepacket_t* packet);
        int get_punch_id(const int id_);

    }; // namespace out
    namespace in {
        bool variantlist(gameupdatepacket_t* packet);
        bool generictext(std::string packet);
        bool gamemessage(std::string packet);
        bool sendmapdata(gameupdatepacket_t* packet);
        bool state(gameupdatepacket_t* packet);
        bool tracking(std::string packet);
        bool OnChangeObject(gameupdatepacket_t* packet);
        bool onTileChangeRequest(gameupdatepacket_t* packet);
    }; // namespace in
};     // namespace events
float hair = 0.f, shirt = 0.f, pants = 0.f;
float shoe = 0.f, face = 0.f, hand = 0.f;
float back = 0.f, mask = 0.f, neck = 0.f;
float ances = 0.f;

bool iswear = false;
bool doublejump = false;
int punch_effect = 8421376;
bool noclip = false;
bool wrench = false;
std::string mode = "`9Pull";
std::string bc = "0";
bool auto_farm = false;
bool auto_stop_farm = false;
bool spam_detect = false;
int x_pos1 = -1, x_pos2 = -1, x_pos3 = -1, x_pos4 = -1, x_posback = -1;
int y_pos1 = -1, y_pos2 = -1, y_pos3 = -1, y_pos4 = -1, y_posback = -1;

bool fastdrop = false;
bool fasttrash = false;

bool superpunch = false;
bool autocollect = false;

std::string title;

bool spin_checker = false;

bool autobgl = false;

bool spam = false;
std::string spam_text = "! Ra#1718";
std::string spam_delay = "4000";

bool savetitle = false;
bool saveset = false;
int farm_id;
bool punch_roulette = false;

bool hidden_r = false;

bool owner_detect = false;
vector<int> owner_netid;
char owner_name[76];

bool     reme_mode = false;
bool	legit = false;
bool    succeed = false;
int setnumber = 6, lampx = 0, lampy = 0, discx = 0, discy = 0, delay = 250, lampxq = 0, lampyq = 0, delayq = 250, setnumberq = 6, bjtilex = 0, bjtiley = 0;
bool qeme_mode = false;
bool succeedq = false;

bool dancemove = false;
int gems = 0;

bool gems_accumulating = false;
int gems_accumulated_count = 0;
bool bj = false;

bool pathmarkerexploit = false;
bool pathmarkerexploitlocalworld;
std::string pathmarkertxt = "hiLol";

int pathfinder_blocks_per_teleport = 8;
int pathfinder_delay_ms = 2;
bool pathfinder_effect_enabled = false;

bool anticollect = false;
std::string country_v;
bool show_ping = true;

bool GetNetid = false;
int GetNetids = -1;
bool GetUid = false;
int GetUids = -1;
bool extractexploit = false;
bool extractready = false;
int target_id = -1;


bool duct_tape = false;

bool is_visual = false;
bool ssup = false;
bool speed = false;

bool bruteforcepassdoor = false;

bool placetp = true;
int placex = 0;
int placey = 0;
bool devmode = true;

std::string last_world;
std::string prev_world;
std::string textcolor = "";
bool textcolorenable = false;
int authfail = -1; // in queue
int role_id = -1;

bool block_wrench = false;
std::string wrench_dialog = "0";
std::string wrench_block = "0";

