#include "utils.h"
#include <algorithm>
#include <chrono>
#include <random>
#include "proton/variant.hpp"
#include <filesystem> 
#include <string> 
#include <fstream>
#include "skStr.h"
#include "json.hpp"
#include "server.h"


using json = nlohmann::json;


BYTE* utils::GetStructPointerFromTankPacket(ENetPacket* packet) {
    unsigned int packetLenght = packet->dataLength;
    BYTE* result = NULL;
    if (packetLenght >= 0x3C) {
        BYTE* packetData = packet->data;
        result = packetData + 4;

        if (*(BYTE*)(packetData + 16) & 8) {
            if (packetLenght < *(int*)(packetData + 56) + 60) {
                result = 0;
            }
        }
        else {
            int zero = 0;
            memcpy(packetData + 56, &zero, 4);
        }
    }
    return result;
}
bool utils::isInside(int circle_x, int circle_y, int rad, int x, int y) {
    // Compare radius of circle with distance
    // of its center from given point
    if ((x - circle_x) * (x - circle_x) + (y - circle_y) * (y - circle_y) <= rad * rad)
        return true;
    else
        return false;
}
char* utils::get_text(ENetPacket* packet) {
    gametankpacket_t* tank = reinterpret_cast<gametankpacket_t*>(packet->data);
    memset(packet->data + packet->dataLength - 1, 0, 1);
    return static_cast<char*>(&tank->m_data);
}
gameupdatepacket_t* utils::get_struct(ENetPacket* packet) {
    if (packet->dataLength < sizeof(gameupdatepacket_t) - 4)
        return nullptr;
    gametankpacket_t* tank = reinterpret_cast<gametankpacket_t*>(packet->data);
    gameupdatepacket_t* gamepacket = reinterpret_cast<gameupdatepacket_t*>(packet->data + 4);
    if (gamepacket->m_packet_flags & 8) {
        if (packet->dataLength < gamepacket->m_data_size + 60) {
            printf("got invalid packet. (too small)\n");
            return nullptr;
        }
        return reinterpret_cast<gameupdatepacket_t*>(&tank->m_data);
    }
    else
        gamepacket->m_data_size = 0;
    return gamepacket;
}
std::mt19937 rng;

int utils::random(int min, int max) {
    if (DO_ONCE)
        rng.seed((unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(rng);
}

std::string utils::generate_rid() {
    std::string rid_str;

    for (int i = 0; i < 16; i++)
        rid_str += utils::hex_str(utils::random(0, 255));

    std::transform(rid_str.begin(), rid_str.end(), rid_str.begin(), ::toupper);

    return rid_str;
}
uint32_t utils::hash(uint8_t* str, uint32_t len) {
    if (!str)
        return 0;
    uint8_t* n = (uint8_t*)str;
    uint32_t acc = 0x55555555;
    if (!len)
        while (*n)
            acc = (acc >> 27) + (acc << 5) + *n++;
    else
        for (uint32_t i = 0; i < len; i++)
            acc = (acc >> 27) + (acc << 5) + *n++;
    return acc;
}
std::string utils::generate_mac(const std::string& prefix) {
    std::string x = prefix + ":";
    for (int i = 0; i < 5; i++) {
        x += utils::hex_str(utils::random(0, 255));
        if (i != 4)
            x += ":";
    }
    return x;
}
const char hexmap_s[17] = "0123456789abcdef";
std::string utils::hex_str(unsigned char data) {
    std::string s(2, ' ');
    s[0] = hexmap_s[(data & 0xF0) >> 4];
    s[1] = hexmap_s[data & 0x0F];
    return s;
}

std::string utils::random(uint32_t length) {
    static auto randchar = []() -> char {
        const char charset[] =
            "0123456789"
            "qwertyuiopasdfghjklzxcvbnm"
            "QWERTYUIOPASDFGHJKLZXCVBNM";
        const uint32_t max_index = (sizeof(charset) - 1);
        return charset[utils::random(INT16_MAX, INT32_MAX) % max_index];
    };

    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}

bool utils::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
bool utils::is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin() + (*s.data() == '-' ? 1 : 0), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

std::string utils::ReadFromJson(std::string path, std::string section)
{
    if (!boost::filesystem::exists(path))
        return skCrypt("File Not Found").decrypt();
    std::ifstream file(path);
    json data = json::parse(file);
    return data[section];
}
bool utils::CheckIfJsonKeyExists(std::string path, std::string section)
{
    if (!boost::filesystem::exists(path))
        return skCrypt("File Not Found").decrypt();
    std::ifstream file(path);
    json data = json::parse(file);
    return data.find(section) != data.end();
}
bool utils::WriteToJson(std::string path, std::string name, std::string value, bool userpass, std::string name2, std::string value2)
{
    json file;
    if (!userpass)
    {
        file[name] = value;
    }
    else
    {
        file[name] = value;
        file[name2] = value2;
    }

    std::ofstream jsonfile(path, std::ios::out);
    jsonfile << file;
    jsonfile.close();
    if (!boost::filesystem::exists(path))
        return false;

    return true;
}

void utils::getBtwString(std::string oStr, std::string sStr1, std::string sStr2, std::string& rStr)
{
    int start = oStr.find(sStr1);
    if (start >= 0)
    {
        std::string tstr = oStr.substr(start + sStr1.length());
        int stop = tstr.find(sStr2);
        if (stop > 1)
            rStr = oStr.substr(start + sStr1.length(), stop);
        else rStr = "error";
    }
    else rStr = "error";
}

std::vector<std::string> utils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> arr;
    int strleng = str.length();
    int delleng = delimiter.length();
    if (delleng == 0) return arr;
    int i = 0;
    int k = 0;
    while (i < strleng) {
        int j = 0;
        while (i + j < strleng && j < delleng && str[i + j] == delimiter[j]) j++;
        if (j == delleng) {
            arr.push_back(str.substr(k, i - k));
            i += delleng;
            k = i;
        }
        else i++;
    }
    arr.push_back(str.substr(k, i - k));
    return arr;
}

uint64_t utils::GetTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string utils::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length() - 1;

    while (start <= end && std::isspace(str[start])) {
        start++;
    }

    while (end > start && std::isspace(str[end])) {
        end--;
    }

    return str.substr(start, end - start + 1);
}



