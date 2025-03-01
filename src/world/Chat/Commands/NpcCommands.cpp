/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/


#include "Chat/ChatHandler.hpp"
#include "Objects/Item.h"
#include "Objects/Units/Creatures/Creature.h"
#include "Objects/Units/Creatures/Summons/Summon.h"
#include "Storage/MySQLDataStore.hpp"
#include "Server/MainServerDefines.h"
#include "Map/MapMgr.h"
#include "Server/Script/CreatureAIScript.h"
#include "Spell/SpellMgr.hpp"
#include "Spell/Definitions/SpellEffects.hpp"

//.npc addagent
bool ChatHandler::HandleNpcAddAgentCommand(const char* args, WorldSession* m_session)
{
    //new
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint32 ai_type;
    uint32 procEvent;
    uint32 procChance;
    uint32 maxcount;
    uint32 spellId;
    uint32 spellType;
    uint32 spelltargetType;
    uint32 spellCooldown;
    float floatMisc1;
    uint32 Misc2;

    if (sscanf(args, "%u %u %u %u %u %u %u %u %f %u", &ai_type, &procEvent, &procChance, &maxcount, &spellId, &spellType, &spelltargetType, &spellCooldown, &floatMisc1, &Misc2) != 10)
    {
        RedSystemMessage(m_session, "Command must be in format: .npc add trainerspell <ai_type> <procEvent> <procChance> <maxcount> <spellId> <spellType> <spelltarget_overwrite> <spellCooldown> <floatMisc1> <Misc2>.");
        return true;
    }

    auto spell_entry = sSpellMgr.getSpellInfo(spellId);
    if (spell_entry == nullptr)
    {
        RedSystemMessage(m_session, "Spell %u is not invalid!", spellId);
        return true;
    }

    SystemMessage(m_session, "Added agent_type %u for spell %u to creature %s (%u).", ai_type, spellId, creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
    sGMLog.writefromsession(m_session, "added agent_type %u for spell %u to creature %s (%u).", ai_type, spellId, creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
    WorldDatabase.Execute("INSERT INTO ai_agents VALUES(%u, 4, %u, %u, %u, %u, %u, %u, %u, %u, %f, %u",
        creature_target->getEntry(), ai_type, procEvent, procChance, maxcount, spellId, spellType, spelltargetType, spellCooldown, floatMisc1, Misc2);


    AI_Spell* ai_spell = new AI_Spell;
    ai_spell->agent = static_cast<uint16>(ai_type);
    ai_spell->procChance = procChance;
    ai_spell->procCount = maxcount;
    ai_spell->spell = spell_entry;
    ai_spell->spellType = static_cast<uint8>(spellType);
    ai_spell->spelltargetType = static_cast<uint8>(spelltargetType);
    ai_spell->floatMisc1 = floatMisc1;
    ai_spell->Misc2 = Misc2;
    ai_spell->cooldown = spellCooldown;
    ai_spell->procCounter = 0;
    ai_spell->cooldowntime = 0;
    ai_spell->minrange = GetMinRange(sSpellRangeStore.LookupEntry(spell_entry->getRangeIndex()));
    ai_spell->maxrange = GetMaxRange(sSpellRangeStore.LookupEntry(spell_entry->getRangeIndex()));

    const_cast<CreatureProperties*>(creature_target->GetCreatureProperties())->spells.push_back(ai_spell);

    switch (ai_type)
    {
        case AGENT_MELEE:
            creature_target->getAIInterface()->setMeleeDisabled(false);
            break;
        case AGENT_RANGED:
            creature_target->getAIInterface()->m_canRangedAttack = true;
            break;
        case AGENT_FLEE:
            creature_target->getAIInterface()->m_canFlee = true;
            break;
        case AGENT_SPELL:
            creature_target->getAIInterface()->addSpellToList(ai_spell);
            break;
        case AGENT_CALLFORHELP:
            creature_target->getAIInterface()->m_canCallForHelp = true;
            break;
        default:
        {
            RedSystemMessage(m_session, "Invalid ai_type %u", ai_type);
            break;
        }
    }
    return true;
}

bool ChatHandler::HandleNpcAppearCommand(const char* /*_*/, WorldSession* session)
{
    const auto target = GetSelectedCreature(session);
    if (!target) {
        return true;
    }

    session->GetPlayer()->Teleport(target->GetPosition(), target->GetMapMgr());
    return true;
}

//.npc addtrainerspell
bool ChatHandler::HandleNpcAddTrainerSpellCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint32_t spellid;
    uint32_t cost;
    uint32_t reqlevel;
    uint32_t reqspell;
    uint32_t delspell;

    if (sscanf(args, "%u %u %u %u %u", &spellid, &cost, &reqspell, &reqlevel, &delspell) != 5)
    {
        RedSystemMessage(m_session, "Command must be in format: .npc add trainerspell <spell_id> <cost> <required_spell> <required_player_level> <delete_spell_id>.");
        return true;
    }

    auto creature_trainer = creature_target->GetTrainer();
    if (creature_trainer == nullptr)
    {
        RedSystemMessage(m_session, "%s (%u) is not a trainer!", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
        return true;
    }

    auto learn_spell = sSpellMgr.getSpellInfo(spellid);
    if (learn_spell == nullptr)
    {
        RedSystemMessage(m_session, "Invalid spell %u.", spellid);
        return true;
    }

    if (learn_spell->getEffect(0) == SPELL_EFFECT_INSTANT_KILL || learn_spell->getEffect(1) == SPELL_EFFECT_INSTANT_KILL || learn_spell->getEffect(2) == SPELL_EFFECT_INSTANT_KILL)
    {
        RedSystemMessage(m_session, "You are not allowed to learn spells with instant kill effect!");
        return true;
    }

    TrainerSpell sp;
    sp.cost = cost;
    sp.learnSpell = learn_spell;
    sp.requiredLevel = reqlevel;
    sp.requiredSpell[0] = reqspell;
    sp.deleteSpell = delspell;

    creature_trainer->Spells.push_back(sp);
    creature_trainer->SpellCount++;

    SystemMessage(m_session, "Added spell %s (%u) to trainer %s (%u).", learn_spell->getName().c_str(), learn_spell->getId(), creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
    sGMLog.writefromsession(m_session, "added spell  %s (%u) to trainer %s (%u)", learn_spell->getName().c_str(), learn_spell->getId(), creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
    WorldDatabase.Execute("REPLACE INTO trainer_spells VALUES(%u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
        creature_target->getEntry(), 0, learn_spell->getId(), cost, reqspell, 0, 0, reqlevel, delspell, 0);

    return true;
}

//.npc cast
bool ChatHandler::HandleNpcCastCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint32 spell_id;
    if (sscanf(args, "%u", &spell_id) != 1)
    {
        RedSystemMessage(m_session, "Command must be in format: .npc cast <spellid>.");
        return true;
    }

    auto spell_entry = sSpellMgr.getSpellInfo(spell_id);
    if (spell_entry == nullptr)
    {
        RedSystemMessage(m_session, "Invalid Spell ID: %u !", spell_id);
        return true;
    }

    auto unit_target = static_cast<Unit*>(creature_target);
    unit_target->castSpell(unit_target, spell_id, false);

    return true;
}

//.npc come
bool ChatHandler::HandleNpcComeCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    auto player = m_session->GetPlayer();
    creature_target->getMovementManager()->movePoint(0, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), false, player->GetOrientation());
    sGMLog.writefromsession(m_session, "used .npc come on %s spawn ID: %u", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);

    return true;
}

//.npc delete
bool ChatHandler::HandleNpcDeleteCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    if (creature_target->isPet())
    {
        SystemMessage(m_session, "You can't delete a pet.");
        return true;
    }

    bool save_to_db = atoi(args) == 1 ? true : false;
    if (creature_target->isSummon())
    {
        creature_target->Delete();
    }
    else
    {
        //creature_target->getAIInterface()->hideWayPoints(m_session->GetPlayer());

        uint32 spawn_id = creature_target->spawnid;

        if (m_session->GetPlayer()->m_saveAllChangesCommand)
            save_to_db = true;

        if (save_to_db && spawn_id != 0)
        {
            BlueSystemMessage(m_session, "Creature %s (%u) deleted from creature_spawn table.", creature_target->GetCreatureProperties()->Name.c_str(), spawn_id);
            sGMLog.writefromsession(m_session, "used npc delete on creature %s (%u), pos %f %f %f", creature_target->GetCreatureProperties()->Name.c_str(), spawn_id, creature_target->GetPositionX(), creature_target->GetPositionY(), creature_target->GetPositionZ());
            creature_target->DeleteFromDB();
        }

        if (!save_to_db)
        {
            BlueSystemMessage(m_session, "Creature %s temporarily deleted from world.", creature_target->GetCreatureProperties()->Name.c_str());
        }

        if (creature_target->m_spawn)
        {
            uint32 cellx = uint32(((_maxX - creature_target->m_spawn->x) / _cellSize));
            uint32 celly = uint32(((_maxY - creature_target->m_spawn->y) / _cellSize));

            if (cellx <= _sizeX && celly <= _sizeY)
            {
                CellSpawns* sp = creature_target->GetMapMgr()->GetBaseMap()->GetSpawnsList(cellx, celly);
                if (sp != nullptr)
                {
                    for (CreatureSpawnList::iterator itr = sp->CreatureSpawns.begin(); itr != sp->CreatureSpawns.end(); ++itr)
                        if ((*itr) == creature_target->m_spawn)
                        {
                            sp->CreatureSpawns.erase(itr);
                            break;
                        }
                }
                delete creature_target->m_spawn;
                creature_target->m_spawn = NULL;
            }
        }

        creature_target->RemoveFromWorld(false, true);
    }

    return true;
}

//.npc follow
bool ChatHandler::HandleNpcFollowCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    creature_target->getMovementManager()->moveFollow(m_session->GetPlayer(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    sGMLog.writefromsession(m_session, "used npc follow command on %s, sqlid %u", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
    return true;
}

//.npc info
bool ChatHandler::HandleNpcInfoCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint32 guid = WoWGuid::getGuidLowPartFromUInt64(m_session->GetPlayer()->getTargetGuid());

    SystemMessage(m_session, "Showing Creature info of %s =============", creature_target->GetCreatureProperties()->Name.c_str());
    RedSystemMessage(m_session, "EntryID: %d", creature_target->getEntry());
    RedSystemMessage(m_session, "SpawnID: %d", creature_target->GetSQL_id());
    SystemMessage(m_session, "GUID: %u", guid);
    SystemMessage(m_session, "Faction: %u", creature_target->getFactionTemplate());
    SystemMessage(m_session, "Phase: %u", creature_target->GetPhase());

    SystemMessage(m_session, "DisplayID: %u", creature_target->getDisplayId());

    uint8 creature_gender = creature_target->getGender();
    if (creature_gender <= 2)
        SystemMessage(m_session, "Gender: %s", GENDER[creature_gender]);
    else
        SystemMessage(m_session, "Gender: invalid %u", creature_gender);

    uint8 creature_class = creature_target->getClass();
    if (creature_class <= 11)
        SystemMessage(m_session, "Class: %s", CLASS[creature_class]);
    else
        SystemMessage(m_session, "Class: invalid %u", creature_class);

    SystemMessage(m_session, "Health (cur / max): %u / %u", creature_target->getHealth(), creature_target->getMaxHealth());

    auto powertype = creature_target->getPowerType();
    if (powertype <= 6)
    {
        SystemMessage(m_session, "Powertype: %s", POWERTYPE[powertype]);
        SystemMessage(m_session, "Power (cur / max): %u / %u", creature_target->getPower(powertype), creature_target->getMaxPower(powertype));
    }

    SystemMessage(m_session, "Damage (min / max): %f / %f", creature_target->getMinDamage(), creature_target->getMaxDamage());

    if (creature_target->getPetTalentPoints() != 0)
        SystemMessage(m_session, "Free pet talent points: %u", creature_target->getPetTalentPoints());

    if (creature_target->GetCreatureProperties()->vehicleid > 0)
        SystemMessage(m_session, "VehicleID: %u", creature_target->GetCreatureProperties()->vehicleid);

    if (creature_target->m_factionTemplate)
        SystemMessage(m_session, "Combat Support: 0x%.3X", creature_target->m_factionTemplate->FriendlyMask);

    if (creature_target->m_combatStatusHandler.IsInCombat())
        SystemMessage(m_session, "Is in combat!");
    else
        SystemMessage(m_session, "Not in combat!");

    uint8 sheat = creature_target->getSheathType();
    if (sheat <= 2)
        SystemMessage(m_session, "Sheat state: %s", SHEATSTATE[sheat]);

    SystemMessage(m_session, "=================================");

    //////////////////////////////////////////////////////////////////////////////////////////
    // resistance
    if (creature_target->getResistance(SCHOOL_NORMAL) || creature_target->getResistance(SCHOOL_HOLY) ||
        creature_target->getResistance(SCHOOL_FIRE) || creature_target->getResistance(SCHOOL_NATURE) ||
        creature_target->getResistance(SCHOOL_FROST) || creature_target->getResistance(SCHOOL_SHADOW) ||
        creature_target->getResistance(SCHOOL_ARCANE))
    {
        GreenSystemMessage(m_session, "Resistance =======================");
        if (creature_target->getResistance(SCHOOL_NORMAL))
            GreenSystemMessage(m_session, "-- Armor: %u", creature_target->getResistance(SCHOOL_NORMAL));
        if (creature_target->getResistance(SCHOOL_HOLY))
            GreenSystemMessage(m_session, "-- Holy: %u", creature_target->getResistance(SCHOOL_HOLY));
        if (creature_target->getResistance(SCHOOL_FIRE))
            GreenSystemMessage(m_session, "-- Fire: %u", creature_target->getResistance(SCHOOL_FIRE));
        if (creature_target->getResistance(SCHOOL_NATURE))
            GreenSystemMessage(m_session, "-- Nature: %u", creature_target->getResistance(SCHOOL_NATURE));
        if (creature_target->getResistance(SCHOOL_FROST))
            GreenSystemMessage(m_session, "-- Frost: %u", creature_target->getResistance(SCHOOL_FROST));
        if (creature_target->getResistance(SCHOOL_SHADOW))
            GreenSystemMessage(m_session, "-- Shadow: %u", creature_target->getResistance(SCHOOL_SHADOW));
        if (creature_target->getResistance(SCHOOL_ARCANE))
            GreenSystemMessage(m_session, "-- Arcane: %u", creature_target->getResistance(SCHOOL_ARCANE));
        GreenSystemMessage(m_session, "=================================");
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // show byte
    std::stringstream sstext;
    uint32 theBytes = creature_target->getBytes0();
    sstext << "UNIT_FIELD_BYTES_0 are:" << '\n';
    sstext << " -Race: " << uint16((uint8)theBytes & 0xFF) << '\n';
    sstext << " -Class: " << uint16((uint8)(theBytes >> 8) & 0xFF) << '\n';
    sstext << " -Gender: " << uint16((uint8)(theBytes >> 16) & 0xFF) << '\n';
    sstext << " -Power Type: " << uint16((uint8)(theBytes >> 24) & 0xFF) << '\n';
    sstext << '\n';

    theBytes = creature_target->getBytes1();
    sstext << "UNIT_FIELD_BYTES_1 are:" << '\n';
    sstext << " -StandState: " << uint16((uint8)theBytes & 0xFF) << '\n';
    sstext << " -Pet TP: " << uint16((uint8)(theBytes >> 8) & 0xFF) << '\n';
    sstext << " -StandState Flag: " << uint16((uint8)(theBytes >> 16) & 0xFF) << '\n';
    sstext << " -Animation Flag: " << uint16((uint8)(theBytes >> 24) & 0xFF) << '\n';
    sstext << '\n';

    theBytes = creature_target->getBytes2();
    sstext << "UNIT_FIELD_BYTES_2 are:" << '\n';
    sstext << " -SheathType: " << uint16((uint8)theBytes & 0xFF) << '\n';
    sstext << " -PvP Flag: " << uint16((uint8)(theBytes >> 8) & 0xFF) << '\n';
    sstext << " -Pet Flag: " << uint16((uint8)(theBytes >> 16) & 0xFF) << '\n';
    sstext << " -ShapeShift Form: " << uint16((uint8)(theBytes >> 24) & 0xFF) << '\n';
    sstext << '\0';

    SystemMessage(m_session, "UNIT_FIELD_BYTES =================");
    SendMultilineMessage(m_session, sstext.str().c_str());
    SystemMessage(m_session, "=================================");

    //////////////////////////////////////////////////////////////////////////////////////////
    // flags
    GreenSystemMessage(m_session, "Flags ============================");
    std::string s = GetNpcFlagString(creature_target);
    GreenSystemMessage(m_session, "NpcFlags: %u%s", creature_target->getNpcFlags(), s.c_str());

    uint8 pvp_flags = creature_target->getPvpFlags();
    GreenSystemMessage(m_session, "PvPFlags: %u", pvp_flags);

    for (uint32 i = 0; i < numpvpflags; i++)
        if ((pvp_flags & UnitPvPFlagToName[i].Flag) != 0)
            GreenSystemMessage(m_session, "%s", UnitPvPFlagToName[i].Name);

    uint8 pet_flags = creature_target->getPetFlags();
    if (pet_flags != 0)
    {
        GreenSystemMessage(m_session, "PetFlags: %u", pet_flags);
        for (uint32 i = 0; i < numpetflags; i++)
            if ((pet_flags & PetFlagToName[i].Flag) != 0)
                GreenSystemMessage(m_session, "%s", PetFlagToName[i].Name);
    }

    uint32 unit_flags = creature_target->getUnitFlags();
    GreenSystemMessage(m_session, "UnitFlags: %u", unit_flags);

    for (uint32 i = 0; i < numflags; i++)
        if ((unit_flags & UnitFlagToName[i].Flag) != 0)
            GreenSystemMessage(m_session, "-- %s", UnitFlagToName[i].Name);

#if VERSION_STRING > Classic
    uint32 unit_flags2 = creature_target->getUnitFlags2();
    GreenSystemMessage(m_session, "UnitFlags2: %u", unit_flags2);
#endif

    for (uint32 i = 0; i < numflags2; i++)
        if ((unit_flags & UnitFlagToName2[i].Flag) != 0)
            GreenSystemMessage(m_session, "-- %s", UnitFlagToName2[i].Name);

    uint32 dyn_flags = creature_target->getDynamicFlags();
    GreenSystemMessage(m_session, "UnitDynamicFlags: %u", dyn_flags);

    for (uint32 i = 0; i < numdynflags; i++)
        if ((dyn_flags & UnitDynFlagToName[i].Flag) != 0)
            GreenSystemMessage(m_session, "%s", UnitDynFlagToName[i].Name);

    GreenSystemMessage(m_session, "=================================");

    //////////////////////////////////////////////////////////////////////////////////////////
    // owner/summoner
    Unit* unit_owner = nullptr;
    bool owner_header_set = false;
    if (creature_target->isSummon())
        unit_owner = static_cast<Summon*>(creature_target)->getUnitOwner();

    if (unit_owner != nullptr)
    {
        SystemMessage(m_session, "Owner/Summoner ===================");

        if (unit_owner->isPlayer())
            SystemMessage(m_session, "Owner is Player: %s", static_cast<Player*>(unit_owner)->getName().c_str());
        if (unit_owner->isPet())
            SystemMessage(m_session, "Owner is Pet: %s", static_cast<Creature*>(unit_owner)->GetCreatureProperties()->Name.c_str());
        if (unit_owner->isCreature())
            SystemMessage(m_session, "Owner is Creature: %s", static_cast<Creature*>(unit_owner)->GetCreatureProperties()->Name.c_str());

        owner_header_set = true;
    }

    if (creature_target->getCreatedByGuid() || creature_target->getSummonedByGuid() ||
        creature_target->getCharmedByGuid() || creature_target->getCreatedBySpellId())
    {
        if (!owner_header_set)
        {
            SystemMessage(m_session, "Owner/Summoner ===================");
            owner_header_set = true;
        }

        if (creature_target->getCreatedByGuid())
            SystemMessage(m_session, "Creator GUID: %u", WoWGuid::getGuidLowPartFromUInt64(creature_target->getCreatedByGuid()));
        if (creature_target->getSummonedByGuid())
            SystemMessage(m_session, "Summoner GUID: %u", WoWGuid::getGuidLowPartFromUInt64(creature_target->getSummonedByGuid()));
        if (creature_target->getCharmedByGuid())
            SystemMessage(m_session, "Charmer GUID: %u", WoWGuid::getGuidLowPartFromUInt64(creature_target->getCharmedByGuid()));
        if (creature_target->getCreatedBySpellId())
            SystemMessage(m_session, "Creator Spell: %u", WoWGuid::getGuidLowPartFromUInt64(creature_target->getCreatedBySpellId()));
    }

    if (owner_header_set)
        SystemMessage(m_session, "=================================");

    if (creature_target->m_spawn != nullptr)
        SystemMessage(m_session, "Is part of table: %s", creature_target->m_spawn->table.c_str());
    else
        SystemMessage(m_session, "Is spawnd by an internal script");

    //////////////////////////////////////////////////////////////////////////////////////////
    // equipment
    GreenSystemMessage(m_session, "Equipment ============================");
    GreenSystemMessage(m_session, "-- Melee: %u (displayid)", creature_target->getVirtualItemSlotId(MELEE));
    GreenSystemMessage(m_session, "-- Offhand: %u (displayid)", creature_target->getVirtualItemSlotId(OFFHAND));
    GreenSystemMessage(m_session, "-- Ranged: %u (displayid)", creature_target->getVirtualItemSlotId(RANGED));

    //////////////////////////////////////////////////////////////////////////////////////////
    // transport
    if (auto transporter = creature_target->GetTransport())
    {
        SystemMessage(m_session, "Creature is on Transporter!");
        if (creature_target->obj_movement_info.hasMovementFlag(MOVEFLAG_TRANSPORT))
            SystemMessage(m_session, "Creature has MovementFlag MOVEFLAG_TRANSPORT");
        else
            SystemMessage(m_session, "!!!!!!!!! NO MovementFlag MOVEFLAG_TRANSPORT !!!!!!!!!!!!");
    }

    if (sScriptMgr.has_creature_script(creature_target->getEntry()))
        SystemMessage(m_session, "Creature has C++/LUA script");
    else
        SystemMessage(m_session, "Creature doesn't have C++/LUA script");

    if (sScriptMgr.has_creature_gossip(creature_target->getEntry()))
        SystemMessage(m_session, "Creature has C++/LUA gossip script");
    else
        SystemMessage(m_session, "Creature doesn't have C++/LUA gossip script");

    RedSystemMessage(m_session, "EntryID: %d", creature_target->getEntry());
    RedSystemMessage(m_session, "SpawnID: %d", creature_target->GetSQL_id());

    return true;
}

//.npc listagent
bool ChatHandler::HandleNpcListAIAgentCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    QueryResult* result = WorldDatabase.Query("SELECT * FROM ai_agents where entry=%u", creature_target->getEntry());
    if (result == nullptr)
    {
        RedSystemMessage(m_session, "Selected Creature %s (%u) has no entries in ai_agents table!", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
        return true;
    }
    else
    {
        SystemMessage(m_session, "Agent list for Creature %s (%u)", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
        do
        {
            Field* fields = result->Fetch();
            SystemMessage(m_session, "-- agent: %u | spellId: %u | event: %u | chance: %u | maxcount: %u", fields[1].GetUInt32(), fields[5].GetUInt32(), fields[2].GetUInt32(), fields[3].GetUInt32(), fields[4].GetUInt32());
        } while (result->NextRow());

        delete result;
    }

    return true;
}

//.npc listloot
bool ChatHandler::HandleNpcListLootCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    QueryResult* loot_result = WorldDatabase.Query("SELECT itemid, normal10percentchance, heroic10percentchance, normal25percentchance, heroic25percentchance, mincount, maxcount FROM loot_creatures WHERE entryid=%u;", creature_target->getEntry());
    if (loot_result != nullptr)
    {
        uint8 numFound = 0;

        uint32 minQuality = 0;
        if (*args)
            minQuality = atol(args);

        SystemMessage(m_session, "Listing loot for Creature %s (%u)", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());

        do
        {
            Field* field = loot_result->Fetch();

            auto item_proto = sMySQLStore.getItemProperties(field[0].GetUInt32());
            if (item_proto == nullptr || item_proto->Quality < minQuality)
                continue;

            RedSystemMessage(m_session, "ItemID: %u %s", item_proto->ItemId, GetItemLinkByProto(item_proto, m_session->language).c_str());
            SystemMessage(m_session, "-- N10 (%3.2lf) N25 (%3.2lf) H10 (%3.2lf) H25 (%3.2lf) min/max (%u/%u)", field[1].GetFloat(), field[3].GetFloat(), field[2].GetFloat(), field[4].GetFloat(), field[5].GetUInt32(), field[6].GetUInt32());

            ++numFound;
        } while (loot_result->NextRow() && (numFound <= 25));
        delete loot_result;
        if (numFound > 25)
        {
            RedSystemMessage(m_session, "More than 25 results found. Use .npc listloot <min quality> to increase the results.");
        }
        else
        {
            SystemMessage(m_session, "%hhu results found.", numFound);
        }
    }
    else
    {
        RedSystemMessage(m_session, "No loot in loot_creatures table for %s (%u).", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->getEntry());
    }
    return true;
}

//.npc stopfollow
bool ChatHandler::HandleNpcStopFollowCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    creature_target->getMovementManager()->remove(FOLLOW_MOTION_TYPE);

    sGMLog.writefromsession(m_session, "cancelled npc follow command on %s, sqlid %u", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
    return true;
}

//.npc respawn
bool ChatHandler::HandleNpcRespawnCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    if (creature_target->isCreature() && creature_target->getDeathState() == CORPSE && creature_target->spawnid != 0)
    {
        sEventMgr.RemoveEvents(creature_target, EVENT_CREATURE_RESPAWN);

        BlueSystemMessage(m_session, "Respawning Creature: `%s` with entry: %u on map: %u spawnid: %u", creature_target->GetCreatureProperties()->Name.c_str(),
            creature_target->getEntry(), creature_target->GetMapMgr()->GetMapId(), creature_target->spawnid);
        sGMLog.writefromsession(m_session, "respawned Creature: `%s` with entry: %u on map: %u sqlid: %u", creature_target->GetCreatureProperties()->Name.c_str(),
            creature_target->getEntry(), creature_target->GetMapMgr()->GetMapId(), creature_target->spawnid);

        if (creature_target->GetMapMgr()->pInstance != nullptr)
        {
            creature_target->GetMapMgr()->pInstance->m_killedNpcs.erase(creature_target->getSpawnId());
            creature_target->GetMapMgr()->pInstance->m_killedNpcs.erase(creature_target->getEntry());
        }

        creature_target->Despawn(0, 1000);
    }
    else
    {
        RedSystemMessage(m_session, "You must select a creature's corpse with a valid spawnid.");
        return true;
    }

    return true;
}

//.npc return
bool ChatHandler::HandleNpcReturnCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    creature_target->getAIInterface()->setAiState(AI_STATE_IDLE);

    creature_target->getMovementManager()->moveTargetedHome();

    sGMLog.writefromsession(m_session, "returned NPC %s (%u)", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);

    return true;
}

//.npc say
bool ChatHandler::HandleNpcSayCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    if (!args)
    {
        RedSystemMessage(m_session, "No text set. Use .npc say <text>!");
        return true;
    }

    creature_target->sendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, args);

    return true;
}

//.npc select
bool ChatHandler::HandleNpcSelectCommand(const char* /*args*/, WorldSession* m_session)
{
    Creature* near_creature = nullptr;
    float dist = 999999.0f;
    float dist2;

    auto player = m_session->GetPlayer();
    for (const auto& itr : player->getInRangeObjectsSet())
    {
        if (itr && (dist2 = player->GetDistance2dSq(itr)) < dist && (itr)->isCreature())
        {
            near_creature = static_cast<Creature*>(itr);
            dist = dist2;
        }
    }

    if (near_creature == nullptr)
    {
        RedSystemMessage(m_session, "No inrange creatures found.");
        return true;
    }

    player->setTargetGuid(near_creature->getGuid());
    SystemMessage(m_session, "Nearest Creature %s spawnID: %u GUID: " I64FMT " selected", near_creature->GetCreatureProperties()->Name.c_str(), near_creature->spawnid, near_creature->getGuid());
    return true;
}

//.npc spawn
bool ChatHandler::HandleNpcSpawnCommand(const char* args, WorldSession* m_session)
{
    uint32 entry = atol(args);
    if (entry == 0)
        return false;

    auto creature_properties = sMySQLStore.getCreatureProperties(entry);
    if (creature_properties == nullptr)
    {
        RedSystemMessage(m_session, "Creature with entry %u is not a valid entry (no properties information in database)", entry);
        return true;
    }

    auto creature_spawn = new MySQLStructure::CreatureSpawn;
    uint8 gender = creature_properties->GetGenderAndCreateRandomDisplayID(&creature_spawn->displayid);
    creature_spawn->entry = entry;
    creature_spawn->id = sObjectMgr.GenerateCreatureSpawnID();
    creature_spawn->movetype = 0;
    creature_spawn->x = m_session->GetPlayer()->GetPositionX();
    creature_spawn->y = m_session->GetPlayer()->GetPositionY();
    creature_spawn->z = m_session->GetPlayer()->GetPositionZ();
    creature_spawn->o = m_session->GetPlayer()->GetOrientation();
    creature_spawn->emote_state = 0;
    creature_spawn->flags = creature_properties->NPCFLags;
    creature_spawn->factionid = creature_properties->Faction;
    creature_spawn->bytes0 = creature_spawn->setbyte(0, 2, gender);
    creature_spawn->bytes1 = 0;
    creature_spawn->bytes2 = 0;
    creature_spawn->stand_state = 0;
    creature_spawn->death_state = 0;
    creature_spawn->channel_target_creature = creature_spawn->channel_target_go = creature_spawn->channel_spell = 0;
    creature_spawn->MountedDisplayID = 0;

    creature_spawn->Item1SlotEntry = creature_properties->itemslot_1;
    creature_spawn->Item2SlotEntry = creature_properties->itemslot_2;
    creature_spawn->Item3SlotEntry = creature_properties->itemslot_3;

    creature_spawn->Item1SlotDisplay = sMySQLStore.getItemDisplayIdForEntry(creature_spawn->Item1SlotEntry);
    creature_spawn->Item2SlotDisplay = sMySQLStore.getItemDisplayIdForEntry(creature_spawn->Item2SlotEntry);
    creature_spawn->Item3SlotDisplay = sMySQLStore.getItemDisplayIdForEntry(creature_spawn->Item3SlotEntry);
    creature_spawn->CanFly = 0;
    creature_spawn->phase = m_session->GetPlayer()->GetPhase();

    if (auto creature = m_session->GetPlayer()->GetMapMgr()->CreateCreature(entry))
    {
        creature->Load(creature_spawn, 0, nullptr);
        creature->m_loadedFromDB = true;
        creature->PushToWorld(m_session->GetPlayer()->GetMapMgr());

        // Add to map
        uint32 x = m_session->GetPlayer()->GetMapMgr()->GetPosX(m_session->GetPlayer()->GetPositionX());
        uint32 y = m_session->GetPlayer()->GetMapMgr()->GetPosY(m_session->GetPlayer()->GetPositionY());
        m_session->GetPlayer()->GetMapMgr()->GetBaseMap()->GetSpawnsListAndCreate(x, y)->CreatureSpawns.push_back(creature_spawn);
        MapCell* map_cell = m_session->GetPlayer()->GetMapMgr()->GetCell(x, y);
        if (map_cell != nullptr)
            map_cell->SetLoaded();

        creature->SaveToDB();

        BlueSystemMessage(m_session, "Spawned a creature `%s` with entry %u at %f %f %f on map %u", creature_properties->Name.c_str(),
            entry, creature_spawn->x, creature_spawn->y, creature_spawn->z, m_session->GetPlayer()->GetMapId());
        sGMLog.writefromsession(m_session, "spawned a %s at %u %f %f %f", creature_properties->Name.c_str(), m_session->GetPlayer()->GetMapId(),
            creature_spawn->x, creature_spawn->y, creature_spawn->z);
    }
    return true;
}

//.npc yell
bool ChatHandler::HandleNpcYellCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    if (!args)
    {
        RedSystemMessage(m_session, "No text set. Use .npc say <text>!");
        return true;
    }

    creature_target->sendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, args);

    return true;
}

// Zyres: following commands are for units
//.npc possess
bool ChatHandler::HandlePossessCommand(const char* /*args*/, WorldSession* m_session)
{
    auto unit_target = GetSelectedUnit(m_session);
    if (unit_target != nullptr)
    {
        if (unit_target->isPet() || unit_target->getCreatedByGuid() != 0)
        {
            RedSystemMessage(m_session, "You can not possess a pet!");
            return false;
        }
        else if (unit_target->isPlayer())
        {
            auto player = static_cast<Player*>(unit_target);
            BlueSystemMessage(m_session, "Player %s selected.", player->getName().c_str());
            sGMLog.writefromsession(m_session, "used possess command on PLAYER %s", player->getName().c_str());
        }
        else if (unit_target->isCreature())
        {
            auto creature = static_cast<Creature*>(unit_target);
            BlueSystemMessage(m_session, "Creature %s selected.", creature->GetCreatureProperties()->Name.c_str());
            sGMLog.writefromsession(m_session, "used possess command on Creature %s spawn_id %u", creature->GetCreatureProperties()->Name.c_str(), creature->GetSQL_id());
        }
    }
    else
    {
        RedSystemMessage(m_session, "You must select a Player/Creature.");
        return false;
    }

    m_session->GetPlayer()->Possess(unit_target);

    return true;
}

//.npc vendoradditem
bool ChatHandler::HandleNpcVendorAddItemCommand(const char* args, WorldSession* m_session)
{
#if VERSION_STRING < Cata
    char* pitem = strtok(const_cast<char*>(args), " ");
    if (!pitem)
        return false;

    WoWGuid wowGuid;
    wowGuid.Init(m_session->GetPlayer()->getTargetGuid());

    if (wowGuid.getRawGuid() == 0)
    {
        SystemMessage(m_session, "No selection.");
        return true;
    }

    Creature* selected_creature = m_session->GetPlayer()->GetMapMgr()->GetCreature(wowGuid.getGuidLowPart());
    if (selected_creature == nullptr)
    {
        SystemMessage(m_session, "You should select a creature.");
        return true;
    }

    uint32 item = atoi(pitem);
    int amount = -1;

    char* pamount = strtok(nullptr, " ");
    if (pamount)
        amount = atoi(pamount);

    if (amount == -1)
    {
        SystemMessage(m_session, "You need to specify an amount.");
        return true;
    }

    uint32 costid = 0;
    char* pcostid = strtok(nullptr, " ");
    if (pcostid)
        costid = atoi(pcostid);

    auto item_extended_cost = (costid > 0) ? sItemExtendedCostStore.LookupEntry(costid) : nullptr;
    if (costid > 0 && sItemExtendedCostStore.LookupEntry(costid) == nullptr)
    {
        SystemMessage(m_session, "You've entered invalid extended cost id.");
        return true;
    }

    ItemProperties const* tmpItem = sMySQLStore.getItemProperties(item);
    if (tmpItem)
    {
        WorldDatabase.Execute("INSERT INTO vendors VALUES (%u, %u, %u, 0, 0, %u", selected_creature->getEntry(), item, amount, costid);

        selected_creature->AddVendorItem(item, amount, item_extended_cost);

        if (costid > 0)
            BlueSystemMessage(m_session, "Item %u (%s) added to vendorlist with extended cost %u.", item, tmpItem->Name.c_str(), costid);
        else
            BlueSystemMessage(m_session, "Item %u (%s) added to vendorlist.", item, tmpItem->Name.c_str());
    }
    else
    {
        RedSystemMessage(m_session, "Item %u not found in database", item);
    }

    sGMLog.writefromsession(m_session, "added item %u to vendor %u", item, selected_creature->getEntry());
#else
    char* pitem = strtok((char*)args, " ");
    if (!pitem)
        return false;

    uint64 guid = m_session->GetPlayer()->getTargetGuid();
    if (guid == 0)
    {
        SystemMessage(m_session, "No selection.");
        return true;
    }
#endif
    return true;
}

//.npc vendorremoveitem
bool ChatHandler::HandleNpcVendorRemoveItemCommand(const char* args, WorldSession* m_session)
{
    char* iguid = strtok((char*)args, " ");
    if (!iguid)
        return false;

    WoWGuid wowGuid;
    wowGuid.Init(m_session->GetPlayer()->getTargetGuid());
    if (wowGuid.getRawGuid() == 0)
    {
        SystemMessage(m_session, "No selection.");
        return true;
    }

    Creature* selected_creature = m_session->GetPlayer()->GetMapMgr()->GetCreature(wowGuid.getGuidLowPart());
    if (selected_creature == nullptr)
    {
        SystemMessage(m_session, "You should select a creature.");
        return true;
    }

    uint32 itemguid = atoi(iguid);
    int slot = selected_creature->GetSlotByItemId(itemguid);
    if (slot != -1)
    {
        uint32 creatureId = selected_creature->getEntry();

        WorldDatabase.Execute("DELETE FROM vendors WHERE entry = %u AND item = %u", creatureId, itemguid);

        selected_creature->RemoveVendorItem(itemguid);
        ItemProperties const* tmpItem = sMySQLStore.getItemProperties(itemguid);
        if (tmpItem)
        {
            BlueSystemMessage(m_session, "Item %u (%s) deleted from list.", itemguid, tmpItem->Name.c_str());
        }
        else
        {
            BlueSystemMessage(m_session, "Item %u deleted from list.", itemguid);
        }
        sGMLog.writefromsession(m_session, "removed item %u from vendor %u", itemguid, creatureId);
    }
    else
    {
        RedSystemMessage(m_session, "Item %u not found in vendorlist.", itemguid);
    }

    return true;
}

//.npc unpossess
bool ChatHandler::HandleUnPossessCommand(const char* /*args*/, WorldSession* m_session)
{
    auto unit_target = GetSelectedUnit(m_session);

    if (unit_target != nullptr)
    {
        if (unit_target->isPlayer())
        {
            auto player = static_cast<Player*>(unit_target);
            BlueSystemMessage(m_session, "Player %s is no longer possessed by you.", player->getName().c_str());
        }
        else if (unit_target->isCreature())
        {
            auto creature = static_cast<Creature*>(unit_target);
            BlueSystemMessage(m_session, "Creature %s is no longer possessed by you.", creature->GetCreatureProperties()->Name.c_str());
        }
    }
    else
    {
        RedSystemMessage(m_session, "You must select a Player/Creature.");
        return false;
    }

    m_session->GetPlayer()->UnPossess();

    return true;
}

//.npc showtimers
bool ChatHandler::HandleNpcShowTimersCommand(const char* /*args*/, WorldSession* m_session)
{
    Creature* creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    if (CreatureAIScript* creatureScript = creature_target->GetScript())
        creatureScript->displayCreatureTimerList(m_session->GetPlayer());

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// .npc set commands
//.npc set canfly
bool ChatHandler::HandleNpcSetCanFlyCommand(const char* args, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    bool save_to_db = atoi(args) == 1 ? true : false;

    if (m_session->GetPlayer()->m_saveAllChangesCommand)
        save_to_db = true;

    if (creature_target->IsFlying())
    {
        creature_target->setMoveCanFly(false);

        if (save_to_db)
        {
            WorldDatabase.Execute("UPDATE %s SET CanFly = 1 WHERE id = %u AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, VERSION_STRING, VERSION_STRING);
            GreenSystemMessage(m_session, "CanFly permanent set from 0 to 1 for Creature %s (%u).", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
            sGMLog.writefromsession(m_session, "changed npc CanFly for %s ID: %u [%s] from 0 to 1", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str());
        }
        else
        {
            GreenSystemMessage(m_session, "CanFly temporarily set from 0 to 1 for Creature %s (%u).", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
        }
    }
    else
    {
        creature_target->setMoveCanFly(true);
        if (save_to_db)
        {
            WorldDatabase.Execute("UPDATE %s SET CanFly = 0 WHERE id = %u AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, VERSION_STRING, VERSION_STRING);
            GreenSystemMessage(m_session, "CanFly permanent set from 1 to 0 for Creature %s (%u).", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
            sGMLog.writefromsession(m_session, "changed npc CanFly for %s ID: %u [%s] from 1 to 0", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str());
        }
        else
        {
            GreenSystemMessage(m_session, "CanFly temporarily set from 1 to 0 for Creature %s (%u).", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
        }
    }
    return true;
}

//.npc set equip
bool ChatHandler::HandleNpcSetEquipCommand(const char* args, WorldSession* m_session)
{
    uint8_t equipment_slot;
    uint32 item_id;

    if (sscanf(args, "%hhu %u", &equipment_slot, &item_id) != 2)
    {
        RedSystemMessage(m_session, "Command must be in format: .npc add equipment <slot> <item_id>.");
        RedSystemMessage(m_session, "Slots: (0)melee, (1)offhand, (2)ranged");
        return true;
    }

    Creature* creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    auto item_entry = sItemStore.LookupEntry(item_id);
    if (item_entry == nullptr)
    {
        RedSystemMessage(m_session, "Item ID: %u is not a valid item!", item_id);
        return true;
    }

    switch (equipment_slot)
    {
        case MELEE:
        {
            creature_target->m_spawn->Item1SlotEntry = item_id;
            GreenSystemMessage(m_session, "Melee slot successfull changed from %u to %u for Creature %s", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->GetCreatureProperties()->Name.c_str());
            sGMLog.writefromsession(m_session, "changed melee slot from %u to %u for creature spawn %u", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->spawnid);
            break;
        }
        case OFFHAND:
        {
            creature_target->m_spawn->Item2SlotEntry = item_id;
            GreenSystemMessage(m_session, "Offhand slot successfull changed from %u to %u for Creature %s", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->GetCreatureProperties()->Name.c_str());
            sGMLog.writefromsession(m_session, "changed offhand slot from %u to %u for creature spawn %u", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->spawnid);
            break;
        }
        case RANGED:
        {
            creature_target->m_spawn->Item3SlotEntry = item_id;
            GreenSystemMessage(m_session, "Ranged slot successfull changed from %u to %u for Creature %s", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->GetCreatureProperties()->Name.c_str());
            sGMLog.writefromsession(m_session, "changed ranged slot from %u to %u for creature spawn %u", creature_target->getVirtualItemSlotId(equipment_slot), item_entry->DisplayId, creature_target->spawnid);
            break;
        }
        default:
        {
            RedSystemMessage(m_session, "Slot: %u is not a valid slot! Use: (0)melee, (1)offhand, (2)ranged.", equipment_slot);
            return true;
        }
    }

    creature_target->setVirtualItemSlotId(equipment_slot, item_entry->DisplayId);
    creature_target->SaveToDB();
    return true;
}

//.npc set emote
bool ChatHandler::HandleNpcSetEmoteCommand(const char* args, WorldSession* m_session)
{
    uint32 emote;
    uint32 save = 0;

    if (sscanf(args, "%u %u", &emote, &save) != 2)
    {
        if (sscanf(args, "%u", &emote) != 1)
        {
            RedSystemMessage(m_session, "Command must be at least in format: .npc set emote <emote>.");
            RedSystemMessage(m_session, "Use the following format to save the emote: .npc set emote <emote> 1.");
            return true;
        }
    }

    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint32 old_emote = creature_target->getEmoteState();
    creature_target->setEmoteState(emote);

    if (m_session->GetPlayer()->m_saveAllChangesCommand)
        save = 1;

    if (save == 1)
    {
        WorldDatabase.Execute("UPDATE %s SET emote_state = '%lu' WHERE id = %lu AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), emote, creature_target->spawnid, VERSION_STRING, VERSION_STRING);
        GreenSystemMessage(m_session, "Emote permanent set from %u to %u for spawn ID: %u.", old_emote, emote, creature_target->spawnid);
        sGMLog.writefromsession(m_session, "changed npc emote of %s ID: %u [%s] from %u to %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str(), old_emote, emote);
    }
    else
    {
        GreenSystemMessage(m_session, "Emote temporarily set from %u to %u for spawn ID: %u.", old_emote, emote, creature_target->spawnid);
    }

    return true;
}

//.npc set formationmaster
bool ChatHandler::HandleNpcSetFormationMasterCommand(const char* /*args*/, WorldSession* m_session)
{
    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    m_session->GetPlayer()->m_formationMaster = creature_target;
    BlueSystemMessage(m_session, "Formation Master set to %s spawn ID: %u.", creature_target->GetCreatureProperties()->Name.c_str(), creature_target->spawnid);
    return true;
}

//.npc set formationslave
bool ChatHandler::HandleNpcSetFormationSlaveCommand(const char* /*args*/, WorldSession* /*m_session*/)
{
    return true;
}

//.npc set formationclear
bool ChatHandler::HandleNpcSetFormationClearCommand(const char* /*args*/, WorldSession* /*m_session*/)
{
    return true;
}

//.npc set flags
bool ChatHandler::HandleNpcSetFlagsCommand(const char* args, WorldSession* m_session)
{
    uint32 npc_flags;
    uint32 save = 0;

    if (sscanf(args, "%u %u", &npc_flags, &save) < 1)
    {
        RedSystemMessage(m_session, "You need to define the flag value!");
        RedSystemMessage(m_session, ".npc set flags <npc_flag>");
        return true;
    }

    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return false;

    uint32 old_npc_flags = creature_target->getNpcFlags();
    creature_target->addNpcFlags(npc_flags);

    if (m_session->GetPlayer()->m_saveAllChangesCommand)
        save = 1;

    if (save == 1)
    {
        GreenSystemMessage(m_session, "Flags changed in spawns table from %u to %u for spawn ID: %u. You may need to clean your client cache.", old_npc_flags, npc_flags, creature_target->spawnid);
        WorldDatabase.Execute("UPDATE %s SET flags = '%lu' WHERE id = %lu AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), npc_flags, creature_target->spawnid, VERSION_STRING, VERSION_STRING);
        sGMLog.writefromsession(m_session, "changed npc flags of %s ID: %u [%s] from %u to %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str(), old_npc_flags, npc_flags);
    }
    else
    {
        GreenSystemMessage(m_session, "Flags temporarily set from %u to %u for spawn ID: %u. You may need to clean your client cache.", old_npc_flags, npc_flags, creature_target->spawnid);
    }

    return true;
}

//.npc set phase
bool ChatHandler::HandleNpcSetPhaseCommand(const char* args, WorldSession* m_session)
{
    uint32 npc_phase;
    uint32 save = 0;

    if (sscanf(args, "%u %u", &npc_phase, &save) < 1)
    {
        RedSystemMessage(m_session, "You need to define the phase!");
        RedSystemMessage(m_session, ".npc set phase <npc_phase>");
        return true;
    }

    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return false;

    uint32 old_npc_phase = creature_target->m_spawn->phase;
    creature_target->setPhase(PHASE_SET, npc_phase);

    if (m_session->GetPlayer()->m_saveAllChangesCommand)
        save = 1;

    if (save == 1)
    {
        GreenSystemMessage(m_session, "Phase changed in spawns table from %u to %u for spawn ID: %u.", old_npc_phase, npc_phase, creature_target->spawnid);
        WorldDatabase.Execute("UPDATE %s SET phase = '%lu' WHERE id = %lu AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), npc_phase, creature_target->spawnid, VERSION_STRING, VERSION_STRING);
        sGMLog.writefromsession(m_session, "changed npc phase of %s ID: %u [%s] from %u to %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str(), old_npc_phase, npc_phase);
    }
    else
    {
        GreenSystemMessage(m_session, "Phase temporarily set from %u to %u for spawn ID: %u.", old_npc_phase, npc_phase, creature_target->spawnid);
    }

    return true;
}

//.npc set standstate
bool ChatHandler::HandleNpcSetStandstateCommand(const char* args, WorldSession* m_session)
{
    uint8_t standstate;
    uint32 save = 0;

    if (sscanf(args, "%hhu %u", &standstate, &save) < 1)
    {
        RedSystemMessage(m_session, "You must specify a standstate value.");
        RedSystemMessage(m_session, ".npc set standstate <standstate>");
        return true;
    }

    auto creature_target = GetSelectedCreature(m_session, true);
    if (creature_target == nullptr)
        return true;

    uint8 old_standstate = creature_target->getStandState();
    creature_target->setStandState(standstate);

    if (m_session->GetPlayer()->m_saveAllChangesCommand)
        save = 1;

    if (save == 1)
    {
        GreenSystemMessage(m_session, "Standstate changed in spawns table from %u to %u for spawn ID: %u.", old_standstate, standstate, creature_target->spawnid);
        WorldDatabase.Execute("UPDATE %s SET standstate = '%lu' WHERE id = %lu AND min_build <= %u AND max_build >= %u", creature_target->m_spawn->table.c_str(), standstate, creature_target->spawnid, VERSION_STRING, VERSION_STRING);
        sGMLog.writefromsession(m_session, "changed npc standstate of %s ID: %u [%s] from %u to %u", creature_target->m_spawn->table.c_str(), creature_target->spawnid, creature_target->GetCreatureProperties()->Name.c_str(), old_standstate, standstate);
    }
    else
    {
        GreenSystemMessage(m_session, "Standstate temporarily set from %u to %u for spawn ID: %u.", old_standstate, standstate, creature_target->spawnid);
    }

    return true;
}
