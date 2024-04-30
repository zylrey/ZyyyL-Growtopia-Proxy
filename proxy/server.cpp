#include "server.h"
#include <iostream>
#include "events.h"
#include "gt.hpp"
#include "proton/hash.hpp"
#include "proton/rtparam.hpp"
#include "utils.h"
#include <chrono>
#include "print.h"
#include "PathFinder.h"
#include <spdlog/spdlog.h>
#include "PathFinder_2.h"
using namespace std;

void server::Join_World(std::string worlddname) {
    std::string p = "action|join_request\nname|" + worlddname + "\ninvitedWorld|0";
    g_server->send(false, p, 3);
}

int getState() {
    int val = 0;
    val |= gt::noclip ? 1 : 2;
    return val;
}

int getState2() {
    int val = 0;
    val |= gt::noclip2 ? 1 : 2;
    return val;
}

void server::print_game_update_packet(gameupdatepacket_t* packet) {
    printf("game packet type: %d\n", packet->m_type);
    printf("network ID: %d\n", packet->m_netid);
    printf("jump amount: %d\n", packet->m_jump_amount);
    printf("count: %d\n", packet->m_count);
    printf("player flags: %d\n", packet->m_player_flags);
    printf("item: %d\n", packet->m_item);
    printf("packet flags: %d\n", packet->m_packet_flags);
    printf("structure flags: %f\n", packet->m_struct_flags);
    printf("integer data: %d\n", packet->m_int_data);
    printf("vector 1 (x, y): (%f, %f)\n", packet->m_vec_x, packet->m_vec_y);
    printf("vector 2 (x, y): (%f, %f)\n", packet->m_vec2_x, packet->m_vec2_y);
    printf("particle time: %f\n", packet->m_particle_time);
    printf("state 1: %u\n", packet->m_state1);
    printf("state 2: %u\n", packet->m_state2);
    printf("data size: %u\n", packet->m_data_size);
    printf("data: %u\n", packet->m_data);
}


bool collecting = true;
PlayerMovings* unpackRaw(BYTE* data) {
    PlayerMovings* p = new PlayerMovings;
    memcpy(&p->packetType, data, 4);
    memcpy(&p->netID, data + 4, 4);
    memcpy(&p->characterState, data + 12, 4);
    memcpy(&p->plantingTree, data + 20, 4);
    memcpy(&p->x, data + 24, 4);
    memcpy(&p->y, data + 28, 4);
    memcpy(&p->XSpeed, data + 32, 4);
    memcpy(&p->YSpeed, data + 36, 4);
    memcpy(&p->punchX, data + 44, 4);
    memcpy(&p->punchY, data + 48, 4);
    return p;
}
BYTE* server::packPlayerMovings(PlayerMovings* dataStruct) {
    BYTE* data = new BYTE[56];
    for (int i = 0; i < 56; i++) {
        data[i] = 0;
    }
    memcpy(data, &dataStruct->packetType, 4);
    memcpy(data + 4, &dataStruct->netID, 4);
    memcpy(data + 8, &dataStruct->SecondaryNetID, 4);
    memcpy(data + 12, &dataStruct->characterState, 4);
    memcpy(data + 20, &dataStruct->plantingTree, 4);
    memcpy(data + 24, &dataStruct->x, 4);
    memcpy(data + 28, &dataStruct->y, 4);
    memcpy(data + 32, &dataStruct->XSpeed, 4);
    memcpy(data + 36, &dataStruct->YSpeed, 4);
    memcpy(data + 44, &dataStruct->punchX, 4);
    memcpy(data + 48, &dataStruct->punchY, 4);
    return data;
}
void server::NoclipState(int netid) {
    PlayerMovings data;
    data.packetType = 0x14;
    data.characterState = 0; // animation
    data.x = 1000;
    data.y = 100;
    data.punchX = 500;
    data.punchY = 0;
    data.XSpeed = 2000;
    data.YSpeed = 500;
    data.netID = netid;
    data.plantingTree = getState();
    BYTE* raw = packPlayerMovings(&data);
    int var = 0x808000;
    float water = 125.0f;
    memcpy(raw + 1, &var, 3);
    memcpy(raw + 16, &water, 4);
    g_server->SendPacketRaw(true, 4, raw, 56, 0, ENET_PACKET_FLAG_RELIABLE);
}

void server::NoclipState2(int netid) {
    PlayerMovings data;
    data.packetType = 0x14;
    data.characterState = 0; // animation
    data.x = 1000;
    data.y = 100;
    data.punchX = 500;
    data.punchY = 0;
    data.XSpeed = 2000;
    data.YSpeed = 500;
    data.netID = netid;
    data.plantingTree = getState2();
    BYTE* raw = packPlayerMovings(&data);
    int var = 0x808000;
    float water = 125.0f;
    memcpy(raw + 1, &var, 3);
    memcpy(raw + 16, &water, 4);
    g_server->SendPacketRaw(true, 4, raw, 56, 0, ENET_PACKET_FLAG_RELIABLE);
}


void server::gravity(int netid) {
    PlayerMovings data;
    data.packetType = 0x14;
    data.characterState = 0;
    data.x = 1000;
    data.y = 100;
    data.punchX = 500;
    data.punchY = 0;
    data.XSpeed = 2000;
    data.YSpeed = 500;
    data.netID = netid;
    data.plantingTree = 3;
    BYTE* raw = g_server->packPlayerMovings(&data);
    int var = 0x808000;
    float water = 125.0f;
    memcpy(raw + 1, &var, 3);
    memcpy(raw + 16, &water, 4);
    g_server->SendPacketRaw(true, 4, raw, 56, 0, ENET_PACKET_FLAG_RELIABLE);
}

void server::speed(int netid) {
    PlayerMovings data;
    data.packetType = 0x14;
    data.characterState = 0;
    data.x = 1000;
    data.y = 100;
    data.punchX = 500;
    data.punchY = 0;
    data.XSpeed = 2000; // Reduce XSpeed to 2000
    data.YSpeed = 500;
    data.netID = netid;
    data.plantingTree = getState();
    BYTE* raw = g_server->packPlayerMovings(&data);
    int var = 0x808000;
    float water = 125.0f;
    memcpy(raw + 1, &var, 3);
    memcpy(raw + 16, &water, 4);
    g_server->SendPacketRaw(true, 4, raw, 56, 0, ENET_PACKET_FLAG_RELIABLE);
}
void server::s_w(int netid) {
    PlayerMovings data;
    data.packetType = 0x14;
    data.characterState = 0;
    data.x = 1000;
    data.y = 100;
    data.punchX = 500;
    data.punchY = 0;
    data.XSpeed = 2000; // Reduce XSpeed to 2000
    data.YSpeed = 500;
    data.netID = netid;
    data.plantingTree = getState();
    BYTE* raw = g_server->packPlayerMovings(&data);
    int var = 0x808000;
    float water = 125.0f;
    memcpy(raw + 1, &var, 3);
    memcpy(raw + 16, &water, 4);
    g_server->SendPacketRaw(true, 4, raw, 56, 0, ENET_PACKET_FLAG_RELIABLE);
}
void server::addblock(bool client, int tile, int x, int y) {
    PlayerMovings data;
    data.packetType = 0x3;
    data.characterState = 0x0; // animation
    data.x = x;
    data.y = y;
    data.punchX = x;
    data.punchY = y;
    data.XSpeed = 0;
    data.YSpeed = 0;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = tile;
    g_server->SendPacketRaw(true, 4, g_server->packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE); // If client = false then auto ban
}

void server::sendState(int netid2, bool server) {
    int32_t netid = netid2;
    int type, charstate, plantingtree, punchx, punchy;
    float x, y, xspeed, yspeed;
    type = PACKET_SET_CHARACTER_STATE;
    charstate = 0;
    int state = 0;
    state |= noclip << 0; //ghost
    state |= doublejump << 1; //double jump
    state |= false << 1; //spirit form
    state |= false << 1; //hoveration
    state |= false << 1; //aurora
    state |= false << 2; //one ring
    state |= false << 4; //mark of growganoth
    state |= false << 7; //halo
    state |= duct_tape << 13; //duct tape
    state |= false << 15; //lucky
    state |= false << 19; //geiger effect
    state |= false << 20; //spotlight
    state |= ssup << 24; //super suporter
    plantingtree = state;
    punchx = 0;
    punchy = 0;
    x = 1000;//1000//speed
    y = 400.0f;//400
    xspeed = 250.0f;//250

    yspeed = 1000.0f; //gravity 1000

    BYTE* data = new BYTE[56];
    memset(data, 0, 56);
    memcpy(data + 0, &type, 4);
    memcpy(data + 4, &netid, 4);
    memcpy(data + 12, &charstate, 4);
    memcpy(data + 20, &plantingtree, 4);
    memcpy(data + 24, &x, 4);
    memcpy(data + 28, &y, 4);
    memcpy(data + 32, &xspeed, 4);
    memcpy(data + 36, &yspeed, 4);
    memcpy(data + 44, &punchx, 4);
    memcpy(data + 48, &punchy, 4);

    memcpy(data + 1, &punch_effect, 3);

    uint8_t build_range = (false ? -1 : 128);
    uint8_t punch_range = (false ? -1 : 128);
    memcpy(data + 2, &build_range, 1);
    memcpy(data + 3, &punch_range, 1);
    float waterspeed = 200.0f;
    memcpy(data + 16, &waterspeed, 4);
    if (server) {
        g_server->SendPacketRaw(true, 4, data, 56, 0, ENET_PACKET_FLAG_RELIABLE);

    }
    else {
        g_server->SendPacketRaw(false, 4, data, 56, 0, ENET_PACKET_FLAG_RELIABLE);
    }
}



void server::handle_outgoing() {
    ENetEvent evt;
    while (enet_host_service(m_proxy_server, &evt, 0) > 0) {
        m_gt_peer = evt.peer;

        switch (evt.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            if (!this->connect())
                return;
        } break;
        case ENET_EVENT_TYPE_RECEIVE: {
            int packet_type = get_packet_type(evt.packet);
            switch (packet_type) {
            case NET_MESSAGE_GENERIC_TEXT:
                if (events::out::generictext(utils::get_text(evt.packet))) {
                    enet_packet_destroy(evt.packet);
                    return;
                }
                break;
            case NET_MESSAGE_GAME_MESSAGE:
                if (events::out::gamemessage(utils::get_text(evt.packet))) {
                    enet_packet_destroy(evt.packet);
                    return;
                }
                break;
            case NET_MESSAGE_GAME_PACKET: {
                auto packet = utils::get_struct(evt.packet);
                if (!packet)
                    break;

                switch (packet->m_type) {
                case PACKET_STATE:
                    if (events::out::state(packet)) {
                        enet_packet_destroy(evt.packet);
                        return;
                    }
                    break;
                case PACKET_ITEM_ACTIVATE_REQUEST: {
                    bool locksfound = true;
                    if (packet->m_int_data != 1796 && packet->m_int_data != 242 && packet->m_int_data != 7188)
                        locksfound = false;
                    if (iswear && locksfound == false) {
                        auto& world = g_server->m_world;
                        auto type = g_server->m_world.itemDataContainer.item_map[packet->m_int_data];

                        spdlog::info("Activated item name: {}", type.name);

                        if (type.name.find("Ancestral") != -1 || type.name.find("Samille") != -1 || type.name.find("Chakram") != -1) {
                            ances = type.itemID == ances ? 0000.0 : type.itemID;
                        }
                        else {
                            switch (type.clothingType) {
                            case 0:
                                hair = type.itemID == hair ? 0000.0 : type.itemID;
                                break;
                            case 1:
                                shirt = type.itemID == shirt ? 0000.0 : type.itemID;
                                break;
                            case 2:
                                pants = type.itemID == pants ? 0000.0 : type.itemID;
                                break;
                            case 3:
                                shoe = type.itemID == shoe ? 0000.0 : type.itemID;
                                break;
                            case 4:
                                face = type.itemID == face ? 0000.0 : type.itemID;
                                break;
                            case 5:
                                hand = type.itemID == hand ? 0000.0 : type.itemID;
                                break;
                            case 6:
                                back = type.itemID == back ? 0000.0 : type.itemID;
                                break;
                            case 7:
                                hair = type.itemID == hair ? 0000.0 : type.itemID;
                                break;
                            case 8:
                                neck = type.itemID == neck ? 0000.0 : type.itemID;
                                break;
                            default:
                                hair = type.itemID == hair ? 0000.0 : type.itemID;
                                break;
                            }
                        }

                        if (events::out::get_punch_id(packet->m_int_data) != 0) {
                            punch_effect = events::out::get_punch_id(packet->m_int_data);
                        }

                        if (back != 0000.0) {
                            doublejump = true;
                        }

                        sendState(g_server->Local_Player.netid, true);
                        variantlist_t liste{ "OnSetClothing" };
                        liste[1] = vector3_t{ hair,  shirt,  pants };
                        liste[2] = vector3_t{ shoe,  face,  hand };
                        liste[3] = vector3_t{ back,  mask,  neck };
                        liste[4] = 1685231359;
                        liste[5] = vector3_t{ ances , 1.f, 0.f };
                        g_server->send(true, liste, g_server->Local_Player.netid, -1);

                        return;
                    }
                }
                case PACKET_CALL_FUNCTION:
                    if (events::out::variantlist(packet)) {
                        enet_packet_destroy(evt.packet);
                        return;
                    }
                    break;
                case PACKET_PING_REPLY:
                    if (events::out::pingreply(packet)) {
                        enet_packet_destroy(evt.packet);
                        return;
                    }
                    break;
                case PACKET_DISCONNECT:
                case PACKET_APP_INTEGRITY_FAIL:
                    if (gt::in_game)
                        return;
                    break;

                default: break;
                }
            } break;
            case NET_MESSAGE_TRACK: //track one should never be used, but its not bad to have it in case.
            case NET_MESSAGE_CLIENT_LOG_RESPONSE: return;

            default: break;
            }

            if (!m_server_peer || !m_real_server)
                return;
            enet_peer_send(m_server_peer, 0, evt.packet);
            enet_host_flush(m_real_server);
        } break;
        case ENET_EVENT_TYPE_DISCONNECT: {
            if (gt::in_game)
                return;
            if (gt::connecting) {
                this->disconnectsr(false);
                gt::connecting = false;
                return;
            }

        } break;
        default: break;
        }
    }
}

void server::handle_incoming() {
    ENetEvent event;

    while (enet_host_service(m_real_server, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT: break;
        case ENET_EVENT_TYPE_DISCONNECT: this->disconnectsr(true); return;
        case ENET_EVENT_TYPE_RECEIVE: {
            if (event.packet->data) {
                int packet_type = get_packet_type(event.packet);
                switch (packet_type) {
                case NET_MESSAGE_GENERIC_TEXT:
                    if (events::in::generictext(utils::get_text(event.packet))) {
                        enet_packet_destroy(event.packet);
                        return;
                    }
                    break;
                case NET_MESSAGE_GAME_MESSAGE:
                    if (events::in::gamemessage(utils::get_text(event.packet))) {
                        enet_packet_destroy(event.packet);
                        return;
                    }
                    break;
                case NET_MESSAGE_GAME_PACKET: {
                    auto packet = utils::get_struct(event.packet);
                    if (!packet)
                        break;

                    switch (packet->m_type) {
                    case PACKET_SEND_INVENTORY_STATE: {
                        server::Local_Player.inventory.slotCount = 0;
                        server::Local_Player.inventory.itemCount = 0;
                        server::Local_Player.inventory.items.clear();
                        std::vector<Item> invBuf;
                        LPBYTE extended_ptr = utils::get_extended(packet);
                        memcpy(&server::Local_Player.inventory.slotCount, extended_ptr + 5, 4);
                        memcpy(&server::Local_Player.inventory.itemCount, extended_ptr + 9, 2);
                        invBuf.resize(server::Local_Player.inventory.itemCount);
                        memcpy(invBuf.data(), extended_ptr + 11, invBuf.size() * sizeof(Item));
                        for (const Item& item : invBuf)
                            server::Local_Player.inventory.items[item.id] = item;
                    } break;

                    case PACKET_MODIFY_ITEM_INVENTORY: {
                        uint32_t itemID = packet->m_int_data;
                        auto it = g_server->Local_Player.inventory.items.find(itemID);
                        if (it != g_server->Local_Player.inventory.items.end()) {
                            Item& item = it->second;
                            if (item.count > packet->m_jump_amount) {
                                item.count -= packet->m_jump_amount;
                                spdlog::info("Decremented item count by {}: {}", packet->m_jump_amount, itemID);
                            }
                            else {
                                g_server->Local_Player.inventory.items.erase(it);
                                spdlog::info("Erased item from inventory: {}", itemID);
                            }
                        }
                    } break;
                    case PACKET_CALL_FUNCTION:
                        if (events::in::variantlist(packet)) {
                            enet_packet_destroy(event.packet);
                            return;
                        }
                        break;

                    case PACKET_SEND_MAP_DATA:
                        if (events::in::sendmapdata(packet)) {
                            enet_packet_destroy(event.packet);
                            return;
                        }
                        break;
                    case PACKET_TILE_CHANGE_REQUEST:
                        if (events::in::onTileChangeRequest(packet)) {
                            enet_packet_destroy(event.packet);
                            return;
                        }
                        break;
                    case PACKET_ITEM_CHANGE_OBJECT:
                        if (events::in::OnChangeObject(packet)) {
                            enet_packet_destroy(event.packet);
                            return;
                        }
                        break;
                    case PACKET_STATE:
                        if (events::in::state(packet)) {
                            enet_packet_destroy(event.packet);
                            return;
                        }
                        break;
                    default: {}
                    }
                } break;
                case NET_MESSAGE_TRACK:
                    if (events::in::tracking(utils::get_text(event.packet))) {
                        enet_packet_destroy(event.packet);
                        return;
                    }
                    break;
                case NET_MESSAGE_CLIENT_LOG_REQUEST: return;

                default: break;
                }
            }

            if (!m_gt_peer || !m_proxy_server)
                return;
            enet_peer_send(m_gt_peer, 0, event.packet);
            enet_host_flush(m_proxy_server);

        } break;

        default: break;
        }
    }
}

void server::poll() {
    this->handle_outgoing();

    if (!m_real_server)
        return;
    this->handle_incoming();
}

Tile* server::GetTile(int x, int y)
{
    if (x >= 0 && x < g_server->m_world.width && y >= 0 && y < g_server->m_world.height)
    {
        int index = x + g_server->m_world.width * y;
        if (index >= 0 && index < static_cast<int>(g_server->m_world.tiles.size()))
        {
            return &g_server->m_world.tiles[index];
        }
    }
    return nullptr;
}



bool server::start() {
    ENetAddress address;
    enet_address_set_host(&address, "0.0.0.0");
    address.port = m_proxyport;
    m_proxy_server = enet_host_create(&address, 1024, 10, 0, 0);
    m_proxy_server->usingNewPacket = false;

    if (!m_proxy_server) {
        spdlog::info("failed to start the proxy server!\n");
        return false;
    }
    m_proxy_server->checksum = enet_crc32;
    auto code = enet_host_compress_with_range_coder(m_proxy_server);
    if (code != 0)
        spdlog::info("enet host compressing failed\n");
    return setup_client();
}

void server::quit() {
    gt::in_game = false;
    this->disconnectsr(true);
}

bool server::setup_client() {
    m_real_server = enet_host_create(0, 1, 2, 0, 0);
    m_real_server->usingNewPacket = true;
    if (!m_real_server) {
        spdlog::info("failed to start the client\n");
        return false;
    }
    m_real_server->checksum = enet_crc32;
    auto code = enet_host_compress_with_range_coder(m_real_server);
    if (code != 0)
        spdlog::info("enet host compressing failed\n");
    enet_host_flush(m_real_server);
    return true;
}

void server::redirect_server(variantlist_t& varlist) {
    m_port = varlist[1].get_uint32();
    m_token = varlist[2].get_uint32();
    m_user = varlist[3].get_uint32();
    std::string str = varlist[4].get_string();

    std::string doorid = str.substr(str.find("|") + 1);
    m_server = str.substr(0, str.find("|")); // extract server without doorid
    spdlog::info("port: {} token: {} user: {} server: {} doorid: {}", m_port, m_token, m_user, m_server, doorid);

    varlist[1] = m_proxyport;
    varlist[4] = "127.0.0.1|" + doorid;

    gt::connecting = true;
    send(true, varlist);

    if (m_real_server) {
        enet_host_destroy(m_real_server);
        m_real_server = nullptr;
    }
}

void server::disconnectsr(bool reset) {
    m_world.connected = false;
    m_world.players.clear();
    if (m_server_peer) {
        enet_peer_disconnect_now(m_server_peer, 0);
        enet_peer_reset(m_server_peer);
        m_server_peer = nullptr;
        enet_host_destroy(m_real_server);
        m_real_server = nullptr;
    }
    if (reset) {
        m_user = 0;
        m_token = 0;
        m_server = "213.179.209.168";
        m_port = 17198;
    }
}

bool server::connect() {
    spdlog::info("Connecting to server.\n");
    ENetAddress address;
    if (enet_address_set_host(&address, m_server.c_str()) != 0) {
        spdlog::info("Invalid server address: {}\n", m_server);
        return false;
    }
    address.port = m_port;
    spdlog::info("Port: {} Server: {}\n", m_port, m_server);
    if (!this->setup_client()) {
        spdlog::info("Failed to set up client when trying to connect to server!\n");
        return false;
    }
    m_server_peer = enet_host_connect(m_real_server, &address, 2, 0);
    if (!m_server_peer) {
        spdlog::info("Failed to connect to the real server.\n");
        return false;
    }
    return true;
}


void server::SendPacketRaw(bool client, int a1, void* packetData, size_t packetDataSize, void* a4, int packetFlag) {
    ENetPacket* p;
    auto peer = client ? m_gt_peer : m_server_peer;
    if (peer) {
        if (a1 == 4 && *((BYTE*)packetData + 12) & 8) {
            p = enet_packet_create(0, packetDataSize + *((DWORD*)packetData + 13) + 5, packetFlag);
            int four = 4;
            memcpy(p->data, &four, 4);
            memcpy((char*)p->data + 4, packetData, packetDataSize);
            memcpy((char*)p->data + packetDataSize + 4, a4, *((DWORD*)packetData + 13));
            enet_peer_send(peer, 0, p);
        }
        else {
            p = enet_packet_create(0, packetDataSize + 5, packetFlag);
            memcpy(p->data, &a1, 4);
            memcpy((char*)p->data + 4, packetData, packetDataSize);
            enet_peer_send(peer, 0, p);
        }
    }
    delete (char*)packetData;
}

//bool client: true - sends to growtopia client    false - sends to gt server
void server::send(bool client, int32_t type, uint8_t* data, int32_t len) {
    auto peer = client ? m_gt_peer : m_server_peer;
    auto host = client ? m_proxy_server : m_real_server;

    if (!peer || !host)
        return;
    auto packet = enet_packet_create(0, len + 5, ENET_PACKET_FLAG_RELIABLE);
    auto game_packet = (gametextpacket_t*)packet->data;
    game_packet->m_type = type;
    if (data)
        memcpy(&game_packet->m_data, data, len);

    memset(&game_packet->m_data + len, 0, 1);
    int code = enet_peer_send(peer, 0, packet);
    if (code != 0)
        PRINTS("Error sending packet! code: %d\n", code);
    enet_host_flush(host);
}

//bool client: true - sends to growtopia client    false - sends to gt server
void server::send(bool client, variantlist_t& list, int32_t netid, int32_t delay) {
    auto peer = client ? m_gt_peer : m_server_peer;
    auto host = client ? m_proxy_server : m_real_server;

    if (!peer || !host)
        return;

    uint32_t data_size = 0;
    void* data = list.serialize_to_mem(&data_size, nullptr);

    //optionally we wouldnt allocate this much but i dont want to bother looking into it
    auto update_packet = MALLOC(gameupdatepacket_t, +data_size);
    auto game_packet = MALLOC(gametextpacket_t, +sizeof(gameupdatepacket_t) + data_size);

    if (!game_packet || !update_packet)
        return;

    memset(update_packet, 0, sizeof(gameupdatepacket_t) + data_size);
    memset(game_packet, 0, sizeof(gametextpacket_t) + sizeof(gameupdatepacket_t) + data_size);
    game_packet->m_type = NET_MESSAGE_GAME_PACKET;

    update_packet->m_type = PACKET_CALL_FUNCTION;
    update_packet->m_player_flags = netid;
    update_packet->m_packet_flags |= 8;
    update_packet->m_int_data = delay;
    memcpy(&update_packet->m_data, data, data_size);
    update_packet->m_data_size = data_size;
    memcpy(&game_packet->m_data, update_packet, sizeof(gameupdatepacket_t) + data_size);
    free(update_packet);

    auto packet = enet_packet_create(game_packet, data_size + sizeof(gameupdatepacket_t), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
    enet_host_flush(host);
    free(game_packet);
}

void server::send(bool client, std::string text, int32_t type) {
    auto peer = client ? m_gt_peer : m_server_peer;
    auto host = client ? m_proxy_server : m_real_server;

    if (!peer || !host)
        return;
    auto packet = enet_packet_create(0, text.length() + 5, ENET_PACKET_FLAG_RELIABLE);
    auto game_packet = (gametextpacket_t*)packet->data;
    game_packet->m_type = type;
    memcpy(&game_packet->m_data, text.c_str(), text.length());

    memset(&game_packet->m_data + text.length(), 0, 1);
    int code = enet_peer_send(peer, 0, packet);
    if (code != 0)
        PRINTS("Error sending packet! code: %d\n", code);
    enet_host_flush(host);
}


void server::punch(int x, int y) {
    int xx = (g_server->Local_Player.pos.m_x / 32) + x;
    int yy = (g_server->Local_Player.pos.m_y / 32) + y;

    PlayerMovings data{};
    data.packetType = 0x3;
    data.characterState = (int(g_server->Local_Player.pos.m_x / 32) > xx ? UPDATE_PACKET_PUNCH_TILE_LEFT : UPDATE_PACKET_PUNCH_TILE_RIGHT);
    data.x = g_server->Local_Player.pos.m_x;
    data.y = g_server->Local_Player.pos.m_y;
    data.punchX = xx;
    data.punchY = yy;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = 18;
    g_server->SendPacketRaw(false, 4, g_server->packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE);
}


void server::enterDoor(bool client, int x, int y) {
    PlayerMovings data;
    data.packetType = 0x7;
    data.characterState = 0x0; // animation
    data.x = g_server->Local_Player.pos.m_x;
    data.y = g_server->Local_Player.pos.m_y;
    data.punchX = x;
    data.punchY = y;
    data.XSpeed = 0;
    data.YSpeed = 0;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = 18;
    g_server->SendPacketRaw(client, 4, g_server->packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE);
}

void server::wearItem(int itemid) {
    variantlist_t varlist{ "OnEquipNewItem" };
    varlist[1] = itemid;
    g_server->send(false, varlist, g_server->Local_Player.netid, -1);

    PlayerMovings data;
    data.packetType = 10;
    data.characterState = 0x0; // animation
    data.x = g_server->Local_Player.pos.m_x;
    data.y = g_server->Local_Player.pos.m_y;
    data.punchX = NULL;
    data.punchY = NULL;
    data.XSpeed = NULL;
    data.YSpeed = NULL;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = itemid;
    g_server->SendPacketRaw(false, 4, g_server->packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE);
}

void server::breakBlock(int x, int y) {
    PlayerMovings data;
    data.packetType = 0x3;
    data.characterState = 0x0; // animation
    data.x = g_server->Local_Player.pos.m_x;
    data.y = g_server->Local_Player.pos.m_y;
    data.punchX = x;
    data.punchY = y;
    data.XSpeed = 0;
    data.YSpeed = 0;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = 18;
    g_server->SendPacketRaw(false , 4, packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE);
}

void server::placeBlock(int tile, int x, int y) {
    PlayerMovings data;
    data.packetType = 0x3;
    data.characterState = 0x0; // animation
    data.x = g_server->Local_Player.pos.m_x;
    data.y = g_server->Local_Player.pos.m_y;
    data.punchX = x;
    data.punchY = y;
    data.XSpeed = 0;
    data.XSpeed = 0;
    data.YSpeed = 0;
    data.netID = g_server->Local_Player.netid;
    data.plantingTree = tile;
    g_server->SendPacketRaw(false ,4, packPlayerMovings(&data), 56, 0, ENET_PACKET_FLAG_RELIABLE);
}


void server::save(float hair, float shirt, float pants, float shoe, float face, float hand, float back, float mask, float neck, float ances)
{
    std::ofstream outfile("savefile.txt");
    if (outfile.is_open())
    {
        outfile << hair << "\n";
        outfile << shirt << "\n";
        outfile << pants << "\n";
        outfile << shoe << "\n";
        outfile << face << "\n";
        outfile << hand << "\n";
        outfile << back << "\n";
        outfile << mask << "\n";
        outfile << neck << "\n";
        outfile << ances << "\n";
        outfile.close();

        g_server->sendState(g_server->Local_Player.netid, true);
    }
    else
    {
        std::cout << "Unable to open file for writing.\n";
    }
}


void server::load(float& hair, float& shirt, float& pants, float& shoe, float& face, float& hand, float& back, float& mask, float& neck, float& ances)
{
    std::ifstream infile("savefile.txt");
    if (infile.is_open())
    {
        infile >> hair;
        infile >> shirt;
        infile >> pants;
        infile >> shoe;
        infile >> face;
        infile >> hand;
        infile >> back;
        infile >> mask;
        infile >> neck;
        infile >> ances;
        infile.close();
        g_server->sendState(g_server->Local_Player.netid, true);
    }
    else
    {
        std::cout << "Unable to open file for writing.\n";
    }
}



void server::Set_Pos(int x, int y)
{
    vector2_t pos;
    pos.m_x = x * 32;
    pos.m_y = y * 32;

    variantlist_t varlist{ "OnSetPos" };
    varlist[1] = pos;
    Local_Player.pos = pos;
    send(true, varlist, Local_Player.netid, -1);
}

void server::pathFindTo(int x, int y) {
    try {
        auto world = g_server->m_world;

        PathFinder pf(100, 60);

        for (int xx = 0; xx < 100; xx++) {
            for (int yy = 0; yy < 60; yy++) {
                int inta = 0;
                auto tile = world.tiles.find(HashCoord(xx, yy));

                int collisionType = world.itemDataContainer.item_map.find(tile->second.header.foreground)->second.collisionType;
                if (collisionType == 0) {
                    inta = 0;
                }
                else if (collisionType == 1) {
                    inta = 1;
                }
                else if (collisionType == 2) {
                    inta = (yy < yy ? 1 : 0);
                }
                else if (collisionType == 3) {
                    inta = !world.hasAccessName() ? (tile->second.header.flags_1 == 0x90 ? 0 : 1) : 0;
                }
                /*else if (collisionType == 4) {
                    inta = tile->second.header.flags_1 == 64 ? 0 : 1;
                }*/
                else {
                    inta = tile->second.header.foreground == 0 ? 0 : 1;
                }

                if (inta == 1) {
                    pf.setBlocked(xx, yy);
                }
            }
        }

        pf.setNeighbors({ -1, 0, 1, 0 }, { 0, 1, 0, -1 });
        vector<pair<int, int>> path = pf.aStar(g_server->Local_Player.pos.m_x / 32, g_server->Local_Player.pos.m_y / 32, x, y);

        if (path.size() > 0) {
            if (path.size() < 150)
                for (auto& p : path) {
                    gameupdatepacket_t packet{ 0 };
                    packet.m_type = PACKET_STATE;
                    packet.m_int_data = 6326;
                    packet.m_vec_x = p.first * 32;
                    packet.m_vec_y = p.second * 32;
                    packet.m_state1 = p.first;
                    packet.m_state2 = p.second;
                    packet.m_packet_flags = UPDATE_PACKET_PLAYER_MOVING_RIGHT;
                    g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));

                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            else {
                for (std::size_t i = 0; i < path.size(); i += 2) {
                    gameupdatepacket_t packet{ 0 };
                    packet.m_type = PACKET_STATE;
                    packet.m_int_data = 6326;
                    packet.m_vec_x = path[i].first * 32;
                    packet.m_vec_y = path[i].second * 32;
                    packet.m_state1 = path[i].first;
                    packet.m_state2 = path[i].second;
                    packet.m_packet_flags = UPDATE_PACKET_PLAYER_MOVING_RIGHT;
                    g_server->send(false, NET_MESSAGE_GAME_PACKET, (uint8_t*)&packet, sizeof(gameupdatepacket_t));

                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));



            variantlist_t lost{ "OnSetPos" };
            vector2_t pos;
            pos.m_x = x * 32;
            pos.m_y = y * 32;
            lost[1] = pos;
            g_server->send(true, lost, g_server->Local_Player.netid, -1);

            variantlist_t notif{ "OnAddNotification" };
            notif[2] = "`2Traveling `9" + to_string(path.size()) + " `2Block";
            notif[4] = 0;
            g_server->send(true, notif, -1, -1);
        }
        else {
            variantlist_t notif{ "OnAddNotification" };
            notif[2] = "`4Traveling Not Possible";
            notif[4] = 0;
            g_server->send(true, notif, -1, -1);
        }
    }
    catch (exception ex)
    {
        variantlist_t notif{ "OnAddNotification" };
        notif[2] = "`8Something Goes Wrong";
        notif[4] = 0;
        g_server->send(true, notif, -1, -1);
    }
}