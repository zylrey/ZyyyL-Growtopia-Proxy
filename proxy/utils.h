#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "enet/include/enet.h"
#include "packet.h"
#include "proton/variant.hpp"
#include <mutex>
#include <__msvc_chrono.hpp>
#include "skStr.h"
#include <boost/filesystem.hpp>

using namespace std::chrono;


#define PRINTS(msg, ...) printf("[SERVER]: " msg, ##__VA_ARGS__);
#define PRINTC(msg, ...) printf("[CLIENT]: " msg, ##__VA_ARGS__);
#define MALLOC(type, ...) (type*)(malloc(sizeof(type) __VA_ARGS__))
#define get_packet_type(packet) (packet->dataLength > 3 ? *packet->data : 0)
#define DO_ONCE            \
    ([]() {                \
        static char o = 0; \
        return !o && ++o;  \
    }())
#ifdef _WIN32
#define INLINE __forceinline
#else //for gcc/clang
#define INLINE inline
#endif

namespace utils {
    char* get_text(ENetPacket* packet);
    gameupdatepacket_t* get_struct(ENetPacket* packet);
    int random(int min, int max);
    std::string generate_rid();
    uint32_t hash(uint8_t* str, uint32_t len);
    std::string generate_mac(const std::string& prefix = "02");
    std::string hex_str(unsigned char data);
    std::string random(uint32_t length);
    bool isInside(int circle_x, int circle_y, int rad, int x, int y);
    BYTE* GetStructPointerFromTankPacket(ENetPacket* packet);
    INLINE uint8_t* get_extended(gameupdatepacket_t* packet) {
        return reinterpret_cast<uint8_t*>(&packet->m_data_size);
    };
    bool replace(std::string& str, const std::string& from, const std::string& to);
    bool is_number(const std::string& s);
    std::string ReadFromJson(std::string path, std::string section);
    bool CheckIfJsonKeyExists(std::string path, std::string section);
    bool WriteToJson(std::string path, std::string name, std::string value, bool userpass, std::string name2, std::string value2);
    void getBtwString(std::string oStr, std::string sStr1, std::string sStr2, std::string& rStr);
    std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    uint64_t GetTime();
    std::string trim(const std::string& str);

    bool runAtInterval(time_point<system_clock>& timer, double interval) {
        auto now = system_clock::now();
        auto elapsed = now - timer;

        if (elapsed >= seconds(static_cast<long long>(interval))) {
            timer = now;
            return true;
        }

        return false;
    }

    bool eh;
} // namespace utils
