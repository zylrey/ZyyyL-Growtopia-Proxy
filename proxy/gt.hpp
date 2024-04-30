#pragma once
#include <string>
#include "packet.h"
namespace gt {
    extern std::string version;
    extern std::string flag;
    extern std::string macaddr;
    extern int block_id;
    extern int max_dropped_block;
    extern bool resolving_uid2;
    extern bool connecting;
    extern bool aapbypass;
    extern bool resolving_uid;
    extern bool resolving_uid2;
    extern bool in_game;
    extern bool ghost;
    extern bool noclip;
    extern bool noclip2;
    extern std::string item_defs_file_name;
    extern bool is_exit;
    void send_log(std::string text);
    static std::string pulse = "Strong";
    static std::string status = "Awake";
    static std::string operation = "Not sanitized";
    static float temp = 97.30;
    static int incisions = 0;
    static int broken = 0;
    static int shattered = 0;

}
