/*
 * AscEmu Framework based on ArcEmu MMORPG Server
 * Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
 * Copyright (C) 2008-2012 ArcEmu Team <http://www.ArcEmu.org/>
 * Copyright (C) 2005-2007 Ascent Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Objects/Units/Players/PlayerDefines.hpp"
#include "Objects/Units/Stats.h"
#include "Management/QuestDefines.hpp"
#include "Management/Battleground/BattlegroundMgr.h"
#include "Management/MailMgr.h"
#include "Management/ItemPrototype.h"
#include "Management/AchievementMgr.h"
#include "Map/InstanceDefines.hpp"
#include "Objects/Units/Unit.h" 
#include "Storage/MySQLStructures.h"
#include "Macros/MapsMacros.hpp"
#include "Macros/PlayerMacros.hpp"
#include "Objects/Units/Creatures/AIInterface.h" //?? what?
#include "WorldConf.h"
#include "Management/AuctionHouse.h"
#include "Management/Guild/Guild.hpp"
#include "Management/ObjectUpdates/UpdateManager.h"
#include "Data/WoWPlayer.hpp"
#include <mutex>

#include "TradeData.hpp"

class ArenaTeam;
struct CharCreate;
class QuestLogEntry;
struct BGScore;
class AchievementMgr;
class Channel;
class Creature;
class Battleground;
class TaxiPath;
class GameObject;
class Transporter;
class Corpse;
class Guild;
class Pet;
class Charter;
class LfgMatch;
struct LevelInfo;
class SpeedCheatDetector;
struct GuildMember;

class QueryBuffer;
struct QuestProperties;
struct SpellShapeshiftForm;
class CBattleground;
class Instance;
struct CharRaceEntry;
struct CharClassEntry;
struct Trainer;
class Aura;

struct OnHitSpell;
class CachedCharacterInfo;

//\todo: everything above this comment, does not belong in this file. Refactor this file to hold only the player class ;-)
// Everything below this line is bloated (seems we need some new concepts like RAII and a lot of refactoring to shrink it to a manageable class.
// Group all related members to a struct/class. Follow the "modern" way of C++ and leave the C way behind.
// 1. Initialize class members in the class
// 2. Use const wherever possible
// 3. move stuff out of this class
// 4. Check out the members (there are duplicats)
// 5. Get rid of legacy files (Player.Legacy.cpp)
struct WoWPlayer;
class SERVER_DECL Player : public Unit
{
public:

    Player(uint32_t guid);
    ~Player();

    //////////////////////////////////////////////////////////////////////////////////////////
    // Essential functions

    void Update(unsigned long time_passed);             // hides function Unit::Update
    void AddToWorld();                                  // hides virtual function Object::AddToWorld
    void AddToWorld(MapMgr* pMapMgr);                   // hides virtual function Object::AddToWorld
    // void PushToWorld(MapMgr*);                       // not used
    // void RemoveFromWorld(bool free_guid);            // not used
    void OnPrePushToWorld() override;                   // overrides virtual function  Object::OnPrePushToWorld
    void OnPushToWorld() override;                      // overrides virtual function  Object::OnPushToWorld
    // void OnPreRemoveFromWorld();                     // not used
    // void OnRemoveFromWorld();                        // not used

private:
    const WoWPlayer* playerData() const { return reinterpret_cast<WoWPlayer*>(wow_data); }
public:
    void resendSpeed();

private:
    UpdateManager m_updateMgr;
public:
    UpdateManager& getUpdateMgr();

    //////////////////////////////////////////////////////////////////////////////////////////
    // Data
    uint64_t getDuelArbiter() const;
    void setDuelArbiter(uint64_t guid);

    uint32_t getPlayerFlags() const;
    void setPlayerFlags(uint32_t flags);
    void addPlayerFlags(uint32_t flags);
    void removePlayerFlags(uint32_t flags);
    bool hasPlayerFlags(uint32_t flags) const;

    uint32_t getGuildId() const;
    void setGuildId(uint32_t guildId);

    uint32_t getGuildRank() const;
    void setGuildRank(uint32_t guildRank);

#if VERSION_STRING >= Cata
    uint32_t getGuildLevel() const;
    void setGuildLevel(uint32_t guildLevel);
#endif

    //bytes begin
    uint32_t getPlayerBytes() const;
    void setPlayerBytes(uint32_t bytes);

    uint8_t getSkinColor() const;
    void setSkinColor(uint8_t color);

    uint8_t getFace() const;
    void setFace(uint8_t face);

    uint8_t getHairStyle() const;
    void setHairStyle(uint8_t style);

    uint8_t getHairColor() const;
    void setHairColor(uint8_t color);
    //bytes end

    //bytes2 begin
    uint32_t getPlayerBytes2() const;
    void setPlayerBytes2(uint32_t bytes2);

    uint8_t getFacialFeatures() const;
    void setFacialFeatures(uint8_t feature);

    uint8_t getBytes2UnknownField() const;
    void setBytes2UnknownField(uint8_t value);

    uint8_t getBankSlots() const;
    void setBankSlots(uint8_t slots);

    uint8_t getRestState() const;
    void setRestState(uint8_t state);
    //bytes2 end

    //bytes3 begin
    uint32_t getPlayerBytes3() const;
    void setPlayerBytes3(uint32_t bytes3);

    //\note already available in unit data
    uint8_t getPlayerGender() const;
    void setPlayerGender(uint8_t gender);

    uint8_t getDrunkValue() const;
    void setDrunkValue(uint8_t value);

    uint8_t getPvpRank() const;
    void setPvpRank(uint8_t rank);

    uint8_t getArenaFaction() const;
    void setArenaFaction(uint8_t faction);
    //bytes3 end

    uint32_t getDuelTeam() const;
    void setDuelTeam(uint32_t team);

    uint32_t getGuildTimestamp() const;
    void setGuildTimestamp(uint32_t timestamp);

    //QuestLog start
    uint32_t getQuestLogEntryForSlot(uint8_t slot) const;
    void setQuestLogEntryBySlot(uint8_t slot, uint32_t questEntry);

    uint32_t getQuestLogStateForSlot(uint8_t slot) const;
    void setQuestLogStateBySlot(uint8_t slot, uint32_t state);

#if VERSION_STRING > TBC
    uint64_t getQuestLogRequiredMobOrGoForSlot(uint8_t slot) const;
    void setQuestLogRequiredMobOrGoBySlot(uint8_t slot, uint64_t mobOrGoCount);
#else
    uint32_t getQuestLogRequiredMobOrGoForSlot(uint8_t slot) const;
    void setQuestLogRequiredMobOrGoBySlot(uint8_t slot, uint32_t mobOrGoCount);
#endif

    uint32_t getQuestLogExpireTimeForSlot(uint8_t slot) const;
    void setQuestLogExpireTimeBySlot(uint8_t slot, uint32_t expireTime);
    //QuestLog end

//VisibleItem start
    uint32_t getVisibleItemEntry(uint32_t slot) const;
    void setVisibleItemEntry(uint32_t slot, uint32_t entry);
#if VERSION_STRING > TBC
    uint16_t getVisibleItemEnchantment(uint32_t slot, uint8_t pos) const;
    void setVisibleItemEnchantment(uint32_t slot, uint8_t pos, uint16_t enchantment);
#else
    uint32_t getVisibleItemEnchantment(uint32_t slot, uint8_t pos) const;
    void setVisibleItemEnchantment(uint32_t slot, uint8_t pos, uint32_t enchantment);
#endif
//VisibleItem end

    uint64_t getVendorBuybackSlot(uint8_t slot) const;
    void setVendorBuybackSlot(uint8_t slot, uint64_t guid);

    uint64_t getFarsightGuid() const;
    void setFarsightGuid(uint64_t farsightGuid);

#if VERSION_STRING > Classic
    uint64_t getKnownTitles(uint8_t index) const;
    void setKnownTitles(uint8_t index, uint64_t title);
#endif

#if VERSION_STRING > Classic
    uint32_t getChosenTitle() const;
    void setChosenTitle(uint32_t title);
#endif

#if VERSION_STRING == WotLK
    uint64_t getKnownCurrencies() const;
    void setKnownCurrencies(uint64_t currencies);
#endif

    uint32_t getXp() const;
    void setXp(uint32_t xp);

    uint32_t getNextLevelXp() const;
    void setNextLevelXp(uint32_t xp);

#if VERSION_STRING < Cata
    uint16_t getSkillInfoId(uint32_t index) const;
    uint16_t getSkillInfoStep(uint32_t index) const;
    uint16_t getSkillInfoCurrentValue(uint32_t index) const;
    uint16_t getSkillInfoMaxValue(uint32_t index) const;
    int16_t getSkillInfoBonusTemporary(uint32_t index) const;
    int16_t getSkillInfoBonusPermanent(uint32_t index) const;
    void setSkillInfoId(uint32_t index, uint16_t id);
    void setSkillInfoStep(uint32_t index, uint16_t step);
    void setSkillInfoCurrentValue(uint32_t index, uint16_t current);
    void setSkillInfoMaxValue(uint32_t index, uint16_t max);
    void setSkillInfoBonusTemporary(uint32_t index, int16_t bonus);
    void setSkillInfoBonusPermanent(uint32_t index, int16_t bonus);
#else
    uint16_t getSkillLineId(uint32_t index, uint8_t offset) const;
    uint16_t getSkillStep(uint32_t index, uint8_t offset) const;
    uint16_t getSkillCurrentValue(uint32_t index, uint8_t offset) const;
    uint16_t getSkillMaximumValue(uint32_t index, uint8_t offset) const;
    uint32_t getProfessionSkillLine(uint32_t index) const;
    void setSkillLineId(uint32_t index, uint32_t value);
    void setSkillStep(uint32_t index, uint32_t value);
    void setSkillCurrentValue(uint32_t index, uint32_t value);
    void setSkillMaximumValue(uint32_t index, uint32_t value);
    void setProfessionSkillLine(uint32_t index, uint32_t value);
#endif

    uint32_t getFreeTalentPoints() const;
#if VERSION_STRING < Cata
    void setFreeTalentPoints(uint32_t points);
#endif

    uint32_t getFreePrimaryProfessionPoints() const;
    void setFreePrimaryProfessionPoints(uint32_t points);
    void modFreePrimaryProfessionPoints(int32_t amount);

    uint32_t getTrackCreature() const;
    void setTrackCreature(uint32_t id);

    uint32_t getTrackResource() const;
    void setTrackResource(uint32_t id);

    float getBlockPercentage() const;
    void setBlockPercentage(float value);

    float getDodgePercentage() const;
    void setDodgePercentage(float value);

    float getParryPercentage() const;
    void setParryPercentage(float value);

#if VERSION_STRING >= TBC
    uint32_t getExpertise() const;
    void setExpertise(uint32_t value);
    void modExpertise(int32_t value);

    uint32_t getOffHandExpertise() const;
    void setOffHandExpertise(uint32_t value);
    void modOffHandExpertise(int32_t value);
#endif

    float getMeleeCritPercentage() const;
    void setMeleeCritPercentage(float value);

    float getRangedCritPercentage() const;
    void setRangedCritPercentage(float value);

#if VERSION_STRING >= TBC
    float getOffHandCritPercentage() const;
    void setOffHandCritPercentage(float value);

    float getSpellCritPercentage(uint8_t school) const;
    void setSpellCritPercentage(uint8_t school, float value);

    uint32_t getShieldBlock() const;
    void setShieldBlock(uint32_t value);
#endif

#if VERSION_STRING >= WotLK
    float getShieldBlockCritPercentage() const;
    void setShieldBlockCritPercentage(float value);
#endif

    uint32_t getExploredZone(uint32_t idx) const;
    void setExploredZone(uint32_t idx, uint32_t data);

    uint32_t getSelfResurrectSpell() const;
    void setSelfResurrectSpell(uint32_t spell);

    uint32_t getWatchedFaction() const;
    void setWatchedFaction(uint32_t factionId);

#if VERSION_STRING < WotLK
    float getManaRegeneration() const;
    void setManaRegeneration(float value);

    float getManaRegenerationWhileCasting() const;
    void setManaRegenerationWhileCasting(float value);
#endif

    uint32_t getMaxLevel() const;
    void setMaxLevel(uint32_t level);

#if VERSION_STRING >= WotLK
    float getRuneRegen(uint8_t rune) const;
    void setRuneRegen(uint8_t rune, float regen);
#endif

    uint32_t getRestStateXp() const;
    void setRestStateXp(uint32_t xp);

    //\brief: the playerfield coinage is an uint64_t since cata
#if VERSION_STRING < Cata
    uint32_t getCoinage() const;
    void setCoinage(uint32_t coinage);
    bool hasEnoughCoinage(uint32_t coinage) const;
    void modCoinage(int32_t coinage);
#else
    uint64_t getCoinage() const;
    void setCoinage(uint64_t coinage);
    bool hasEnoughCoinage(uint64_t coinage) const;
    void modCoinage(int64_t coinage);
#endif

#if VERSION_STRING == Classic
    uint32_t getResistanceBuffModPositive(uint8_t type) const;
    void setResistanceBuffModPositive(uint8_t type, uint32_t value);

    uint32_t getResistanceBuffModNegative(uint8_t type) const;
    void setResistanceBuffModNegative(uint8_t type, uint32_t value);
#endif

    uint32_t getModDamageDonePositive(uint16_t school) const;
    void setModDamageDonePositive(uint16_t school, uint32_t value);
    void modModDamageDonePositive(uint16_t school, int32_t value);

    uint32_t getModDamageDoneNegative(uint16_t school) const;
    void setModDamageDoneNegative(uint16_t school, uint32_t value);
    void modModDamageDoneNegative(uint16_t school, int32_t value);

    float getModDamageDonePct(uint8_t shool) const;
    void setModDamageDonePct(float damagePct, uint8_t shool);

#if VERSION_STRING >= TBC
    uint32_t getModHealingDone() const;
    void setModHealingDone(uint32_t value);
    void modModHealingDone(int32_t value);

    // Spell penetration?
    uint32_t getModTargetResistance() const;
    void setModTargetResistance(uint32_t value);
    void modModTargetResistance(int32_t value);

    // Armor penetration?
    uint32_t getModTargetPhysicalResistance() const;
    void setModTargetPhysicalResistance(uint32_t value);
    void modModTargetPhysicalResistance(int32_t value);
#endif

    // playerfieldbytes start
    uint32_t getPlayerFieldBytes() const;
    void setPlayerFieldBytes(uint32_t bytes);

    uint8_t getActionBarId() const;
    void setActionBarId(uint8_t actionBarId);
    // playerfieldbytes end

#if VERSION_STRING < Cata
    uint32_t getAmmoId() const;
    void setAmmoId(uint32_t id);
#endif

    uint32_t getBuybackPriceSlot(uint8_t slot) const;
    void setBuybackPriceSlot(uint8_t slot, uint32_t price);

    uint32_t getBuybackTimestampSlot(uint8_t slot) const;
    void setBuybackTimestampSlot(uint8_t slot, uint32_t timestamp);

#if VERSION_STRING > Classic
    uint32_t getFieldKills() const;
    void setFieldKills(uint32_t kills);
#endif

#if VERSION_STRING > Classic
#if VERSION_STRING < Cata
    uint32_t getContributionToday() const;
    void setContributionToday(uint32_t contribution);

    uint32_t getContributionYesterday() const;
    void setContributionYesterday(uint32_t contribution);
#endif
#endif

    uint32_t getLifetimeHonorableKills() const;
    void setLifetimeHonorableKills(uint32_t kills);

    // playerfieldbytes2 start
    uint32_t getPlayerFieldBytes2() const;
    void setPlayerFieldBytes2(uint32_t bytes);
    // playerfieldbytes2 end

    uint32_t getCombatRating(uint8_t combatRating) const;
    void setCombatRating(uint8_t combatRating, uint32_t value);
    void modCombatRating(uint8_t combatRating, int32_t value);

#if VERSION_STRING > Classic
    // field_arena_team_info start
    uint32_t getArenaTeamId(uint8_t teamSlot) const;
    void setArenaTeamId(uint8_t teamSlot, uint32_t teamId);

    uint32_t getArenaTeamMemberRank(uint8_t teamSlot) const;
    void setArenaTeamMemberRank(uint8_t teamSlot, uint32_t rank);
    // field_arena_team_info end
#endif

    uint64_t getInventorySlotItemGuid(uint8_t index) const;
    void setInventorySlotItemGuid(uint8_t index, uint64_t guid);

#if VERSION_STRING > Classic
#if VERSION_STRING < Cata
    uint32_t getHonorCurrency() const;
    void setHonorCurrency(uint32_t amount);
    void modHonorCurrency(int32_t value);

    uint32_t getArenaCurrency() const;
    void setArenaCurrency(uint32_t amount);
    void modArenaCurrency(int32_t value);
#endif
#endif

#if VERSION_STRING >= WotLK
    uint32_t getNoReagentCost(uint8_t index) const;
    void setNoReagentCost(uint8_t index, uint32_t value);

    uint32_t getGlyphSlot(uint16_t slot) const;
    void setGlyphSlot(uint16_t slot, uint32_t glyph);

    uint32_t getGlyph(uint16_t slot) const;
    void setGlyph(uint16_t slot, uint32_t glyph);

    uint32_t getGlyphsEnabled() const;
    void setGlyphsEnabled(uint32_t glyphs);
#endif

    //////////////////////////////////////////////////////////////////////////////////////////
    // Movement/Position
public:

    void sendForceMovePacket(UnitSpeedType speed_type, float speed);
    void sendMoveSetSpeedPaket(UnitSpeedType speed_type, float speed);

    bool isMoving() const { return m_isMoving; }

    void handleBreathing(MovementInfo const& movement_info, WorldSession* session);
    void handleAuraInterruptForMovementFlags(MovementInfo const& movement_info);

    uint32_t getAreaId() const { return m_areaId; }
    void setAreaId(uint32_t area) { m_areaId = area; }

    bool isInCity() const;

    void initialiseNoseLevel();

    bool m_isWaterBreathingEnabled = false;
    uint32_t m_underwaterTime = 180000;
    uint32_t m_underwaterMaxTime = 180000;
    uint32_t m_underwaterState = 0;

    void setTransferStatus(uint8_t status);
    uint8_t getTransferStatus() const;
    bool isTransferPending() const;

protected:

    bool m_isMoving = false;
    bool m_isMovingFB = false;
    bool m_isStrafing = false;
    bool m_isTurning = false;
    bool m_isJumping = false;

    uint32_t m_areaId = 0;

    float m_noseLevel = .0f;

    uint8_t m_transferStatus = TRANSFER_NONE;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Basic
public:

    std::string getName() const;
    void setName(std::string name);

    void setInitialDisplayIds(uint8_t gender, uint8_t race);

    void applyLevelInfo(uint32_t newLevel);

    virtual bool isClassMage();
    virtual bool isClassDeathKnight();
    virtual bool isClassPriest();
    virtual bool isClassRogue();
    virtual bool isClassShaman();
    virtual bool isClassHunter();
    virtual bool isClassWarlock();
    virtual bool isClassWarrior();
    virtual bool isClassPaladin();
    virtual bool isClassMonk();
    virtual bool isClassDruid();

    PlayerTeam getTeam() const;
    PlayerTeam getBgTeam() const;
    void setTeam(uint32_t team);
    void setBgTeam(uint32_t team);

    uint32_t getInitialTeam() const;

    void resetTeam();
    bool isTeamHorde() const;
    bool isTeamAlliance() const;

    Player* getPlayerOwner() override;

    void toggleAfk();
    void toggleDnd();

private:

    //used for classic
    uint32_t max_level = 60;

    std::string m_name;

    uint32_t m_team = 0;
    uint32_t m_bgTeam = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Stats
    // Initializes stats and unit/playerdata fields
public:

    void setInitialPlayerData();

    // Not same as Unit::regeneratePowers
    void regeneratePlayerPowers(uint16_t diff);
#if VERSION_STRING >= Cata
    void resetHolyPowerTimer();
#endif

    // PlayerStats.cpp
    void updateManaRegeneration();

private:
    // Regenerate timers
    // Rage and Runic Power
    uint16_t m_rageRunicPowerRegenerateTimer = 0;
#if VERSION_STRING >= Cata
    uint16_t m_holyPowerRegenerateTimer = 0;
#endif

    // This timer ticks even if the player is not eating or drinking
    uint16_t m_foodDrinkSpellVisualTimer = 5000;

#if VERSION_STRING == Classic
    // Classic doesn't have these in unit or playerdata
    float m_manaRegeneration = 0.0f;
    float m_manaRegenerationWhileCasting = 0.0f;
#endif

    //////////////////////////////////////////////////////////////////////////////////////////
    // Database stuff
public:

    bool loadSpells(QueryResult* result);
    bool loadReputations(QueryResult* result);

    //////////////////////////////////////////////////////////////////////////////////////////
    // Spells and skills
    void setInitialPlayerSkills();

    void updateAutoRepeatSpell();
    bool canUseFlyingMountHere();

    bool canDualWield2H() const;
    void setDualWield2H(bool enable);

    bool isSpellFitByClassAndRace(uint32_t spell_id) const;

    // Cooldowns
    bool hasSpellOnCooldown(SpellInfo const* spellInfo);
    bool hasSpellGlobalCooldown(SpellInfo const* spellInfo);
    // Do NOT add cooldownTime if you don't know what you're doing (it's required for spells with dynamic cooldown)
    void addSpellCooldown(SpellInfo const* spellInfo, Item const* itemCaster, Spell* castingSpell = nullptr, int32_t cooldownTime = 0);
    void addGlobalCooldown(SpellInfo const* spellInfo, Spell* castingSpell, const bool sendPacket = false);
    void sendSpellCooldownPacket(SpellInfo const* spellInfo, const uint32_t duration, const bool isGcd);
    void clearCooldownForSpell(uint32_t spellId);
    void clearGlobalCooldown();
    void resetAllCooldowns();

    void setInitialLanguages();

    uint32_t getArmorProficiency() const;
    void addArmorProficiency(uint32_t proficiency);
    uint32_t getWeaponProficiency() const;
    void addWeaponProficiency(uint32_t proficiency);

#ifdef FT_GLYPHS
    // Glyphs
    // Initializes glyph slots or updates them on levelup
    void updateGlyphs();
#endif

    // Combo Points
    uint64_t getComboPointTarget() const;
    int8_t getComboPoints() const;
    void addComboPoints(uint64_t targetGuid, int8_t points);
    void updateComboPoints();
    void clearComboPoints();

    bool m_FirstCastAutoRepeat = false;

private:
    bool m_canDualWield2H = false;

    uint32_t armorProficiency = 0;
    uint32_t weaponProficiency = 0;

    uint64_t m_comboTarget = 0;
    int8_t m_comboPoints = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Talents
public:

    void learnTalent(uint32_t talentId, uint32_t talentRank);
    void addTalent(SpellInfo const* sp);
    void removeTalent(uint32_t spellId, bool onSpecChange = false);
    // Resets only current spec's talents
    void resetTalents();
    // Resets talents for both specs
    void resetAllTalents();
    void setTalentPoints(uint32_t talentPoints, bool forBothSpecs = true);
    void addTalentPoints(uint32_t talentPoints, bool forBothSpecs = true);
    void setInitialTalentPoints(bool talentsResetted = false);

    uint32_t getTalentPointsFromQuests() const;
    void setTalentPointsFromQuests(uint32_t talentPoints);
    void smsg_TalentsInfo(bool SendPetTalents); // TODO: classic and tbc

    void activateTalentSpec(uint8_t specId);

private:
    uint32_t m_talentPointsFromQuests = 0;

    /////////////////////////////////////////////////////////////////////////////////////////
    // Tutorials
public:

    uint32_t getTutorialValueById(uint8_t id);
    void setTutorialValueForId(uint8_t id, uint32_t value);

    void loadTutorials();
    void saveTutorials();

protected:
    uint32_t m_Tutorials[8] = {0};

    /////////////////////////////////////////////////////////////////////////////////////////
    // Actionbar
public:

    void setActionButton(uint8_t button, uint32_t action, uint8_t type, uint8_t misc);
    void sendActionBars(bool clearBars);

    //////////////////////////////////////////////////////////////////////////////////////////
    // Auction
    void sendAuctionCommandResult(Auction* auction, uint32_t Action, uint32_t ErrorCode, uint32_t bidError = 0);

    //////////////////////////////////////////////////////////////////////////////////////////
    // Trade
public:

    Player* getTradeTarget() const;
    TradeData* getTradeData() const;
    void cancelTrade(bool sendToSelfAlso, bool silently = false);

private:
    TradeData* m_TradeData = nullptr;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Messages
public:

    void sendReportToGmMessage(std::string playerName, std::string damageLog);


    //////////////////////////////////////////////////////////////////////////////////////////
    // Commands
public:

    void disableSummoning(bool disable);
    bool isSummoningDisabled() const;
    void disableAppearing(bool disable);
    bool isAppearingDisabled() const;

    bool isBanned() const;
    void setBanned(uint32_t timestamp = 4, std::string Reason = "");
    void unsetBanned();
    std::string getBanReason() const;

    GameObject* getSelectedGo() const;
    void setSelectedGo(uint64_t guid);

    void kickFromServer(uint32_t delay = 0);
    void eventKickFromServer();

    PlayerCheat m_cheats = {false};
    float m_goLastXRotation = 0.0f;
    float m_goLastYRotation = 0.0f;

    bool m_saveAllChangesCommand = false;

    bool m_XpGainAllowed = true;

    AIInterface* m_aiInterfaceWaypoint = nullptr;

    bool m_isGmInvisible = false;

    Creature* m_formationMaster = nullptr;

private:

    bool m_disableAppearing = false;
    bool m_disableSummoning = false;

    uint64_t m_GMSelectedGO = 0;

    uint32_t m_banned = 0;
    std::string m_banreason;
    uint32_t m_kickDelay = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Items
public:

    void unEquipOffHandIfRequired();
    bool hasOffHandWeapon() const;

    bool hasItem(uint32_t itemId, uint32_t amount = 1, bool checkBankAlso = false) const;

#if VERSION_STRING >= WotLK
    // Soulbound Tradeable
    void updateSoulboundTradeItems();
    void addTradeableItem(Item* item);
    void removeTradeableItem(Item* item);
    ItemDurationList m_itemSoulboundTradeable;
#endif

    // Player's item storage
    ItemInterface* getItemInterface() const;
private:
    ItemInterface* m_itemInterface = nullptr;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Difficulty
public:

    void setDungeonDifficulty(uint8_t diff);
    uint8_t getDungeonDifficulty();

    void setRaidDifficulty(uint8_t diff);
    uint8_t getRaidDifficulty();

private:
    uint8_t m_dungeonDifficulty = 0;
    uint8_t m_raidDifficulty = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Corpse
public:

    void setCorpseData(LocationVector position, int32_t instanceId);
    LocationVector getCorpseLocation() const;
    int32_t getCorpseInstanceId() const;

    void setAllowedToCreateCorpse(bool allowed);
    bool isAllowedToCreateCorpse() const;

private:
    struct CorpseData
    {
        LocationVector location = {0,0,0,0};
        int32_t instanceId = 0;
    };
    CorpseData m_corpseData;

    bool isCorpseCreationAllowed = true;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Bind
public:

    void setBindPoint(float x, float y, float z, uint32_t mapId, uint32_t zoneId);

    LocationVector getBindPosition() const;
    uint32_t getBindMapId() const;
    uint32_t getBindZoneId() const;

    bool m_hasBindDialogOpen = false;

private:
    struct BindData
    {
        LocationVector location = { 0, 0, 0 };
        uint32_t mapId = 0;
        uint32_t zoneId = 0;
    };
    BindData m_bindData;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Battleground Entry
public:

    void setBGEntryPoint(float x, float y, float z, float o, uint32_t mapId, int32_t instanceId);

    LocationVector getBGEntryPosition() const;
    uint32_t getBGEntryMapId() const;
    int32_t getBGEntryInstanceId() const;

private:
    struct BGEntryData
    {
        LocationVector location = { 0, 0, 0, 0 };
        uint32_t mapId = 0;
        int32_t instanceId = 0;
    };
    BGEntryData m_bgEntryData;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Charter
public:

    void unsetCharter(uint8_t charterType);
    Charter* getCharter(uint8_t charterType);

    bool canSignCharter(Charter* charter, Player* requester);
    void initialiseCharters();

private:
    Charter* m_charters[NUM_CHARTER_TYPES] = {nullptr};

    //////////////////////////////////////////////////////////////////////////////////////////
    // Guild
public:

    void setInvitedByGuildId(uint32_t GuildId);
    uint32_t getInvitedByGuildId() const;

    Guild* getGuild() const;
    bool isInGuild();

    uint32_t getGuildRankFromDB();

private:
    uint32_t m_invitedByGuildId = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Group
public:

    void setGroupInviterId(uint32_t inviterId);
    uint32_t getGroupInviterId() const;
    bool isAlreadyInvitedToGroup() const;

    bool isInGroup() const;

    Group* getGroup();
    bool isGroupLeader() const;

    int8_t getSubGroupSlot() const;

private:
    uint32_t m_GroupInviter = 0;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Channels
public:

    void joinedChannel(Channel* channel);
    void leftChannel(Channel* channel);

    void updateChannels();
    void removeAllChannels();

private:

    std::set<Channel*> m_channels;
    mutable std::mutex m_mutexChannel;

    //////////////////////////////////////////////////////////////////////////////////////////
    // ArenaTeam
public:

    void setArenaTeam(uint8_t type, ArenaTeam* arenaTeam);
    ArenaTeam* getArenaTeam(uint8_t type);

    bool isInArenaTeam(uint8_t type);
    void initialiseArenaTeam();

private:
    ArenaTeam* m_arenaTeams[NUM_ARENA_TEAM_TYPES] = {nullptr};

    //////////////////////////////////////////////////////////////////////////////////////////
    // Quests
public:
    void setQuestLogInSlot(QuestLogEntry* entry, uint32_t slotId);

    bool hasAnyQuestInQuestSlot() const;
    bool hasTimedQuestInQuestSlot() const;
    bool hasQuestInQuestLog(uint32_t questId) const;
    uint8_t getFreeQuestSlot() const;

    QuestLogEntry* getQuestLogByQuestId(uint32_t questId) const;
    QuestLogEntry* getQuestLogBySlotId(uint32_t slotId) const;

    void addQuestIdToFinishedDailies(uint32_t questId);
    std::set<uint32_t> getFinishedDailies() const;
    bool hasQuestInFinishedDailies(uint32_t questId) const;
    void resetFinishedDailies();

private:
    QuestLogEntry* m_questlog[MAX_QUEST_LOG_SIZE] = {nullptr};

    mutable std::mutex m_mutextDailies;
    std::set<uint32_t> m_finishedDailies = {};

    //////////////////////////////////////////////////////////////////////////////////////////
    // Social
public:

    struct SocialFriends
    {
        uint32_t friendGuid = 0;
        mutable std::string note;
    };

    // Initialise Database values
    void loadFriendList();
    void loadFriendedByOthersList();
    void loadIgnoreList();

    void addToFriendList(std::string name, std::string note);
    void removeFromFriendList(uint32_t guid);
    void addNoteToFriend(uint32_t guid, std::string note);
    bool isFriended(uint32_t guid) const;

    void sendFriendStatus(bool comesOnline);
    void sendFriendLists(uint32_t flags);

    void addToIgnoreList(std::string name);
    void removeFromIgnoreList(uint32_t guid);
    bool isIgnored(uint32_t guid) const;

private:
    std::vector<SocialFriends> m_socialIFriends = {};
    mutable std::mutex m_mutexFriendList;

    std::vector<uint32_t> m_socialFriendedByGuids = {};
    mutable std::mutex m_mutexFriendedBy;

    std::vector<uint32_t> m_socialIgnoring = {};
    mutable std::mutex m_mutexIgnoreList;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Hackdetection

    //Speed
    //Fly
    //Teleport
    //NoClip
    //Waterwalk
    //Size
    //Wallclimb
    //Itemstacking (spell/attack power stacking)

    //////////////////////////////////////////////////////////////////////////////////////////
    // Misc
public:

    bool isGMFlagSet();

    void sendMovie(uint32_t movieId);

    void logIntoBattleground();
    bool logOntoTransport();
    void setLoginPosition();
    void setPlayerInfoIfNeeded();
    void setGuildAndGroupInfo();
    void sendCinematicOnFirstLogin();

    void sendTalentResetConfirmPacket();
    void sendPetUnlearnConfirmPacket();
    void sendDungeonDifficultyPacket();
    void sendRaidDifficultyPacket();
    void sendInstanceDifficultyPacket(uint8_t difficulty);
    void sendNewDrunkStatePacket(uint32_t state, uint32_t itemId);
    void sendSetProficiencyPacket(uint8_t itemClass, uint32_t proficiency);
    void sendPartyKillLogPacket(uint64_t killedGuid);
    void sendDestroyObjectPacket(uint64_t destroyedGuid);
    void sendEquipmentSetUseResultPacket(uint8_t result);
    void sendTotemCreatedPacket(uint8_t slot, uint64_t guid, uint32_t duration, uint32_t spellId);

    void sendGossipPoiPacket(float posX, float posY, uint32_t icon, uint32_t flags, uint32_t data, std::string name);
    void sendPoiById(uint32_t id);

    void sendStopMirrorTimerPacket(MirrorTimerTypes type);
    void sendMeetingStoneSetQueuePacket(uint32_t dungeonId, uint8_t status);
    void sendPlayObjectSoundPacket(uint64_t objectGuid, uint32_t soundId);
    void sendPlaySoundPacket(uint32_t soundId);
    void sendExploreExperiencePacket(uint32_t areaId, uint32_t experience);
    void sendSpellCooldownEventPacket(uint32_t spellId);
#if VERSION_STRING < Cata
    void sendSpellModifierPacket(uint8_t spellGroup, uint8_t spellType, int32_t modifier, bool isPct);
#else
    void sendSpellModifierPacket(uint8_t spellType, std::vector<std::pair<uint8_t, float>> modValues, bool isPct);
#endif
    void sendLoginVerifyWorldPacket(uint32_t mapId, float posX, float posY, float posZ, float orientation);
    void sendMountResultPacket(uint32_t result);
    void sendDismountResultPacket(uint32_t result);
    void sendLogXpGainPacket(uint64_t guid, uint32_t normalXp, uint32_t restedXp, bool type);
    void sendCastFailedPacket(uint32_t spellId, uint8_t errorMessage, uint8_t multiCast, uint32_t extra1, uint32_t extra2 = 0);
    void sendLevelupInfoPacket(uint32_t level, uint32_t hp, uint32_t mana, uint32_t stat0, uint32_t stat1, uint32_t stat2, uint32_t stat3, uint32_t stat4);
    void sendItemPushResultPacket(bool created, bool recieved, bool sendtoset, uint8_t destbagslot, uint32_t destslot, uint32_t count, uint32_t entry, uint32_t suffix, uint32_t randomprop, uint32_t stack);
    void sendClientControlPacket(Unit* target, uint8_t allowMove);
    void sendGuildMotd();

    bool isPvpFlagSet() override;
    void setPvpFlag() override;
    void removePvpFlag() override;

    bool isFfaPvpFlagSet() override;
    void setFfaPvpFlag() override;
    void removeFfaPvpFlag() override;

    bool isSanctuaryFlagSet() override;
    void setSanctuaryFlag() override;
    void removeSanctuaryFlag() override;

    void sendPvpCredit(uint32_t honor, uint64_t victimGuid, uint32_t victimRank);
    void sendRaidGroupOnly(uint32_t timeInMs, uint32_t type);

    void setVisibleItemFields(uint32_t slot, Item* item);
    void applyReforgeEnchantment(Item* item, bool apply);

    void setAFKReason(std::string reason) { afkReason = reason; }
    std::string getAFKReason() const { return afkReason; }

    void addToGMTargetList(uint32_t guid);
    void removeFromGMTargetList(uint32_t guid);
    bool isOnGMTargetList(uint32_t guid) const;

    bool isAtGroupRewardDistance(Object* pRewardSource);

private:
    uint16_t m_spellAreaUpdateTimer = 1000;
    uint16_t m_pendingPacketTimer = 100;
    uint16_t m_partyUpdateTimer = 1000;
    uint16_t m_durationUpdateTimer = 1000;

    std::string afkReason;

    std::vector<uint32_t> m_gmPlayerTargetList;
    mutable std::mutex m_lockGMTargetList;

#if VERSION_STRING > WotLK
    // Void Storage
public:
    void loadVoidStorage();
    void saveVoidStorage();

    bool isVoidStorageUnlocked() const { return hasPlayerFlags(PLAYER_FLAGS_VOID_UNLOCKED); }
    void unlockVoidStorage() { setPlayerFlags(PLAYER_FLAGS_VOID_UNLOCKED); }
    void lockVoidStorage() { removePlayerFlags(PLAYER_FLAGS_VOID_UNLOCKED); }
    uint8_t getNextVoidStorageFreeSlot() const;
    uint8_t getNumOfVoidStorageFreeSlots() const;
    uint8_t addVoidStorageItem(const VoidStorageItem& item);
    void addVoidStorageItemAtSlot(uint8_t slot, const VoidStorageItem& item);
    void deleteVoidStorageItem(uint8_t slot);
    bool swapVoidStorageItem(uint8_t oldSlot, uint8_t newSlot);
    VoidStorageItem* getVoidStorageItem(uint8_t slot) const;
    VoidStorageItem* getVoidStorageItem(uint64_t id, uint8_t& slot) const;

    VoidStorageItem* _voidStorageItems[VOID_STORAGE_MAX_SLOT];
#endif

public:
    //MIT End
    //AGPL Start

    friend class WorldSession;
    friend class Pet;

    public:

        bool Teleport(const LocationVector& vec, MapMgr* map) override;

        void EventGroupFullUpdate();

        // Skill System
        void _AdvanceSkillLine(uint32 SkillLine, uint32 Count = 1);
        void _AddSkillLine(uint32 SkillLine, uint32 Current, uint32 Max);
        uint32 _GetSkillLineMax(uint32 SkillLine);
        uint32 _GetSkillLineCurrent(uint32 SkillLine, bool IncludeBonus = true);
        void _RemoveSkillLine(uint32 SkillLine);
        void _UpdateMaxSkillCounts();
        void _ModifySkillBonus(uint32 SkillLine, int32 Delta);
        void _ModifySkillBonusByType(uint32 SkillType, int32 Delta);
        bool _HasSkillLine(uint32 SkillLine);
        void RemoveSpellsFromLine(uint32 skill_line);
        void _RemoveAllSkills();
        void _RemoveLanguages();
        void _AdvanceAllSkills(uint32 count);
        void _ModifySkillMaximum(uint32 SkillLine, uint32 NewMax);
        void _LearnSkillSpells(uint32 SkillLine, uint32 Current);

        void UpdatePvPCurrencies();
        void FillRandomBattlegroundReward(bool wonBattleground, uint32 &honorPoints, uint32 &arenaPoints);
        void ApplyRandomBattlegroundReward(bool wonBattleground);

    protected:
        void _UpdateSkillFields();

        SkillMap m_skills;

        // COOLDOWNS
        uint32 m_lastPotionId = 0;
        PlayerCooldownMap m_cooldownMap[NUM_COOLDOWN_TYPES];
        uint32 m_globalCooldown = 0;

        /***********************************************************************************
            AFTER THIS POINT, public and private ARE PASSED AROUND LIKE A CHEAP WH*RE :P
            Let's keeps thing clean (use encapsulation) above this line. Thanks.
        ***********************************************************************************/

    public:
        void SetLastPotion(uint32 itemid) { m_lastPotionId = itemid; }
        void Cooldown_AddItem(ItemProperties const* pProto, uint32 x);
        bool Cooldown_CanCast(ItemProperties const* pProto, uint32 x);
        void UpdatePotionCooldown();
        bool HasSpellWithAuraNameAndBasePoints(uint32 auraname, uint32 basepoints);

    protected:

        void AddCategoryCooldown(uint32 category_id, uint32 time, uint32 SpellId, uint32 ItemId);
        void _Cooldown_Add(uint32 Type, uint32 Misc, uint32 Time, uint32 SpellId, uint32 ItemId);
        void _LoadPlayerCooldowns(QueryResult* result);
        void _SavePlayerCooldowns(QueryBuffer* buf);

        // END COOLDOWNS
    public:

        void RemoveItemByGuid(uint64 GUID);

        //! Okay to remove from world
        bool ok_to_remove = false;

        void SendEquipmentSetList();
        void SendEquipmentSetSaved(uint32 setID, uint32 setGUID);
        
        void SendEmptyPetSpellList();

        void SendInitialWorldstates();

        void OutPacket(uint16 opcode, uint16 len, const void* data);
        void SendPacket(WorldPacket* packet);
        void SendMessageToSet(WorldPacket* data, bool self, bool myteam_only = false);
        void OutPacketToSet(uint16 Opcode, uint16 Len, const void* Data, bool self);

        static void CharChange_Looks(uint64 GUID, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair);
        static void CharChange_Language(uint64 GUID, uint8 race);

        void RemoveFromWorld();
        bool Create(CharCreate& charCreateContent);

        void BuildFlagUpdateForNonGroupSet(uint32 index, uint32 flag);
        void BuildPetSpellList(WorldPacket & data);

        void GiveXP(uint32 xp, const uint64 & guid, bool allowbonus);       /// to stop rest xp being given
        void ModifyBonuses(uint32 type, int32 val, bool apply);
        void CalcExpertise();
        std::map<uint32, uint32> m_wratings;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Taxi
        /////////////////////////////////////////////////////////////////////////////////////////
        TaxiPath* GetTaxiPath() { return m_CurrentTaxiPath; }
        bool isOnTaxi() const { return m_onTaxi; }
        const uint32 & GetTaximask(uint32_t index) const { return m_taximask[index]; }
        void LoadTaxiMask(const char* data);
        void TaxiStart(TaxiPath* path, uint32 modelid, uint32 start_node);
        void JumpToEndTaxiNode(TaxiPath* path);
        void EventDismount(uint32 money, float x, float y, float z);
        void EventTaxiInterpolate();

        void SetTaxiState(bool state) { m_onTaxi = state; }
        void SetTaximask(uint32_t index, uint32 value) { m_taximask[index] = value; }
        void SetTaxiPath(TaxiPath* path) { m_CurrentTaxiPath = path; }
        void SetTaxiPos() { m_taxi_pos_x = m_position.x; m_taxi_pos_y = m_position.y; m_taxi_pos_z = m_position.z; }
        void UnSetTaxiPos() { m_taxi_pos_x = 0; m_taxi_pos_y = 0; m_taxi_pos_z = 0; }

        // Taxi related variables
        std::vector<TaxiPath*> m_taxiPaths;
        TaxiPath* m_CurrentTaxiPath = nullptr;
        uint32 taxi_model_id = 0;
        uint32 lastNode = 0;
        uint32 m_taxi_ride_time = 0;
        uint32_t m_taximask[DBC_TAXI_MASK_SIZE];
        float m_taxi_pos_x = 0.0f;
        float m_taxi_pos_y = 0.0f;
        float m_taxi_pos_z = 0.0f;
        bool m_onTaxi = false;
        uint32 m_taxiMapChangeNode = 0;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Quests
        /////////////////////////////////////////////////////////////////////////////////////////
        uint32 GetQuestSharer() { return m_questSharer; }

        void SetQuestSharer(uint32 guid) { m_questSharer = guid; }

        void PushToRemovedQuests(uint32 questid) { m_removequests.insert(questid);}
        
        void AddToFinishedQuests(uint32 quest_id);
        void AreaExploredOrEventHappens(uint32 questId);   // scriptdev2

        bool HasFinishedQuest(uint32 quest_id);

        void EventTimedQuestExpire(uint32 questid);

        void ClearQuest(uint32 id);

        bool GetQuestRewardStatus(uint32 quest_id);
        bool HasQuestForItem(uint32 itemid);
        bool HasQuestSpell(uint32 spellid);
        void RemoveQuestSpell(uint32 spellid);
        bool HasQuestMob(uint32 entry);
        
        void RemoveQuestMob(uint32 entry);
        void AddQuestKill(uint32 questid, uint8 reqid, uint32 delay = 0);

        void AcceptQuest(uint64 guid, uint32 quest_id);

        std::set<uint32> m_removequests;
        std::set<uint32> m_finishedQuests;
        uint32 m_questSharer = 0;
        std::set<uint32> quest_spells;
        std::set<uint32> quest_mobs;

        void EventPortToGM(Player* p);

        /////////////////////////////////////////////////////////////////////////////////////////
        // Spells
        /////////////////////////////////////////////////////////////////////////////////////////
        bool HasSpell(uint32 spell);
        bool HasDeletedSpell(uint32 spell);
        void smsg_InitialSpells();
        void addSpell(uint32 spell_idy);
        bool removeSpell(uint32 SpellID, bool MoveToDeleted, bool SupercededSpell, uint32 SupercededSpellID);
        bool removeDeletedSpell(uint32 SpellID);
        void SendPreventSchoolCast(uint32 SpellSchool, uint32 unTimeMs);

        // PLEASE DO NOT INLINE!
        void AddOnStrikeSpell(SpellInfo const* sp, uint32 delay)
        {
            m_onStrikeSpells.insert(std::map<SpellInfo const*, std::pair<uint32, uint32>>::value_type(sp, std::make_pair(delay, 0)));
        }
        void RemoveOnStrikeSpell(SpellInfo const* sp)
        {
            m_onStrikeSpells.erase(sp);
        }
        void AddOnStrikeSpellDamage(uint32 spellid, uint32 mindmg, uint32 maxdmg)
        {
            OnHitSpell sp;
            sp.spellid = spellid;
            sp.mindmg = mindmg;
            sp.maxdmg = maxdmg;
            m_onStrikeSpellDmg[spellid] = sp;
        }
        void RemoveOnStrikeSpellDamage(uint32 spellid)
        {
            m_onStrikeSpellDmg.erase(spellid);
        }

        //Spells variables
        StrikeSpellMap m_onStrikeSpells;
        StrikeSpellDmgMap m_onStrikeSpellDmg;
        SpellOverrideMap mSpellOverrideMap;
        SpellSet mSpells;
        SpellSet mDeletedSpells;
        SpellSet mShapeShiftSpells;

        void AddShapeShiftSpell(uint32 id);
        void RemoveShapeShiftSpell(uint32 id);

        bool IsInFeralForm()
        {
            //\todo shapeshiftform is never negative.
            int s = getShapeShiftForm();
            if (s <= 0)
                return false;

            // Fight forms that do not use player's weapon
            return (s == FORM_BEAR || s == FORM_DIREBEAR || s == FORM_CAT);     //Shady: actually ghostwolf form doesn't use weapon too.
        }

        void CalcDamage();
        float offhand_dmg_mod = 0.5f;

        int32 GetDamageDoneMod(uint16_t school)
        {
            if (school >= TOTAL_SPELL_SCHOOLS)
                return 0;

            return static_cast<int32>(getModDamageDonePositive(school)) - static_cast<int32>(getModDamageDoneNegative(school));
        }

        float GetDamageDonePctMod(uint16_t school)
        {
            if (school >= TOTAL_SPELL_SCHOOLS)
                return 0;

            return getModDamageDonePct(static_cast<uint8_t>(school));
        }

        uint32 GetMainMeleeDamage(uint32 AP_owerride);          // I need this for windfury
        /////////////////////////////////////////////////////////////////////////////////////////
        // Reputation
        /////////////////////////////////////////////////////////////////////////////////////////
        void ModStanding(uint32 Faction, int32 Value);
        int32 GetStanding(uint32 Faction);
        int32 GetBaseStanding(uint32 Faction);
        void SetStanding(uint32 Faction, int32 Value);
        void SetAtWar(uint32 Faction, bool Set);

        Standing GetStandingRank(uint32 Faction);
        bool IsHostileBasedOnReputation(DBC::Structures::FactionEntry const* dbc);
        void UpdateInrangeSetsBasedOnReputation();
        void Reputation_OnKilledUnit(Unit* pUnit, bool InnerLoop);
        void Reputation_OnTalk(DBC::Structures::FactionEntry const* dbc);
        static Standing GetReputationRankFromStanding(int32 Standing_);
        void SetFactionInactive(uint32 faction, bool set);
        bool AddNewFaction(DBC::Structures::FactionEntry const* dbc, int32 standing, bool base);
        void OnModStanding(DBC::Structures::FactionEntry const* dbc, FactionReputation* rep);
        uint32 GetExaltedCount();

        // Factions
        void smsg_InitialFactions();
        uint32 GetInitialFactionId();
        // factions variables
        int32 pctReputationMod = 0;
        FactionReputation* reputationByListId[128] = {nullptr};

        /////////////////////////////////////////////////////////////////////////////////////////
        // PVP
        /////////////////////////////////////////////////////////////////////////////////////////
        uint32 GetMaxPersonalRating();

        bool HasTitle(RankTitles title)
        {
#if VERSION_STRING > Classic
            const auto index = static_cast<uint8_t>(title / 32);

            return (getKnownTitles(index) & 1ULL << static_cast<uint64_t>((title % 32))) != 0;
#else
            return false;
#endif
        }
        void SetKnownTitle(RankTitles title, bool set);
        void SendAvailSpells(DBC::Structures::SpellShapeshiftFormEntry const* shapeshift_form, bool active);

        /////////////////////////////////////////////////////////////////////////////////////////
        // Duel
        /////////////////////////////////////////////////////////////////////////////////////////
        void RequestDuel(Player* pTarget);
        void DuelBoundaryTest();
        void EndDuel(uint8 WinCondition);
        void DuelCountdown();
        void cancelDuel();
        void SetDuelStatus(uint8 status) { m_duelStatus = status; }
        uint8 GetDuelStatus() { return m_duelStatus; }
        void SetDuelState(uint8 state) { m_duelState = state; }
        uint8 GetDuelState() { return m_duelState; }

        Player* DuelingWith = nullptr;
        uint32 m_duelCountdownTimer = 0;
        uint8 m_duelStatus = 0;
        uint8 m_duelState = DUEL_STATE_FINISHED;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Pets/Summons
        /////////////////////////////////////////////////////////////////////////////////////////
        void AddSummon(Pet* pet) { m_Summons.push_front(pet); }
        Pet* GetSummon()     // returns 1st summon
        {
            if (!m_Summons.empty())
                return m_Summons.front();

            return nullptr;
        }
        std::list<Pet*> GetSummons() { return m_Summons; }

        void RemoveSummon(Pet* pet);
        uint32 GeneratePetNumber();
        void RemovePlayerPet(uint32 pet_number);
        void AddPlayerPet(PlayerPet* pet, uint32 index) { m_Pets[index] = pet; }
        PlayerPet* GetPlayerPet(uint32 idx)
        {
            std::map<uint32, PlayerPet*>::iterator itr = m_Pets.find(idx);
            if (itr != m_Pets.end())
                return itr->second;

            return nullptr;
        }
        void SpawnPet(uint32 pet_number);
        void SpawnActivePet();
        void DismissActivePets();
        uint8 GetPetCount() { return (uint8)m_Pets.size(); }
        void SetStableSlotCount(uint8 count) { m_StableSlotCount = count; }
        uint8 GetStableSlotCount() { return m_StableSlotCount; }

        uint32 GetUnstabledPetNumber();
        void EventSummonPet(Pet* new_pet);   // if we charmed or simply summoned a pet, this function should get called
        void EventDismissPet();              // if pet/charm died or whatever happened we should call this function

        Object* GetSummonedObject() { return m_SummonedObject; };
        void SetSummonedObject(Object* t_SummonedObject) { m_SummonedObject = t_SummonedObject; };

    protected:
        std::list<Pet*>  m_Summons;
        uint8 m_StableSlotCount = 0;
        uint32 m_PetNumberMax = 0;
        std::map<uint32, PlayerPet*> m_Pets;

        Object* m_SummonedObject = nullptr;
    
        /////////////////////////////////////////////////////////////////////////////////////////
        // Item Interface
        /////////////////////////////////////////////////////////////////////////////////////////
    public:
        void ApplyItemMods(Item* item, int16 slot, bool apply, bool justdrokedown = false) { _ApplyItemMods(item, slot, apply, justdrokedown); }
    protected:
        void _ApplyItemMods(Item* item, int16 slot, bool apply, bool justdrokedown = false, bool skip_stat_apply = false);

        /////////////////////////////////////////////////////////////////////////////////////////
        // Loot
        /////////////////////////////////////////////////////////////////////////////////////////
    public:
        const uint64 & GetLootGUID() const { return m_lootGuid; }
        void SetLootGUID(const uint64 & guid) { m_lootGuid = guid; }
        void sendLooter(Creature* creature);
        void SendLoot(uint64 guid, uint8 loot_type, uint32 mapid);
        void SendLootUpdate(Object* o);
        void TagUnit(Object* o);
        
        // loot variables
        uint64 m_lootGuid = 0;
        uint64 m_currentLoot = 0;
        bool bShouldHaveLootableOnCorpse = false;

        Item* storeNewLootItem(uint8_t slot, Loot* loot);
        Item* storeItem(LootItem const* lootItem);

        /////////////////////////////////////////////////////////////////////////////////////////
        // World Session
        /////////////////////////////////////////////////////////////////////////////////////////
        WorldSession* GetSession() const { return m_session; }
        void SetSession(WorldSession* s) { m_session = s; }

        void SendDelayedPacket(WorldPacket* data, bool bDeleteOnSend);
protected:
        WorldSession* m_session = nullptr;
public:

        // Talents
        // These functions build a specific type of A9 packet
        uint32 buildCreateUpdateBlockForPlayer(ByteBuffer* data, Player* target);
        void SetTalentHearthOfWildPCT(int value) { hearth_of_wild_pct = value; }
        void EventTalentHearthOfWildChange(bool apply);

        std::list<LoginAura> loginauras;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Player loading and savings Serialize character to db
        /////////////////////////////////////////////////////////////////////////////////////////
        void SaveToDB(bool bNewCharacter);
        void SaveAuras(std::stringstream &);
        bool LoadFromDB(uint32 guid);
        void LoadFromDBProc(QueryResultVector & results);

        bool SaveSpells(bool NewCharacter, QueryBuffer* buf);

        bool LoadDeletedSpells(QueryResult* result);
        bool SaveDeletedSpells(bool NewCharacter, QueryBuffer* buf);

        bool SaveReputations(bool NewCharacter, QueryBuffer *buf);

        bool LoadSkills(QueryResult* result);
        bool SaveSkills(bool NewCharacter, QueryBuffer* buf);

        bool m_FirstLogin = false;
protected:
        ReputationMap m_reputation;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Drunk system
        /////////////////////////////////////////////////////////////////////////////////////////
public:
        void SetDrunkValue(uint16 newDrunkValue, uint32 itemid = 0);
        uint16 GetDrunkValue() const { return m_drunk; }
        static DrunkenState GetDrunkenstateByValue(uint16 value);
        void HandleSobering();

        uint32 m_drunkTimer = 0;
        uint16 m_drunk = 0;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Death system
        /////////////////////////////////////////////////////////////////////////////////////////
        void SpawnCorpseBody();
        void SpawnCorpseBones();
        void CreateCorpse();
        void KillPlayer();
        void ResurrectPlayer();
        void BuildPlayerRepop();
        void RepopRequestedPlayer();
        void DeathDurabilityLoss(double percent);
        void RepopAtGraveyard(float ox, float oy, float oz, uint32 mapid);

        uint64 m_resurrecter = 0;

        /////////////////////////////////////////////////////////////////////////////////////////
        // Talent Specs
        /////////////////////////////////////////////////////////////////////////////////////////
        uint16 m_maxTalentPoints = 0;
        uint8 m_talentSpecsCount = 1;
        uint8 m_talentActiveSpec = 0;
#if VERSION_STRING >= Cata
        uint32 m_FirstTalentTreeLock = 0;
#endif

#ifdef FT_DUAL_SPEC
        PlayerSpec m_specs[MAX_SPEC_COUNT];
#else
        PlayerSpec m_spec;
#endif

        PlayerSpec& getActiveSpec();

#if VERSION_STRING > TBC
        uint32 GetGlyph(uint32 spec, uint32 slot) const { return m_specs[spec].glyphs[slot]; }
#endif

        /////////////////////////////////////////////////////////////////////////////////////////
        // Attack stuff
        /////////////////////////////////////////////////////////////////////////////////////////
public:
        void EventAttackStart();
        void EventAttackStop();
        void EventAttackUpdateSpeed() { }
        void EventDeath();

        /////////////////////////////////////////////////////////////////////////////////////////
        // Battleground stuff
        /////////////////////////////////////////////////////////////////////////////////////////
        bool QueuedForRbg();
        void SetQueuedForRbg(bool value);
        bool HasWonRbgToday();
        void SetHasWonRbgToday(bool value);

        CBattleground* m_bg = nullptr;
        CBattleground* m_pendingBattleground = nullptr;
        bool m_bgHasFlag = false;
        bool m_bgIsQueued = false;
        uint32 m_bgQueueType = 0;
        uint32 m_bgQueueInstanceId = 0;

    protected:
        bool m_bgIsRbg = false;
        bool m_bgIsRbgWon = false;


        /////////////////////////////////////////////////////////////////////////////////////////
        // Visible objects
        /////////////////////////////////////////////////////////////////////////////////////////
    public:
        bool IsVisible(uint64 pObj) { return !(m_visibleObjects.find(pObj) == m_visibleObjects.end()); }
        void addToInRangeObjects(Object* pObj);
        void onRemoveInRangeObject(Object* pObj);
        void clearInRangeSets();
        void AddVisibleObject(uint64 pObj) { m_visibleObjects.insert(pObj); }
        void RemoveVisibleObject(uint64 pObj) { m_visibleObjects.erase(pObj); }
        void RemoveVisibleObject(std::set< uint64 >::iterator itr) { m_visibleObjects.erase(itr); }
        std::set< uint64 >::iterator FindVisible(uint64 obj) { return m_visibleObjects.find(obj); }
        void RemoveIfVisible(uint64 obj);
protected:
        std::set<uint64> m_visibleObjects;

public:
        /////////////////////////////////////////////////////////////////////////////////////////
        // rest
        /////////////////////////////////////////////////////////////////////////////////////////
        uint32 SubtractRestXP(uint32 amount);
        void AddCalculatedRestXP(uint32 seconds);
        void ApplyPlayerRestState(bool apply);
        void UpdateRestState();
protected:
        uint8 m_isResting = 0;
        uint8 m_restState = 0;
        uint32 m_restAmount = 0;
public:

        /////////////////////////////////////////////////////////////////////////////////////////
        //  PVP Stuff
        /////////////////////////////////////////////////////////////////////////////////////////
        uint32 m_pvpTimer = 0;

        void AddHonor(uint32 honorPoints, bool sendUpdate);
        void UpdateHonor();

        void AddArenaPoints(uint32 arenaPoints, bool sendUpdate);
        void UpdateArenaPoints();

        // Do this on /pvp off
        void ResetPvPTimer();
        // Stop the timer for pvp off
        void StopPvPTimer() { m_pvpTimer = 0; }

        // Called at login to add the honorless buff, etc.
        void LoginPvPSetup();
        // Update our pvp area (called when zone changes)
        void UpdatePvPArea();
        // PvP Toggle (called on /pvp)
        void PvPToggle();




        //Note:ModSkillLine -> value+=amt;ModSkillMax -->value=amt; --weird
        float GetSkillUpChance(uint32 id);

        float SpellHasteRatingBonus = 1.0f;
        void UpdateAttackSpeed();
        float GetDefenseChance(uint32 opLevel);
        float GetDodgeChance();
        float GetBlockChance();
        float GetParryChance();
        void UpdateChances();
        void UpdateStats();
        uint32 GetBlockDamageReduction();
        void ApplyFeralAttackPower(bool apply, Item* item = NULL);

        float GetSpellCritFromSpell() { return m_spellcritfromspell; }
        float GetHitFromSpell() { return m_hitfromspell; }
        void SetSpellCritFromSpell(float value) { m_spellcritfromspell = value; }
        void SetHitFromSpell(float value) { m_hitfromspell = value; }

        uint32 GetHealthFromSpell() { return m_healthfromspell; }
        uint32 GetManaFromSpell() { return m_manafromspell; }
        void SetHealthFromSpell(uint32 value) { m_healthfromspell = value;}
        void SetManaFromSpell(uint32 value) { m_manafromspell = value;}

        uint32 CalcTalentResetCost(uint32 resetnum);
        
        uint32 GetTalentResetTimes() { return m_talentresettimes; }
        void SetTalentResetTimes(uint32 value) { m_talentresettimes = value; }

        uint32 m_nextSave;

        int m_lifetapbonus = 0;         //warlock spell related
        bool m_requiresNoAmmo = false;      //warlock spell related

        // Misc
        void EventCannibalize(uint32 amount);

        uint32 m_modblockabsorbvalue = 0;
        uint32 m_modblockvaluefromspells = 0;
        void SendInitialLogonPackets();
        void Reset_Spells();

        void _InitialReputation();
        void EventActivateGameObject(GameObject* obj);
        void EventDeActivateGameObject(GameObject* obj);
        void UpdateNearbyGameObjects();

        void CalcResistance(uint8_t type);
        float res_M_crit_get() { return m_resist_critical[0]; }
        void res_M_crit_set(float newvalue) { m_resist_critical[0] = newvalue; }
        float res_R_crit_get() { return m_resist_critical[1]; }
        void res_R_crit_set(float newvalue) { m_resist_critical[1] = newvalue; }
        uint32 FlatResistanceModifierPos[TOTAL_SPELL_SCHOOLS] = {0};
        uint32 FlatResistanceModifierNeg[TOTAL_SPELL_SCHOOLS] = {0};
        uint32 BaseResistanceModPctPos[TOTAL_SPELL_SCHOOLS] = {0};
        uint32 BaseResistanceModPctNeg[TOTAL_SPELL_SCHOOLS] = {0};
        uint32 ResistanceModPctPos[TOTAL_SPELL_SCHOOLS] = {0};
        uint32 ResistanceModPctNeg[TOTAL_SPELL_SCHOOLS] = {0};
        float m_resist_critical[2] = {0};             // when we are a victim we can have talents to decrease chance for critical hit. This is a negative value and it's added to critchances
        float m_resist_hit[2] = {0};                  // 0 = melee; 1= ranged;
        int32 m_resist_hit_spell[TOTAL_SPELL_SCHOOLS] = {0}; // spell resist per school
        uint32 m_modphyscritdmgPCT = 0;
        uint32 m_RootedCritChanceBonus = 0;         // Class Script Override: Shatter
        uint32 m_IncreaseDmgSnaredSlowed = 0;

        // SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
        uint32 m_ModInterrMRegenPCT = 0;
        // SPELL_AURA_MOD_POWER_REGEN
        int32 m_ModInterrMRegen = 0;
        // SPELL_AURA_REGEN_MANA_STAT_PCT
        int32_t m_modManaRegenFromStat[STAT_COUNT] = {0};
        float m_RegenManaOnSpellResist = 0.0f;
        uint32 m_casted_amount[TOTAL_SPELL_SCHOOLS] = {0};   // Last casted spells amounts. Need for some spells. Like Ignite etc. DOesn't count HoTs and DoTs. Only directs

        uint32 FlatStatModPos[5] = {0};
        uint32 FlatStatModNeg[5] = {0};
        uint32 StatModPctPos[5] = {0};
        uint32 StatModPctNeg[5] = {0};
        uint32 TotalStatModPctPos[5] = {0};
        uint32 TotalStatModPctNeg[5] = {0};
        int32 IncreaseDamageByType[12] = {0};         // mod dmg by creature type
        float IncreaseDamageByTypePCT[12] = {0};
        float IncreaseCricticalByTypePCT[12] = {0};
        int32 DetectedRange = 0;
        float PctIgnoreRegenModifier = 0.0f;
        uint32 m_retainedrage = 0;                  // Warrior spell related

        uint32* GetPlayedtime() { return m_playedtime; };
        void CalcStat(uint8_t t);
        float CalcRating(PlayerCombatRating t);
        void RegenerateHealth(bool inCombat);

        uint64 misdirectionTarget = 0;              // Hunter spell related

        uint64 GetMisdirectionTarget() { return misdirectionTarget; }
        void SetMisdirectionTarget(uint64 PlayerGUID) { misdirectionTarget = PlayerGUID; }

        bool bReincarnation = false;                    // Shaman spell related

        std::map<uint32, WeaponModifier> damagedone;
        std::map<uint32, WeaponModifier> tocritchance;
        bool cannibalize = false;
        uint8 cannibalizeCount = 0;
        int32 rageFromDamageDealt = 0;
        int32 rageFromDamageTaken = 0;

        void _Relocate(uint32 mapid, const LocationVector & v, bool sendpending, bool force_new_world, uint32 instance_id);

        void AddItemsToWorld();
        void RemoveItemsFromWorld();
        void UpdateKnownCurrencies(uint32 itemId, bool apply);

        uint32 m_MountSpellId = 0;
        uint32 mountvehicleid = 0;

        bool IsMounted();

        void Dismount()
        {
            if (m_MountSpellId != 0)
            {
                RemoveAura(m_MountSpellId);
                m_MountSpellId = 0;
            }
        }

        bool isVehicle() const override
        {
            if (mountvehicleid != 0)
                return true;

            return false;
        }

        void addVehicleComponent(uint32 creature_entry, uint32 vehicleid);

        void removeVehicleComponent();

        uint32 TrackingSpell = 0;
        void _EventCharmAttack();
        
        void SoftDisconnect();

        void ClearCooldownsOnLine(uint32 skill_line, uint32 called_from);

        void Phase(uint8 command = PHASE_SET, uint32 newphase = 1);

        void ProcessPendingUpdates();
        bool CompressAndSendUpdateBuffer(uint32 size, const uint8* update_buffer);

        bool ExitInstance();
        void SaveEntryPoint(uint32 mapId);

        // Cheat section
        void SpeedCheatDelay(uint32 ms_delay);
        void SpeedCheatReset();

        void ZoneUpdate(uint32 ZoneId);

        // Instance IDs
        uint32 GetPersistentInstanceId(uint32 mapId, uint8 difficulty)
        {
            if (mapId >= MAX_NUM_MAPS || difficulty >= InstanceDifficulty::MAX_DIFFICULTY || m_playerInfo == NULL)
                return 0;

            std::lock_guard<std::mutex> lock(m_playerInfo->savedInstanceIdsLock);
            PlayerInstanceMap::iterator itr = m_playerInfo->savedInstanceIds[difficulty].find(mapId);
            if (itr == m_playerInfo->savedInstanceIds[difficulty].end())
            {
                return 0;
            }

            return (*itr).second;
        }

        void SetPersistentInstanceId(Instance* pInstance);
        //Use this method carefully..
        void SetPersistentInstanceId(uint32 mapId, uint8 difficulty, uint32 instanceId);

    public:

        bool m_Autojoin = false;
        bool m_AutoAddMem = false;
        void SendMirrorTimer(MirrorTimerTypes Type, uint32 max, uint32 current, int32 regen);
        
        BGScore m_bgScore;
        
        void UpdateChanceFields();
        //Honor Variables
        time_t m_fallDisabledUntil = 0;
        uint32 m_honorToday = 0;
        uint32 m_honorYesterday = 0;

        void RolloverHonor();
        uint32 m_honorPoints = 0;
        uint32 m_honorRolloverTime = 0;
        uint32 m_killsToday = 0;
        uint32 m_killsYesterday = 0;
        uint32 m_killsLifetime = 0;
        uint32 m_arenaPoints = 0;
        uint32 m_honorless = 0;
        uint32 m_lastSeenWeather = 0;
        std::set<Object*> m_visibleFarsightObjects;
        void EventTeleport(uint32 mapid, float x, float y, float z);
        void EventTeleportTaxi(uint32 mapid, float x, float y, float z);
        void BroadcastMessage(const char* Format, ...);
        std::map<uint32, std::set<uint32> > SummonSpells;
        std::map<uint32, std::map<SpellInfo const*, uint16>*> PetSpells;
        void AddSummonSpell(uint32 Entry, uint32 SpellID);
        void RemoveSummonSpell(uint32 Entry, uint32 SpellID);
        std::set<uint32>* GetSummonSpells(uint32 Entry);

        uint32 m_UnderwaterLastDmg = Util::getMSTime();
        
        uint32 m_resurrectHealth = 0;
        uint32 m_resurrectMana = 0;
        uint32 m_resurrectInstanceID = 0;
        uint32 m_resurrectMapId = 0;
        LocationVector m_resurrectPosition;
        bool blinked = false;
        uint32 m_explorationTimer = Util::getMSTime();
        // DBC stuff
        DBC::Structures::ChrRacesEntry const* myRace = nullptr;
        DBC::Structures::ChrClassesEntry const* myClass = nullptr;

        bool SafeTeleport(uint32 MapID, uint32 InstanceID, float X, float Y, float Z, float O);
        bool SafeTeleport(uint32 MapID, uint32 InstanceID, const LocationVector & vec);
        void SafeTeleport(MapMgr* mgr, const LocationVector & vec);
        void EjectFromInstance();
        bool raidgrouponlysent = false;

        void EventSafeTeleport(uint32 MapID, uint32 InstanceID, const LocationVector vec)
        {
            SafeTeleport(MapID, InstanceID, vec);
        }

        // Hack fix here!
        void ForceZoneUpdate();

        bool HasAreaExplored(::DBC::Structures::AreaTableEntry const*);
        bool HasOverlayUncovered(uint32 overlayID);


        void HandleSpellLoot(uint32 itemid);

        void Die(Unit* pAttacker, uint32 damage, uint32 spellid) override;
        void handleKnockback(Object* caster, float horizontal, float vertical) override;

        uint32 LastHonorResetTime() const { return m_lastHonorResetTime; }
        void LastHonorResetTime(uint32 val) { m_lastHonorResetTime = val; }
        uint32 OnlineTime = static_cast<uint32_t>(UNIXTIME);
        bool tutorialsDirty = true;
        LevelInfo* lvlinfo = nullptr;
        uint32 load_health = 0;
        uint32 load_mana = 0;
        void CompleteLoading();
        void OnWorldPortAck();
        uint32 m_TeleportState = 1;
        bool m_beingPushed = false;
        
        uint32 flying_aura = 0;
        bool resend_speed = false;
        uint32 login_flags = LOGIN_NO_FLAG;

        uint32 m_speedChangeCounter = 1;

        void SendAreaTriggerMessage(const char* message, ...);

        void RemoteRevive();

        LocationVector m_last_group_position;
        int32 m_rap_mod_pct = 0;


        void SummonRequest(uint32 Requestor, uint32 ZoneID, uint32 MapID, uint32 InstanceID, const LocationVector & Position);
    protected:

        LocationVector m_summonPos;
        uint32 m_summonInstanceId = 0;
        uint32 m_summonMapId = 0;
        uint32 m_summoner = 0;
    public:


        bool m_deathVision = false;
        SpellInfo const* last_heal_spell = nullptr;
        LocationVector m_sentTeleportPosition;

        bool InBattleground() const { return m_bgQueueInstanceId != 0; }
        bool InBattlegroundQueue() const { return m_bgIsQueued != 0; }

        void RemoveFromBattlegroundQueue();
        void FullHPMP();
        void RemoveTempEnchantsOnArena();
        uint32 m_arenateaminviteguid = 0;

        void ResetTimeSync();
        void SendTimeSync();

        /////////////////////////////////////////////////////////////////////////////////////////
        // Spell Packet wrapper Please keep this separated
        /////////////////////////////////////////////////////////////////////////////////////////
        void SendWorldStateUpdate(uint32 WorldState, uint32 Value);

        /////////////////////////////////////////////////////////////////////////////////////////
        // End of SpellPacket wrapper
        /////////////////////////////////////////////////////////////////////////////////////////

        Mailbox m_mailBox;
        uint64 m_areaSpiritHealer_guid = 0;
        bool m_finishingmovesdodge = false;

        bool IsAttacking() { return m_attacking; }

        static void InitVisibleUpdateBits();
        static UpdateMask m_visibleUpdateMask;

        void CopyAndSendDelayedPacket(WorldPacket* data);
        
        SpeedCheatDetector* SDetector;

    protected:

        void _SetCreateBits(UpdateMask* updateMask, Player* target) const;
        void _SetUpdateBits(UpdateMask* updateMask, Player* target) const;

        void _SaveQuestLogEntry(QueryBuffer* buf);
        void _LoadQuestLogEntry(QueryResult* result);

        void _LoadPet(QueryResult* result);
        void _LoadPetSpells(QueryResult* result);
        void _SavePet(QueryBuffer* buf);
        void _SavePetSpells(QueryBuffer* buf);
        
        void _EventAttack(bool offhand);
        
        void CastSpellArea();

    // exploration
        void _EventExploration();

        /////////////////////////////////////////////////////////////////////////////////////////
        // Player Class systems, info and misc things
        /////////////////////////////////////////////////////////////////////////////////////////
        PlayerCreateInfo const* info = nullptr;
        uint32 m_AttackMsgTimer = 0;        // "too far away" and "wrong facing" timer
        bool m_attacking = false;

        uint32 m_invitersGuid = 0;      // It is guild inviters guid ,0 when its not used

        std::list<ItemSet> m_itemsets;

        // Rested State Stuff
        uint32 m_timeLogoff = 0;
        // Played time
        // 0 = played on level
        // 1 = played total
        // 2 = played session
        uint32 m_playedtime[3] = { 0, 0, static_cast<uint32_t>(UNIXTIME) };

        //combat mods
        float m_blockfromspellPCT = 0.0f;
        float m_critfromspell = 0.0f;
        float m_spellcritfromspell = 0.0f;
        float m_hitfromspell = 0.0f;
        //stats mods
        uint32 m_healthfromspell = 0;
        uint32 m_manafromspell = 0;
        uint32 m_healthfromitems = 0;
        uint32 m_manafromitems = 0;

        // Talents
        uint32 m_talentresettimes = 0;
        // Raid
        uint8 m_targetIcon = 0;

        uint32 m_lastHonorResetTime = 0;
        uint32 _fields[getSizeOfStructure(WoWPlayer)];
        int hearth_of_wild_pct = 0;        // druid hearth of wild talent used on shapeshifting. We either know what is last talent level or memo on learn

        uint32 m_indoorCheckTimer = 0;
        void RemovePendingPlayer();

    public:

        void addDeletedSpell(uint32 id) { mDeletedSpells.insert(id); }

        std::map<uint32, uint32> m_forcedReactions;

        bool m_passOnLoot = false;
        uint32 m_tradeSequence;
        bool m_changingMaps = true;

        uint32 m_outStealthDamageBonusPct = 0;
        uint32 m_outStealthDamageBonusPeriod = 0;
        uint32 m_outStealthDamageBonusTimer = 0;

    private:

        CachedCharacterInfo* m_playerInfo = nullptr;

        bool resettalents = false;
        std::list< Item* > m_GarbageItems;

        void RemoveGarbageItems();

        uint32 ChampioningFactionID = 0;

        uint32 m_timeSyncCounter = 0;
        uint32 m_timeSyncTimer = 0;
        uint32 m_timeSyncClient = 0;
        uint32 m_timeSyncServer = 0;

    public:

        void SetChampioningFaction(uint32 f) { ChampioningFactionID = f; }
        void AddGarbageItem(Item* it);
        uint32 CheckDamageLimits(uint32 dmg, uint32 spellid);

        CachedCharacterInfo* getPlayerInfo() const { return m_playerInfo; }

        void LoadFieldsFromString(const char* string, uint16 firstField, uint32 fieldsNum);

        // Avenging Wrath
        void AvengingWrath() { mAvengingWrath = true; }
        bool mAvengingWrath = true;

        void ToggleXpGain();
        bool CanGainXp();

#if VERSION_STRING > TBC
        AchievementMgr & GetAchievementMgr() { return m_achievementMgr; }
        AchievementMgr m_achievementMgr = this;
#endif

        void SendUpdateDataToSet(ByteBuffer* groupbuf, ByteBuffer* nongroupbuf, bool sendtoself);

        bool CanBuyAt(MySQLStructure::VendorRestrictions const* vendor);
        bool CanTrainAt(Trainer*);

        void SetRoles(uint8 role) { m_roles = role; }
        uint8 GetRoles() { return m_roles; }
        uint8 m_roles = 0;

        uint32 GetGroupUpdateFlags() { return GroupUpdateFlags; }
        void SetGroupUpdateFlags(uint32 flags);
        void AddGroupUpdateFlag(uint32 flag);
        uint16 GetGroupStatus();
        void SendUpdateToOutOfRangeGroupMembers();
        uint32 GroupUpdateFlags;

        void SendTeleportPacket(float x, float y, float z, float o);
        void SendTeleportAckPacket(float x, float y, float z, float o);

        void SendCinematicCamera(uint32 id);

        void SetMover(Unit* target);

        // AGPL End
};
