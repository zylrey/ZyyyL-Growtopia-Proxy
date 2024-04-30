#pragma once
#include <string>
#include "proton/variant.hpp"
#include "enet/include/enet.h"
#include "world.h"
#include <queue>
#include "PathFinder.h"
using namespace std;

struct PlayerMovings {
    int packetType;
    int netID;
    float x;
    float y;
    int characterState;
    int plantingTree;
    float XSpeed;
    float YSpeed;
    int punchX;
    int punchY;
    int SecondaryNetID;
};

class server {
private:
    ENetHost* m_proxy_server;
    ENetHost* m_real_server;
    ENetPeer* m_server_peer;
    ENetPeer* m_gt_peer;

    void handle_outgoing();
    void handle_incoming();
    bool connect();

public:
    void disconnectsr(bool reset);
    int itemCount;
    int m_user = 0;
    int m_token = 0;
    std::string m_server = "213.179.209.168";
    int m_port = 17201;
    std::string serverz = m_server;
    int portz = m_port;
    int m_proxyport = 17191;
    std::string ipserver = "127.0.0.1";
    std::string create = "0.0.0.0";
    std::string meta = "NULL";

    world m_world;
    LocalPlayer Local_Player;


    Tile* GetTile(int x, int y);
    
    bool start();
    void quit();
    bool setup_client();
    void redirect_server(variantlist_t& varlist);
    void send(bool client, int32_t type, uint8_t* data, int32_t len);
    void send(bool client, variantlist_t& list, int32_t netid = -1, int32_t delay = 0);
    void send(bool client, std::string packet, int32_t type = 2);
    void punch(int x, int y);
    void enterDoor(bool client, int x, int y);
    void wearItem(int itemid);
    void breakBlock(int x, int y);
    void save(float hair, float shirt, float pants, float shoe, float face, float hand, float back, float mask, float neck, float ances);
    void load(float& hair, float& shirt, float& pants, float& shoe, float& face, float& hand, float& back, float& mask, float& neck, float& ances);
    void Set_Pos(int x, int y);
    void pathFindTo(int x, int y);
    void placeBlock(int itemID, int x, int y);
    void SendPacketRaw(bool client, int a1, void* packetData, size_t packetDataSize, void* a4, int packetFlag);
    void gravity(int netid);
    void speed(int netid);
    void s_w(int netid);
    void addblock(bool client, int tile, int x, int y);
    void sendState(int netid2, bool server);
    void Join_World(std::string worlddname);
    void print_game_update_packet(gameupdatepacket_t* packet);
    BYTE* packPlayerMovings(PlayerMovings* dataStruct);
    void NoclipState(int netid);
    void NoclipState2(int netid);
    void poll();
    vector2_t lastPos{};

    std::vector<std::string> flagDescriptions = {
        "Ghost [0]", //0
        "Double Jump [1]", //1
        "One ring [2]", //2
        "No hand [3]", //3
        "mark of growganoth [4]", //4
        "Half invis [5]",
    };
};

extern server* g_server;
