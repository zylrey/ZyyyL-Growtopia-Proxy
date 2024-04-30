#pragma once
#include <fstream>
#include <string>
#include <iostream>
#ifndef __linux__
#include <conio.h>
#endif
#include "json.hpp"
#include <iomanip>
#include "print.h"
#include "world.h"
void color(int y) {
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(h, y);
}
using namespace std;
std::unordered_map<uint32_t, world::ItemData> item_map;
string to_lower(string s) {
	for (char& c : s)
		c = tolower(c);
	return s;
}
string to_upper(string s) {
	for (char& c : s)
		c = toupper(c);
	return s;
}
inline string getStrLower(string txt)
{
	string ret;
	for (char c : txt) ret += tolower(c);
	return ret;
}
struct types {
	float hair, shirt, pants = 0.f; //cloth_hair & cloth_shirt & cloth_pants
	float shoe, face, hand = 0.f; //cloth_feet & cloth_face & cloth_hand
	float back, mask, neck = 0.f;//cloth_back cloth_mask cloth_necklace
	float ances = 0; //cloth_ances
	float hat = 0.f;
};
void LoadItemData(string filePath) {
	string secret = "PBG892FXX982ABC*";
	string bruh = filePath;

	std::ifstream file(bruh, std::ios::binary | std::ios::ate);
	int size = file.tellg();
	if (size == -1) {
		return;
	}
	char* data = new char[size];
	file.seekg(0, std::ios::beg);

	if (file.read((char*)(data), size))
	{
	}
	else {
		printf("Error occourred from decoding items.dat\n");
		return;
	}
	int memPos = 0;
	int16_t itemsdatVersion = 0;
	memcpy(&itemsdatVersion, data + memPos, 2);
	memPos += 2;
	int intemcount;
	memcpy(&intemcount, data + memPos, 4);
	memPos += 4;

	for (int i = 0; i < intemcount; i++) {
		world::ItemData item;
		{
			memcpy(&item.itemID, data + memPos, 4);
			memPos += 4;
		}
		{
			item.editableType = data[memPos];
			memPos += 1;
		}
		{
			item.itemCategory = data[memPos];
			memPos += 1;
		}
		{
			item.actionType = data[memPos];
			memPos += 1;
		}
		{
			item.hitSoundType = data[memPos];
			memPos += 1;
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.name += data[memPos] ^ (secret[(j + item.itemID) % secret.length()]);
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.texture += data[memPos];
				memPos++;
			}
		}
		memcpy(&item.textureHash, data + memPos, 4);
		memPos += 4;
		item.itemKind = memPos[data];
		memPos += 1;
		memcpy(&item.val1, data + memPos, 4);
		memPos += 4;
		item.textureX = data[memPos];
		memPos += 1;
		item.textureY = data[memPos];
		memPos += 1;
		item.spreadType = data[memPos];
		memPos += 1;
		item.isStripeyWallpaper = data[memPos];
		memPos += 1;
		item.collisionType = data[memPos];
		memPos += 1;
		item.breakHits = data[memPos];
		memPos += 1;
		memcpy(&item.dropChance, data + memPos, 4);
		memPos += 4;
		item.clothingType = data[memPos];
		memPos += 1;
		memcpy(&item.rarity, data + memPos, 2);
		memPos += 2;
		item.maxAmount = data[memPos];
		memPos += 1;
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.extraFile += data[memPos];
				memPos++;
			}
		}
		memcpy(&item.extraFileHash, data + memPos, 4);
		memPos += 4;
		memcpy(&item.audioVolume, data + memPos, 4);
		memPos += 4;
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.petName += data[memPos];
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.petPrefix += data[memPos];
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.petSuffix += data[memPos];
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.petAbility += data[memPos];
				memPos++;
			}
		}
		{
			item.seedBase = data[memPos];
			memPos += 1;
		}
		{
			item.seedOverlay = data[memPos];
			memPos += 1;
		}
		{
			item.treeBase = data[memPos];
			memPos += 1;
		}
		{
			item.treeLeaves = data[memPos];
			memPos += 1;
		}
		{
			memcpy(&item.seedColor, data + memPos, 4);
			memPos += 4;
		}
		{
			memcpy(&item.seedOverlayColor, data + memPos, 4);
			memPos += 4;
		}
		memPos += 4; // deleted ingredients
		{
			memcpy(&item.growTime, data + memPos, 4);
			memPos += 4;
		}
		memcpy(&item.val2, data + memPos, 2);
		memPos += 2;
		memcpy(&item.isRayman, data + memPos, 2);
		memPos += 2;
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.extraOptions += data[memPos];
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.texture2 += data[memPos];
				memPos++;
			}
		}
		{
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				item.extraOptions2 += data[memPos];
				memPos++;
			}
		}
		memPos += 80;
		if (itemsdatVersion >= 11) {
			{
				int16_t strLen = *(int16_t*)&data[memPos];
				memPos += 2;
				for (int j = 0; j < strLen; j++) {
					item.punchOptions += data[memPos];
					memPos++;
				}
			}
		}
		if (itemsdatVersion >= 12) memPos += 13;
		if (itemsdatVersion >= 13) memPos += 4;
		if (itemsdatVersion >= 14) memPos += 4;
		if (itemsdatVersion >= 15) {
			memPos += 25;
			int16_t strLen = *(int16_t*)&data[memPos];
			memPos += 2;
			for (int j = 0; j < strLen; j++) {
				memPos++;
			}
		}
		item_map[item.itemID] = item;
	}
}