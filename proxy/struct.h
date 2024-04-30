#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <ctime>
#include <functional>
#include <iostream>
#include "enet/include/enet.h"
#include "proton/variant.hpp"
#include "proton/rtparam.hpp"
#include "utils.h"
#include <map>
#include <set>

template<typename T1, typename T2>
constexpr auto HashCoord(T1 x, T2  y) { return (((y) * 100) + (x)); }
constexpr auto TILE_SIZE = 32;

inline vector2i_t VF2I(vector2_t v) {
	return vector2i_t(round(v.m_x / TILE_SIZE), uint32_t(v.m_y / TILE_SIZE));
}
inline vector2i_t VF2I_S(vector2_t v) { //strict
	return vector2i_t(uint32_t(v.m_x / TILE_SIZE), uint32_t(v.m_y / TILE_SIZE));
}
inline vector2_t VI2F(vector2i_t v) {
	return vector2_t(v.m_x * TILE_SIZE, v.m_y * TILE_SIZE);
}

struct AutoFarm {
	int x;
	int y;
	bool active;
};

struct Item {
	uint16_t id;
	uint8_t count;
	uint8_t type;
};

struct ScanItem {
	uint16_t id;
	uint8_t count;
	std::string name;
};


struct DroppedItem {
	uint16_t itemID;
	vector2_t pos;
	uint8_t count;
	uint8_t flags;
	uint32_t uid;
};

#pragma region ExtraTileDataDefinitions

struct IExtraTileData {

};

struct WorldLockData : IExtraTileData {
	uint8_t settings;
	uint32_t owner_id;
	std::vector<uint32_t> access_list;

	bool isAuthorized(uint32_t userid) {
		return userid != owner_id ? std::find(access_list.begin(), access_list.end(), userid) != access_list.end() : true;
	}
};

struct SeedData : IExtraTileData {
	uint32_t time = 0;
	uint8_t count = 0;
	time_t growTime = 0, ctrTime = 0;

	SeedData(time_t growTime) : growTime(growTime), ctrTime(std::time(nullptr)) { }

	bool isReadyToHarvest() {
		return std::time(nullptr) - ctrTime + time >= growTime;
	}
};

struct MagplantVariant : IExtraTileData {
	uint16_t itemid;
	uint16_t count;
	uint8_t enabled;
};
#pragma endregion

struct TileHeader {
	uint16_t foreground;
	uint16_t background;
	uint16_t data;
	uint8_t flags_1;
	uint8_t flags_2;
	std::shared_ptr<IExtraTileData> extraData;
};

struct Tile {
	TileHeader header;
	vector2i_t pos;
};

struct PlayerInventory {
	uint32_t slotCount;
	uint16_t itemCount;
	std::unordered_map<uint32_t, Item> items;

	bool isItemEquipped(uint32_t itemID) {
		return doesItemExistUnsafe(itemID) ? items[itemID].type == 1 : false;
	}

	bool doesItemExist(uint32_t itemID) {
		return doesItemExistUnsafe(itemID);
	}

	bool doesItemExistUnsafe(uint32_t itemID) {
		return items.find(itemID) != items.end();
	}

	int getItemCount(uint32_t itemID) {
		return getItemCountUnsafe(itemID);
	}

	int getItemCountUnsafe(uint32_t itemID) {
		return doesItemExistUnsafe(itemID) ? items.find(itemID)->second.count : 0;
	}

	int getObjectAmountToPickUp(DroppedItem obj) {
		return getObjectAmountToPickUpUnsafe(obj);
	}

	int getObjectAmountToPickUpUnsafe(DroppedItem obj) {
		int count = getItemCountUnsafe(obj.itemID);
		return count ? (count < 200 ? (200 - count < obj.count ? 200 - count : obj.count) : 0) : (slotCount > items.size() ? obj.count : 0);
	}
};

struct Player {
	std::string name;
	std::string country;
	int netid = -1;
	int userid = -1;
	int state = 0;
	vector2_t pos{};
	bool invis = false;
	bool mod = false;

	virtual ~Player() = default;
	virtual vector2_t GetPos() const {
		return pos;
	}
};

struct LocalPlayer : Player {
	PlayerInventory inventory;
	uint32_t gems_balance = 0;
	uint32_t wl_balance = 0;
	uint32_t level = 0;
	uint32_t awesomeness = 0;

	vector2_t GetPos() const override {
		return pos;
	}
};

struct ItemData {
	int itemID = 0;
	char editableType = 0;
	char itemCategory = 0;
	char actionType = 0;
	char hitSoundType = 0;
	std::string name = "";
	std::string texture = "";
	int textureHash = 0;
	char itemKind = 0;
	int val1;
	char textureX = 0;
	char textureY = 0;
	char spreadType = 0;
	char isStripeyWallpaper = 0;
	char collisionType = 0;
	char aa = 0;
	char breakHits = 0;
	int dropChance = 0;
	char clothingType = 0;
	int16_t rarity = 0;
	unsigned char maxAmount = 0;
	std::string extraFile = "";
	int extraFileHash = 0;
	int audioVolume = 0;
	std::string petName = "";
	std::string petPrefix = "";
	std::string petSuffix = "";
	std::string petAbility = "";
	char seedBase = 0;
	char seedOverlay = 0;
	char treeBase = 0;
	char treeLeaves = 0;
	int seedColor = 0;
	int seedOverlayColor = 0;
	int growTime = 0;
	short val2;
	short isRayman = 0;
	std::string extraOptions = "";
	std::string texture2 = "";
	std::string extraOptions2 = "";
	std::string punchOptions = "";
};
std::map<int, ItemData> itemDefs;
