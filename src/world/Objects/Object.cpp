/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Units/Unit.h"
#include "Units/Creatures/Summons/Summon.h"
#include "Storage/DBC/DBCStores.h"
#include "Management/QuestLogEntry.hpp"
#include "Server/EventableObject.h"
#include "VMapFactory.h"
#include "MMapFactory.h"
#include "TLSObject.h"
#include "Management/Battleground/Battleground.h"
#include "Management/ItemInterface.h"
#include "Server/WorldSocket.h"
#include "Storage/MySQLDataStore.hpp"
#include "Map/Area/AreaStorage.hpp"
#include "Map/MapMgr.h"
#include "Management/Faction.h"
#include "Map/WorldCreator.h"
#include "Spell/Definitions/ProcFlags.hpp"
#include "Spell/Definitions/SpellDamageType.hpp"
#include "Spell/Definitions/SpellMechanics.hpp"
#include "Spell/Definitions/SpellState.hpp"
#include <Spell/Definitions/AuraInterruptFlags.hpp>
#include "Chat/ChatHandler.hpp"
#include "Spell/Definitions/PowerType.hpp"
#include "Spell/SpellMgr.hpp"
#include "Units/Creatures/CreatureDefines.hpp"
#include "Data/WoWObject.hpp"
#include "Server/Packets/SmsgDestoyObject.h"
#include "Server/Packets/SmsgPlaySound.h"
#include "Server/Packets/SmsgGameobjectDespawnAnim.h"
#include "Server/Packets/SmsgSpellLogMiss.h"
#include "Server/Packets/SmsgAiReaction.h"
#include "Movement/PathGenerator.h"
#include "Movement/Spline/MovementPacketBuilder.h"

using namespace AscEmu::Packets;

Object::Object()
{
    //////////////////////////////////////////////////////////////////////////
    m_objectType = TYPE_OBJECT;
    m_objectTypeId = TYPEID_OBJECT;
    m_updateFlag = UPDATEFLAG_NONE;
    //////////////////////////////////////////////////////////////////////////
}

Object::~Object()
{
    if (!isItem())
    {
        if (!m_inQueue && !IsInWorld())
        {
            m_instanceId = INSTANCEID_NOT_IN_WORLD;

            for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
            {
                if (m_currentSpell[i] != nullptr)
                    interruptSpellWithSpellType(static_cast<CurrentSpellType>(i));
            }

            for (auto travelingSpellItr = m_travelingSpells.begin(); travelingSpellItr != m_travelingSpells.end();)
            {
                delete (*travelingSpellItr).first;
                travelingSpellItr = m_travelingSpells.erase(travelingSpellItr);
            }

            removeGarbageSpells();

            for (auto pendingSpellItr = m_pendingSpells.begin(); pendingSpellItr != m_pendingSpells.end();)
            {
                delete (*pendingSpellItr);
                pendingSpellItr = m_pendingSpells.erase(pendingSpellItr);
            }

            sEventMgr.RemoveEvents(this);
        }
    }
}

bool Object::write(const uint8_t& member, uint8_t val)
{
    if (member == val)
        return false;

    const auto member_ptr = const_cast<uint8_t*>(&member);
    *member_ptr = val;

    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance -= distance % 4;
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint16_t& member, uint16_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint16_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const float& member, float val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<float*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const int32_t& member, int32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<int32_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint32_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint32_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint64_t& member, uint64_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *nonconst_member = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::writeLow(const uint64_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *reinterpret_cast<uint32_t*>(*nonconst_member) = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::writeHigh(const uint64_t& member, uint32_t val)
{
    if (member == val)
        return false;

    const auto nonconst_member = const_cast<uint64_t*>(&member);
    *(reinterpret_cast<uint32_t*>(*nonconst_member) + 1) = val;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

bool Object::write(const uint64_t& member, uint32_t low, uint32_t high)
{
    const auto nonconst_member = const_cast<uint64_t*>(&member);
    const auto low_ptr = reinterpret_cast<uint32_t*>(*nonconst_member);
    const auto high_ptr = low_ptr + 1;

    if (*low_ptr == low && *high_ptr == high)
        return false;

    *low_ptr = low;
    *high_ptr = high;

    const auto member_ptr = reinterpret_cast<uint8_t*>(nonconst_member);
    auto distance = static_cast<uint32_t>(member_ptr - wow_data_ptr);
    distance /= 4;

    m_updateMask.SetBit(distance);
    m_updateMask.SetBit(distance + 1);

    if (!skipping_updates)
        updateObject();

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// WoWData
uint64_t Object::getGuid() const { return objectData()->guid; }
void Object::setGuid(uint64_t guid)
{
    write(objectData()->guid, guid);
    m_wowGuid.Init(guid);
    obj_movement_info.guid = guid;
}
void Object::setGuid(uint32_t low, uint32_t high) { setGuid(static_cast<uint64_t>(high) << 32 | low); }

uint32_t Object::getGuidLow() const { return objectData()->guid_parts.low; }
void Object::setGuidLow(uint32_t low) { setGuid(low, objectData()->guid_parts.high); }

uint32_t Object::getGuidHigh() const { return objectData()->guid_parts.high; }
void Object::setGuidHigh(uint32_t high) { setGuid(objectData()->guid_parts.low, high); }

#if VERSION_STRING < Cata
uint32_t Object::getOType() const { return objectData()->type; }
void Object::setOType(uint32_t type) { write(objectData()->type, type); }
void Object::setObjectType(uint8_t objectTypeId)
{
    uint16_t object_type = TYPE_OBJECT;
    switch (objectTypeId)
    {
    case TYPEID_CONTAINER:
        object_type |= TYPE_CONTAINER;
    case TYPEID_ITEM:
        object_type |= TYPE_ITEM;
        break;
    case TYPEID_PLAYER:
        object_type |= TYPE_PLAYER;
    case TYPEID_UNIT:
        object_type |= TYPE_UNIT;
        break;
    case TYPEID_GAMEOBJECT:
        object_type |= TYPE_GAMEOBJECT;
        break;
    case TYPEID_DYNAMICOBJECT:
        object_type |= TYPE_DYNAMICOBJECT;
        break;
    case TYPEID_CORPSE:
        object_type |= TYPE_CORPSE;
        break;
    default:
        break;
    }

    m_objectType = object_type;
    m_objectTypeId = objectTypeId;
    write(objectData()->type, static_cast<uint32_t>(m_objectType));
}
#else
uint16_t Object::getOType() const { return objectData()->parts.type; }
void Object::setOType(uint16_t type) { write(objectData()->parts.type, type); }
void Object::setObjectType(uint8_t objectTypeId)
{
    uint16_t object_type = TYPE_OBJECT;
    switch (objectTypeId)
    {
    case TYPEID_CONTAINER:
        object_type |= TYPE_CONTAINER;
    case TYPEID_ITEM:
        object_type |= TYPE_ITEM;
        break;
    case TYPEID_PLAYER:
        object_type |= TYPE_PLAYER;
    case TYPEID_UNIT:
        object_type |= TYPE_UNIT;
        break;
    case TYPEID_GAMEOBJECT:
        object_type |= TYPE_GAMEOBJECT;
        break;
    case TYPEID_DYNAMICOBJECT:
        object_type |= TYPE_DYNAMICOBJECT;
        break;
    case TYPEID_CORPSE:
        object_type |= TYPE_CORPSE;
        break;
    default:
        break;
    }

    m_objectType = object_type;
    m_objectTypeId = objectTypeId;
    write(objectData()->parts.type, static_cast<uint16_t>(m_objectType));
}
#endif

uint32_t Object::getEntry() const { return objectData()->entry; }
void Object::setEntry(uint32_t entry) { write(objectData()->entry, entry); }

float Object::getScale() const { return objectData()->scale_x; }
void Object::setScale(float scaleX) { write(objectData()->scale_x, scaleX); }

//////////////////////////////////////////////////////////////////////////////////////////
// Object update
void Object::updateObject()
{
    if (IsInWorld() && !m_objectUpdated)
    {
        m_mapMgr->ObjectUpdated(this);
        m_objectUpdated = true;
    }
}

uint32_t Object::buildCreateUpdateBlockForPlayer(ByteBuffer* data, Player* target)
{
    if (m_wowGuid.GetNewGuidLen() <= 0)
        return 0;

    if (target == nullptr)
        return 0;

    uint8_t updateType = UPDATETYPE_CREATE_OBJECT;
#if VERSION_STRING <= TBC
    uint8_t updateFlags = static_cast<uint8_t>(m_updateFlag);
#endif

#if VERSION_STRING >= WotLK
    uint16_t updateFlags = m_updateFlag;
#endif

    if (target == this)
        updateFlags |= UPDATEFLAG_SELF;

    switch (m_objectTypeId)
    {
        case TYPEID_PLAYER:
        case TYPEID_DYNAMICOBJECT:
        case TYPEID_CORPSE:
        case TYPEID_AREATRIGGER:
        {
            updateType = UPDATETYPE_CREATE_OBJECT2;
        } break;
        case TYPEID_UNIT:
        {
            ///\ todo: vehicles?
            if (isPet())
            {
                updateType = UPDATETYPE_CREATE_OBJECT2;
            }
            else if (isSummon())
            {
                // Only player summons
                const auto summoner = GetMapMgrPlayer(static_cast<Summon*>(this)->getSummonedByGuid());
                if (summoner != nullptr)
                    updateType = UPDATETYPE_CREATE_OBJECT2;
            }
        } break;
        case TYPEID_GAMEOBJECT:
        {
            // Only player gameobjects
            if (static_cast<GameObject*>(this)->getPlayerOwner() != nullptr)
                updateType = UPDATETYPE_CREATE_OBJECT2;
        } break;
        default:
            break;
    }

    if (!(updateFlags & UPDATEFLAG_LIVING))
    {
        if (obj_movement_info.transport_guid != 0)
            updateFlags |= UPDATEFLAG_HAS_POSITION;
    }

    if (updateFlags & UPDATEFLAG_HAS_POSITION)
    {
        if (isGameObject())
        {
            const auto gameObj = static_cast<GameObject*>(this);
            switch (gameObj->getGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_DUEL_ARBITER:
                case GAMEOBJECT_TYPE_FLAGSTAND:
                case GAMEOBJECT_TYPE_FLAGDROP:
                    updateType = UPDATETYPE_CREATE_OBJECT2;
                    break;
                case GAMEOBJECT_TYPE_TRANSPORT:
                case GAMEOBJECT_TYPE_MO_TRANSPORT:
                    updateFlags |= UPDATEFLAG_TRANSPORT;
                    break;
                default:
                    // Set update type for other gameobjects only if it's created by a player
                    if (gameObj->getPlayerOwner() != nullptr)
                        updateType = UPDATETYPE_CREATE_OBJECT2;
                    break;
            }
        }
    }

#ifdef FT_VEHICLES
    if (isVehicle())
        updateFlags |= UPDATEFLAG_VEHICLE;
#endif

    if (isCreatureOrPlayer())
    {
        if (dynamic_cast<Unit*>(this)->getTargetGuid() != 0)
            updateFlags |= UPDATEFLAG_HAS_TARGET;
    }

    // build our actual update
    *data << uint8_t(updateType);
    *data << m_wowGuid;
    *data << uint8_t(m_objectTypeId);

    buildMovementUpdate(data, updateFlags, target);

    // we have dirty data, or are creating for ourself.
    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);
    _SetCreateBits(&updateMask, target);

    // this will cache automatically if needed
    buildValuesUpdate(data, &updateMask, target);
#if VERSION_STRING == Mop
    *data << uint8_t(0);
#endif
    // Update count
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Object Type Id
uint8_t Object::getObjectTypeId() const { return m_objectTypeId; }

bool Object::isCreatureOrPlayer() const { return (getObjectTypeId() == TYPEID_UNIT || getObjectTypeId() == TYPEID_PLAYER); }
bool Object::isPlayer() const { return getObjectTypeId() == TYPEID_PLAYER; }
bool Object::isCreature() const { return getObjectTypeId() == TYPEID_UNIT; }
bool Object::isItem() const { return getObjectTypeId() == TYPEID_ITEM; }
bool Object::isGameObject() const { return getObjectTypeId() == TYPEID_GAMEOBJECT; }
bool Object::isCorpse() const { return getObjectTypeId() == TYPEID_CORPSE; }
bool Object::isContainer() const { return getObjectTypeId() == TYPEID_CONTAINER; }

//////////////////////////////////////////////////////////////////////////////////////////
// Position functions
bool Object::isInRange(LocationVector location, float square_r) const
{
    return getDistanceSq(location) <= square_r;
}

bool Object::isInRange(float x, float y, float z, float square_r) const
{
    return getDistanceSq(x, y, z) <= square_r;
}

float Object::getDistanceSq(LocationVector comp) const
{
    return comp.distanceSquare(m_position);
}

float Object::getDistanceSq(float x, float y, float z) const
{
    return m_position.distanceSquare({ x, y, z });
}

Player* Object::asPlayer()
{
    return reinterpret_cast<Player*>(this);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Spell functions
Spell* Object::getCurrentSpell(CurrentSpellType spellType) const
{
    return m_currentSpell[spellType];
}

Spell* Object::getCurrentSpellById(uint32_t spellId) const
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] == nullptr)
            continue;
        if (m_currentSpell[i]->getSpellInfo()->getId() == spellId)
            return m_currentSpell[i];
    }
    return nullptr;
}

void Object::setCurrentSpell(Spell* curSpell)
{
    if (curSpell != nullptr) // curSpell cannot be nullptr
    {
        // Get current spell type
        CurrentSpellType spellType = CURRENT_GENERIC_SPELL;
        if (curSpell->getSpellInfo()->isOnNextMeleeAttack())
        {
            // Melee spell
            spellType = CURRENT_MELEE_SPELL;
        }
        else if (curSpell->getSpellInfo()->isRangedAutoRepeat())
        {
            // Autorepeat spells (Auto shot / Shoot (wand))
            spellType = CURRENT_AUTOREPEAT_SPELL;
        }
        else if (curSpell->getSpellInfo()->isChanneled())
        {
            // Channeled spells
            spellType = CURRENT_CHANNELED_SPELL;
        }

        // We've already set this spell to current spell, ignore
        if (curSpell == m_currentSpell[spellType])
            return;

        // Interrupt spell with same spell type
        interruptSpellWithSpellType(spellType);

        // Handle spelltype specific cases
        switch (spellType)
        {
        case CURRENT_GENERIC_SPELL:
        {
            // Generic spells break channeled spells
            interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);

            if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr)
            {
                // Generic spells do not break Auto Shot
                if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL]->getSpellInfo()->getId() != 75)
                    interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
            }
        } break;
        case CURRENT_CHANNELED_SPELL:
        {
            // Channeled spells break generic spells
            interruptSpellWithSpellType(CURRENT_GENERIC_SPELL);
            // as well break channeled spells too
            interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);

            // Also break autorepeat spells, unless it's Auto Shot
            if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr && m_currentSpell[CURRENT_AUTOREPEAT_SPELL]->getSpellInfo()->getId() != 75)
                interruptSpellWithSpellType(CURRENT_AUTOREPEAT_SPELL);
        } break;
        case CURRENT_AUTOREPEAT_SPELL:
        {
            // Other autorepeats than Auto Shot break non-delayed generic and channeled spells
            if (curSpell->getSpellInfo()->getId() != 75)
            {
                interruptSpellWithSpellType(CURRENT_GENERIC_SPELL);
                interruptSpellWithSpellType(CURRENT_CHANNELED_SPELL);
            }

            if (isPlayer())
                static_cast<Player*>(this)->m_FirstCastAutoRepeat = true;
        } break;
        default:
            break;
        }

        // If spell is not yet cancelled, force it
        if (m_currentSpell[spellType] != nullptr)
            m_currentSpell[spellType]->finish(false);

        // Set new current spell
        m_currentSpell[spellType] = curSpell;
    }
    else
    {
        sLogger.failure("Object::setCurrentSpell tried to set invalid current spell (nullptr)");
    }
}

void Object::interruptSpell(uint32_t spellId, bool checkMeleeSpell)
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (!checkMeleeSpell && i == CURRENT_MELEE_SPELL)
            continue;

        if (m_currentSpell[i] != nullptr && (spellId == 0 || m_currentSpell[i]->getSpellInfo()->getId() == spellId))
            interruptSpellWithSpellType(CurrentSpellType(i));
    }
}

void Object::interruptSpellWithSpellType(CurrentSpellType spellType)
{
    Spell* curSpell = m_currentSpell[spellType];
    if (curSpell != nullptr)
    {
        if (spellType == CURRENT_AUTOREPEAT_SPELL)
        {
            if (isPlayer() && IsInWorld())
            {
                // Send server-side cancel message
                WorldPacket data(SMSG_CANCEL_AUTO_REPEAT, 8);
                data << GetNewGUID();
                SendMessageToSet(&data, false);
            }
        }

        // Cancel spell
        curSpell->cancel();
        m_currentSpell[spellType] = nullptr;
    }
}

bool Object::isCastingSpell(bool skipChanneled /*= false*/, bool skipAutorepeat /*= false*/, bool isAutoshoot /*= false*/) const
{
    // Check generic spell, but ignore finished spells
    if (m_currentSpell[CURRENT_GENERIC_SPELL] != nullptr && m_currentSpell[CURRENT_GENERIC_SPELL]->getState() != SPELL_STATE_FINISHED && m_currentSpell[CURRENT_GENERIC_SPELL]->getCastTimeLeft() > 0 &&
        (!isAutoshoot || !(m_currentSpell[CURRENT_GENERIC_SPELL]->getSpellInfo()->getAttributesExB() & ATTRIBUTESEXB_NOT_RESET_AUTO_ATTACKS)))
    {
        return true;
    }

    // If not skipped, check channeled spell
    if (!skipChanneled && m_currentSpell[CURRENT_CHANNELED_SPELL] != nullptr && m_currentSpell[CURRENT_CHANNELED_SPELL]->getState() != SPELL_STATE_FINISHED &&
        (!isAutoshoot || !(m_currentSpell[CURRENT_CHANNELED_SPELL]->getSpellInfo()->getAttributesExB() & ATTRIBUTESEXB_NOT_RESET_AUTO_ATTACKS)))
    {
        return true;
    }

    // If not skipped, check autorepeat spell
    if (!skipAutorepeat && m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr)
    {
        return true;
    }
    return false;
}

Spell* Object::findCurrentCastedSpellBySpellId(uint32_t spellId)
{
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] == nullptr)
            continue;
        if (m_currentSpell[i]->getSpellInfo()->getId() == spellId)
            return m_currentSpell[i];
    }
    return nullptr;
}

DamageInfo Object::doSpellDamage(Unit* victim, uint32_t spellId, float_t dmg, uint8_t effIndex, bool isTriggered/* = false*/, bool isPeriodic/* = false*/, bool isLeech/* = false*/, bool forceCrit/* = false*/, Spell* spell/* = nullptr*/, Aura* aur/* = nullptr*/, AuraEffectModifier* aurEff/* = nullptr*/)
{
    if (victim == nullptr || !victim->isAlive())
        return DamageInfo();

    const auto spellInfo = sSpellMgr.getSpellInfo(spellId);
    if (spellInfo == nullptr)
        return DamageInfo();

    float_t damage = dmg;
    const auto school = spellInfo->getFirstSchoolFromSchoolMask();

    // Create damage info
    DamageInfo dmgInfo;
    dmgInfo.schoolMask = SchoolMask(spellInfo->getSchoolMask());
    dmgInfo.isCritical = forceCrit;
    dmgInfo.isPeriodic = isPeriodic;

    // Check if victim is immune to this school
    // or if victim has god mode cheat
    if (victim->SchoolImmunityList[school] != 0 ||
        (victim->isPlayer() && static_cast<Player*>(victim)->m_cheats.hasGodModeCheat))
    {
        if (isCreatureOrPlayer())
            static_cast<Unit*>(this)->sendSpellOrDamageImmune(getGuid(), victim, spellId);

        return DamageInfo();
    }

    // Setup proc flags
    dmgInfo.victimProcFlags = PROC_ON_TAKEN_ANY_DAMAGE;

    if (!isPeriodic)
    {
        switch (spellInfo->getDmgClass())
        {
            case SPELL_DMG_TYPE_NONE:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_NEGATIVE_SPELL_DAMAGE_CLASS_NONE;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_NEGATIVE_SPELL_DAMAGE_CLASS_NONE;
                break;
            case SPELL_DMG_TYPE_MAGIC:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_NEGATIVE_SPELL_DAMAGE_CLASS_MAGIC;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_NEGATIVE_SPELL_DAMAGE_CLASS_MAGIC;
                break;
            case SPELL_DMG_TYPE_MELEE:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_MELEE_SPELL_HIT;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_MELEE_SPELL_HIT;
                break;
            case SPELL_DMG_TYPE_RANGED:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_RANGED_SPELL_HIT;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_RANGED_SPELL_HIT;
                break;
            default:
                break;
        }
    }
    else
    {
        dmgInfo.attackerProcFlags |= PROC_ON_DONE_PERIODIC;
        dmgInfo.victimProcFlags |= PROC_ON_TAKEN_PERIODIC;
    }

    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        casterUnit->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_START_ATTACK);
    }

    // Mage talent - Torment the Weak
    if (isPlayer() && (victim->HasAuraWithMechanics(MECHANIC_ENSNARED) || victim->HasAuraWithMechanics(MECHANIC_DAZED)))
    {
        const auto pct = static_cast<float_t>(static_cast<Player*>(this)->m_IncreaseDmgSnaredSlowed);
        damage = damage * (1.0f + (pct / 100.0f));
    }

    // Add creature type bonuses
    if (isPlayer() && victim->isCreature())
    {
        const auto plr = static_cast<Player*>(this);
        const auto cre = static_cast<Creature*>(victim);
        const auto type = cre->GetCreatureProperties()->Type;

        damage += damage * plr->IncreaseDamageByTypePCT[type];
        if (isPeriodic)
        {
            if (aur != nullptr && aurEff != nullptr)
                damage += static_cast<float_t>(plr->IncreaseDamageByType[type] / aur->getPeriodicTickCountForEffect(aurEff->getEffectIndex()));
        }
        else
        {
            damage += static_cast<float_t>(plr->IncreaseDamageByType[type]);
        }
    }

    // Check if spell can crit
    if (damage > 0.0f && isCreatureOrPlayer())
    {
        // If the damage is forced to be crit, do not alter it
        if (!dmgInfo.isCritical && !(spellInfo->getAttributesExB() & ATTRIBUTESEXB_CANT_CRIT))
        {
            if (isPeriodic)
            {
                // For periodic effects use aura's own crit chance
                if (aur != nullptr)
                    dmgInfo.isCritical = Util::checkChance(aur->getCritChance());
            }
            else if (spell != nullptr)
            {
                dmgInfo.isCritical = static_cast<Unit*>(this)->isCriticalDamageForSpell(victim, spell);
            }
        }

        // Get critical spell damage bonus
        if (dmgInfo.isCritical)
            damage = static_cast<Unit*>(this)->getCriticalDamageBonusForSpell(damage, victim, spell, aur);
    }

    // Calculate damage reduction
    damage += damage * victim->DamageTakenPctMod[school];
    damage += damage * victim->ModDamageTakenByMechPCT[spellInfo->getMechanicsType()];
    if (isPeriodic)
    {
        if (aur != nullptr && aurEff != nullptr)
            damage += static_cast<float_t>(victim->DamageTakenMod[school] / aur->getPeriodicTickCountForEffect(aurEff->getEffectIndex()));
    }
    else
    {
        damage += static_cast<float_t>(victim->DamageTakenMod[school]);
    }

    // Resilience
    ///\ todo: move this elsewhere and fix this
    float_t damageReductionPct = 1.0f;
    if (victim->isPlayer())
    {
        auto resilienceValue = static_cast<Player*>(victim)->CalcRating(CR_CRIT_TAKEN_SPELL) / 100.0f;
        if (resilienceValue > 1.0f)
            resilienceValue = 1.0f;
        damageReductionPct -= resilienceValue;
    }
    damage *= damageReductionPct;

    if (damage < 0.0f)
        damage = 0.0f;

    // For periodic auras convert the float amount to integer and save the damage fraction for next tick
    if (isPeriodic && aur != nullptr && aurEff != nullptr)
    {
        damage += aurEff->getEffectDamageFraction();
        if (aur->getTimeLeft() >= aurEff->getEffectAmplitude())
        {
            dmgInfo.fullDamage = static_cast<int32_t>(damage);
            aurEff->setEffectDamageFraction(damage - dmgInfo.fullDamage);
        }
        else
        {
            // In case this is the last tick, just round the value
            dmgInfo.fullDamage = static_cast<int32_t>(std::round(damage));
        }
    }
    else
    {
        // If this is a direct damage spell just round the value
        dmgInfo.fullDamage = static_cast<int32_t>(std::round(damage));
    }

    dmgInfo.realDamage = dmgInfo.fullDamage;

    // Calculate resistance reduction
    if (dmgInfo.realDamage > 0 && isCreatureOrPlayer() && !(isPeriodic && school == SCHOOL_NORMAL))
    {
        static_cast<Unit*>(this)->CalculateResistanceReduction(victim, &dmgInfo, spellInfo, 0.0f);
        if (dmgInfo.resistedDamage > static_cast<uint32_t>(dmgInfo.fullDamage))
            dmgInfo.realDamage = 0;
        else
            dmgInfo.realDamage = dmgInfo.fullDamage - dmgInfo.resistedDamage;
    }

    // Check for absorb effects
    dmgInfo.absorbedDamage = victim->absorbDamage(SchoolMask(spellInfo->getSchoolMask()), &dmgInfo.realDamage);
    const auto manaShieldAbsorb = victim->ManaShieldAbsorb(dmgInfo.realDamage);
    if (manaShieldAbsorb > 0)
    {
        if (manaShieldAbsorb > dmgInfo.realDamage)
            dmgInfo.realDamage = 0;
        else
            dmgInfo.realDamage -= manaShieldAbsorb;

        dmgInfo.absorbedDamage += manaShieldAbsorb;
    }

    // Hackfix from legacy method
    // Incanter's Absorption
    if (victim->isPlayer())
    {
        uint32 incanterSAbsorption[] =
        {
            //SPELL_HASH_INCANTER_S_ABSORPTION
            44394,
            44395,
            44396,
            44413,
            0
        };

        if (victim->hasAurasWithId(incanterSAbsorption))
        {
            float_t pctmod = 0.0f;
            const auto pl = static_cast<Player*>(victim);
            if (pl->hasAurasWithId(44394))
                pctmod = 0.05f;
            else if (pl->hasAurasWithId(44395))
                pctmod = 0.10f;
            else if (pl->hasAurasWithId(44396))
                pctmod = 0.15f;

            const auto hp = static_cast<int32_t>(0.05f * pl->getMaxHealth());
            auto spellpower = static_cast<int32_t>(pctmod * pl->getModDamageDonePositive(SCHOOL_NORMAL));

            if (spellpower > hp)
                spellpower = hp;

            SpellInfo const* entry = sSpellMgr.getSpellInfo(44413);
            SpellForcedBasePoints forcedBasePoints;
            forcedBasePoints.basePoints[0] = spellpower;
            if (entry != nullptr)
                pl->castSpell(pl->getGuid(), entry, forcedBasePoints, true);
        }
    }

    // Check for damage split target
    if (victim->m_damageSplitTarget != nullptr)
        dmgInfo.realDamage = victim->DoDamageSplitTarget(dmgInfo.realDamage, dmgInfo.schoolMask, false);

    // Get estimated overkill amount
    const auto overKill = victim->calculateEstimatedOverKillForCombatLog(dmgInfo.realDamage);

    // Send packet
    if (isPeriodic && aurEff != nullptr && isCreatureOrPlayer())
    {
        const auto wasPacketSent = victim->sendPeriodicAuraLog(GetNewGUID(), victim->GetNewGUID(), spellInfo, dmgInfo.realDamage, overKill, dmgInfo.absorbedDamage, dmgInfo.resistedDamage, aurEff->getAuraEffectType(), dmgInfo.isCritical);

        // In case periodic log couldn't be sent, send normal spell log
        // Required for leech and mana burn auras
        if (!wasPacketSent)
            victim->sendSpellNonMeleeDamageLog(this, victim, spellInfo, dmgInfo.realDamage, dmgInfo.absorbedDamage, dmgInfo.resistedDamage, 0, overKill, true, dmgInfo.isCritical);
    }
    else
    {
        victim->sendSpellNonMeleeDamageLog(this, victim, spellInfo, dmgInfo.realDamage, dmgInfo.absorbedDamage, dmgInfo.resistedDamage, 0, overKill, false, dmgInfo.isCritical);
    }

    // Create damaging health batch event
    auto healthBatch = new HealthBatchEvent;
    healthBatch->caster = dynamic_cast<Unit*>(this); // can be nullptr
    healthBatch->damageInfo = dmgInfo;
    healthBatch->isPeriodic = isPeriodic;
    healthBatch->spellInfo = spellInfo;

    if (isLeech)
    {
        const uint8_t index = aur != nullptr && aurEff->getAuraEffectType() != SPELL_AURA_NONE ? aurEff->getEffectIndex() : effIndex;
        healthBatch->isLeech = true;
        healthBatch->leechMultipleValue = spellInfo->getEffectMultipleValue(index);
    }

    victim->addHealthBatchEvent(healthBatch);

    // Tagging should happen when damage packets are sent
    const auto plrOwner = getPlayerOwner();
    if (plrOwner != nullptr && victim->isCreature() && victim->isTaggable())
    {
        victim->setTaggerGuid(getGuid());
        plrOwner->TagUnit(victim);
    }

    // Handle procs
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        victim->HandleProc(dmgInfo.victimProcFlags, casterUnit, spellInfo, dmgInfo, isTriggered);
        // If called from spell class, handle caster's procs when spell has finished all targets
        if (spell == nullptr)
            casterUnit->HandleProc(dmgInfo.attackerProcFlags, victim, spellInfo, dmgInfo, isTriggered);
    }

    if (isPlayer())
        static_cast<Player*>(this)->m_casted_amount[school] = dmgInfo.realDamage;

    // Cause push back to victim's spell casting if damage was not fully absorbed
    if (!(dmgInfo.realDamage == 0 && dmgInfo.absorbedDamage > 0) && !isPeriodic)
    {
        if (victim->getCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr && victim->getCurrentSpell(CURRENT_CHANNELED_SPELL)->getCastTimeLeft() > 0)
            victim->getCurrentSpell(CURRENT_CHANNELED_SPELL)->AddTime(school);
        else if (victim->getCurrentSpell(CURRENT_GENERIC_SPELL) != nullptr && victim->getCurrentSpell(CURRENT_GENERIC_SPELL)->getCastTimeLeft() > 0)
            victim->getCurrentSpell(CURRENT_GENERIC_SPELL)->AddTime(school);
    }

    // Creature emote on critical spell damage
    if (victim->isCreature() && !isPeriodic && dmgInfo.isCritical && dmgInfo.realDamage > 0)
    {
        const auto creatureTarget = static_cast<Creature*>(victim);
        if (creatureTarget->GetCreatureProperties()->Rank != ELITE_WORLDBOSS)
            creatureTarget->emote(EMOTE_ONESHOT_WOUNDCRITICAL);
    }

    if (dmgInfo.resistedDamage == dmgInfo.realDamage && dmgInfo.absorbedDamage == 0)
    {
        // Hackfix from legacy method
        if (victim->isPlayer())
        {
            const auto pVictim = static_cast<Player*>(victim);
            if (pVictim->m_RegenManaOnSpellResist > 0)
            {
                const auto maxMana = pVictim->getMaxPower(POWER_TYPE_MANA);
                const auto amount = static_cast<uint32_t>(maxMana * pVictim->m_RegenManaOnSpellResist);

                pVictim->energize(pVictim, 29442, amount, POWER_TYPE_MANA);
            }

            pVictim->combatResetPvPTimeout();
        }

        if (isPlayer())
            static_cast<Player*>(this)->combatResetPvPTimeout();
    }

    // Hackfix from legacy method
    if (spellInfo->getSchoolMask() & SCHOOL_MASK_SHADOW)
    {
        if (victim->isAlive() && isCreatureOrPlayer())
        {
            //Shadow Word:Death
            if (spellId == 32379 || spellId == 32996 || spellId == 48157 || spellId == 48158)
            {
                auto backfireAmount = dmgInfo.realDamage + dmgInfo.absorbedDamage;
                static_cast<Unit*>(this)->addSimpleDamageBatchEvent(backfireAmount, static_cast<Unit*>(this), spellInfo);
            }
        }
    }

    return dmgInfo;
}

DamageInfo Object::doSpellHealing(Unit* victim, uint32_t spellId, float_t amt, bool isTriggered/* = false*/, bool isPeriodic/* = false*/, bool isLeech/* = false*/, bool forceCrit/* = false*/, Spell* spell/* = nullptr*/, Aura* aur/* = nullptr*/, AuraEffectModifier* aurEff/* = nullptr*/)
{
    if (victim == nullptr || !victim->isAlive())
        return DamageInfo();

    const auto spellInfo = sSpellMgr.getSpellInfo(spellId);
    if (spellInfo == nullptr)
        return DamageInfo();

    float_t heal = amt;
    const auto school = spellInfo->getFirstSchoolFromSchoolMask();

    // Create damage info
    DamageInfo dmgInfo;
    dmgInfo.schoolMask = SchoolMask(spellInfo->getSchoolMask());
    dmgInfo.isHeal = true;
    dmgInfo.isCritical = forceCrit;
    dmgInfo.isPeriodic = isPeriodic;

    // Setup proc flags
    if (!isPeriodic)
    {
        switch (spellInfo->getDmgClass())
        {
            case SPELL_DMG_TYPE_NONE:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_POSITIVE_SPELL_DAMAGE_CLASS_NONE;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_POSITIVE_SPELL_DAMAGE_CLASS_NONE;
                break;
            case SPELL_DMG_TYPE_MAGIC:
                dmgInfo.attackerProcFlags |= PROC_ON_DONE_POSITIVE_SPELL_DAMAGE_CLASS_MAGIC;
                dmgInfo.victimProcFlags |= PROC_ON_TAKEN_POSITIVE_SPELL_DAMAGE_CLASS_MAGIC;
                break;
            default:
                break;
        }
    }
    else
    {
        dmgInfo.attackerProcFlags |= PROC_ON_DONE_PERIODIC;
        dmgInfo.victimProcFlags |= PROC_ON_TAKEN_PERIODIC;
    }

    // Hackfixes from legacy method
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        switch (spellInfo->getId())
        {
            //SPELL_HASH_LESSER_HEALING_WAVE
            case 8004:
            case 8008:
            case 8010:
            case 10466:
            case 10467:
            case 10468:
            case 25420:
            case 27624:
            case 28849:
            case 28850:
            case 44256:
            case 46181:
            case 49275:
            case 49276:
            case 49309:
            case 66055:
            case 68115:
            case 68116:
            case 68117:
            case 75366:
            //SPELL_HASH_HEALING_WAVE
            case 331:
            case 332:
            case 547:
            case 913:
            case 939:
            case 959:
            case 8005:
            case 10395:
            case 10396:
            case 11986:
            case 12491:
            case 12492:
            case 15982:
            case 25357:
            case 25391:
            case 25396:
            case 26097:
            case 38330:
            case 43548:
            case 48700:
            case 49272:
            case 49273:
            case 51586:
            case 52868:
            case 55597:
            case 57785:
            case 58980:
            case 59083:
            case 60012:
            case 61569:
            case 67528:
            case 68318:
            case 69958:
            case 71133:
            case 75382:
            {
                //Tidal Waves
                casterUnit->RemoveAura(53390, casterUnit->getGuid());
            }
            //SPELL_HASH_CHAIN_HEAL
            case 1064:
            case 10622:
            case 10623:
            case 14900:
            case 15799:
            case 16367:
            case 25422:
            case 25423:
            case 33642:
            case 41114:
            case 42027:
            case 42477:
            case 43527:
            case 48894:
            case 54481:
            case 55458:
            case 55459:
            case 59473:
            case 69923:
            case 70425:
            case 71120:
            case 75370:
            {
                //Maelstrom Weapon
                casterUnit->removeAllAurasByIdForGuid(53817, casterUnit->getGuid());
            } break;
            case 54172: //Paladin - Divine Storm heal effect
            {
                int dmg = (int)CalculateDamage(casterUnit, victim, MELEE, nullptr, sSpellMgr.getSpellInfo(53385));    //1 hit
                int target = 0;

                for (const auto& itr : casterUnit->getInRangeObjectsSet())
                {
                    if (itr)
                    {
                        auto obj = itr;
                        if (itr->isCreatureOrPlayer() && static_cast<Unit*>(itr)->isAlive() && obj->isInRange(casterUnit, 8) && (casterUnit->GetPhase() & itr->GetPhase()))
                        {
                            // TODO: fix me!

                            //did_hit_result = Spell::DidHit(sSpellMgr.getSpellInfo(53385)->getEffect(0), static_cast<Unit*>(itr));
                            //if (did_hit_result == SPELL_DID_HIT_SUCCESS)
                                target++;
                        }
                    }
                }
                if (target > 4)
                    target = 4;

                heal = static_cast<float_t>((dmg * target) >> 2);   // 25%
            } break;
            default:
                break;
        }
    }

    // Check if heal can crit
    if (heal > 0.0f && isCreatureOrPlayer())
    {
        // If the heal is forced to be crit, do not alter it
        if (!dmgInfo.isCritical && !(spellInfo->getAttributesExB() & ATTRIBUTESEXB_CANT_CRIT))
        {
            if (isPeriodic)
            {
                // For periodic effects use aura's own crit chance
                if (aur != nullptr)
                    dmgInfo.isCritical = Util::checkChance(aur->getCritChance());
            }
            else if (spell != nullptr)
            {
                dmgInfo.isCritical = static_cast<Unit*>(this)->isCriticalHealForSpell(victim, spell);
            }
        }

        // Get critical spell heal bonus
        if (dmgInfo.isCritical)
            heal = static_cast<Unit*>(this)->getCriticalHealBonusForSpell(heal, spell, aur);
    }

    // Get target's heal taken mod
    heal += heal * victim->HealTakenPctMod[school];
    if (isPeriodic)
    {
        if (aur != nullptr && aurEff != nullptr)
            heal += static_cast<float_t>(victim->HealTakenMod[school] / aur->getPeriodicTickCountForEffect(aurEff->getEffectIndex()));
    }
    else
    {
        heal += static_cast<float_t>(victim->HealTakenMod[school]);
    }

    if (heal < 0.0f)
        heal = 0.0f;

    // For periodic auras convert the float amount to integer and save the heal fraction for next tick
    // but skip leech effects because the damage part only uses fractions
    if (isPeriodic && !isLeech && aur != nullptr && aurEff != nullptr)
    {
        heal += aurEff->getEffectDamageFraction();
        if (aur->getTimeLeft() >= aurEff->getEffectAmplitude())
        {
            dmgInfo.fullDamage = static_cast<int32_t>(heal);
            aurEff->setEffectDamageFraction(heal - dmgInfo.fullDamage);
        }
        else
        {
            // In case this is the last tick, just round the value
            dmgInfo.fullDamage = static_cast<int32_t>(std::round(heal));
        }
    }
    else
    {
        // If this is a direct heal spell just round the value
        dmgInfo.fullDamage = static_cast<int32_t>(std::round(heal));
    }

    dmgInfo.realDamage = dmgInfo.fullDamage;

    ///\ todo: implement absorbed heal (aura effect 301)
    dmgInfo.absorbedDamage = 0;

    // Calculate estimated overheal amount
    const auto overHeal = victim->calculateEstimatedOverHealForCombatLog(dmgInfo.realDamage);

    // Send packet
    if (isPeriodic && aurEff != nullptr && isCreatureOrPlayer())
    {
        const auto wasPacketSent = victim->sendPeriodicAuraLog(GetNewGUID(), victim->GetNewGUID(), spellInfo, dmgInfo.realDamage, overHeal, dmgInfo.absorbedDamage, 0, aurEff->getAuraEffectType(), dmgInfo.isCritical);

        // In case periodic log couldn't be sent, send normal spell log
        // Required for leech auras
        if (!wasPacketSent)
            victim->sendSpellHealLog(this, victim, spellId, dmgInfo.realDamage, dmgInfo.isCritical, overHeal, dmgInfo.absorbedDamage);
    }
    else
    {
        victim->sendSpellHealLog(this, victim, spellId, dmgInfo.realDamage, dmgInfo.isCritical, overHeal, dmgInfo.absorbedDamage);
    }

    // Create healing health batch
    auto healthBatch = new HealthBatchEvent;
    healthBatch->caster = dynamic_cast<Unit*>(this); // can be nullptr
    healthBatch->damageInfo = dmgInfo;
    healthBatch->isPeriodic = isPeriodic;
    healthBatch->isHeal = true;
    healthBatch->spellInfo = spellInfo;

    victim->addHealthBatchEvent(healthBatch);

    // Handle procs
    if (isCreatureOrPlayer())
    {
        const auto casterUnit = static_cast<Unit*>(this);
        victim->HandleProc(dmgInfo.victimProcFlags, casterUnit, spellInfo, dmgInfo, isTriggered);
        // If called from spell class, handle caster's procs when spell has finished all targets
        if (spell == nullptr)
            casterUnit->HandleProc(dmgInfo.attackerProcFlags, victim, spellInfo, dmgInfo, isTriggered);
    }

    if (isPlayer())
    {
        const auto plr = static_cast<Player*>(this);

        plr->last_heal_spell = spellInfo;
        plr->m_casted_amount[school] = dmgInfo.realDamage;
    }

    victim->RemoveAurasByHeal();
    return dmgInfo;
}

void Object::_UpdateSpells(uint32_t time)
{
    // Remove garbage
    removeGarbageSpells();

    // Update autorepeat spells
    if (m_currentSpell[CURRENT_AUTOREPEAT_SPELL] != nullptr && isPlayer())
        static_cast<Player*>(this)->updateAutoRepeatSpell();

    // Update traveling spells
    for (auto travelingSpellItr = m_travelingSpells.begin(); travelingSpellItr != m_travelingSpells.end();)
    {
        auto spellItr = *travelingSpellItr;

        // Remove finished spells from list
        // They will be deleted on next update tick
        if (spellItr.first == nullptr || spellItr.second)
        {
            addGarbageSpell(spellItr.first);
            travelingSpellItr = m_travelingSpells.erase(travelingSpellItr);
            continue;
        }

        spellItr.first->update(time);
        ++travelingSpellItr;
    }

    // Update current spells
    for (uint8_t i = 0; i < CURRENT_SPELL_MAX; ++i)
    {
        if (m_currentSpell[i] != nullptr)
        {
            // Remove finished spells from object's current spell array
            // If the spell is in finished state, it's already in garbage collector
            if (m_currentSpell[i]->getState() == SPELL_STATE_FINISHED)
            {
                m_currentSpell[i] = nullptr;
            }
            // Move traveling spells out from current spells
            else if (m_currentSpell[i]->getState() == SPELL_STATE_TRAVELING)
            {
                addTravelingSpell(m_currentSpell[i]);
                m_currentSpell[i] = nullptr;
            }
            // Update spells with other state
            else
            {
                m_currentSpell[i]->update(time);
            }
        }
    }
}

void Object::addTravelingSpell(Spell* spell)
{
    m_travelingSpells.insert(std::make_pair(spell, false));
}

void Object::removeTravelingSpell(Spell* spell)
{
    auto itr = m_travelingSpells.find(spell);
    if (itr != m_travelingSpells.end())
        (*itr).second = true;
    else
        addGarbageSpell(spell);
}

void Object::addGarbageSpell(Spell* spell)
{
    m_garbageSpells.push_back(spell);
}

void Object::removeGarbageSpells()
{
    for (auto itr = m_garbageSpells.begin(); itr != m_garbageSpells.end();)
    {
        delete *itr;
        itr = m_garbageSpells.erase(itr);
    }
}

void Object::removeSpellModifierFromCurrentSpells(AuraEffectModifier const* aura)
{
    for (auto& curSpell : m_currentSpell)
    {
        if (curSpell == nullptr)
            continue;

        curSpell->removeUsedSpellModifier(aura);
    }

    for (auto& travelSpell : m_travelingSpells)
    {
        travelSpell.first->removeUsedSpellModifier(aura);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// InRange sets
void Object::clearInRangeSets()
{
    mInRangeObjectsSet.clear();
    mInRangePlayersSet.clear();
    mInRangeOppositeFactionSet.clear();
    mInRangeSameFactionSet.clear();
}

void Object::addToInRangeObjects(Object* pObj)
{
    if (pObj == nullptr)
        sLogger.failure("Invalid object pointers can't be added!");

    if (pObj == this)
        sLogger.failure("We are in range of ourselves!");

    if (pObj->isPlayer())
        mInRangePlayersSet.push_back(pObj);

    mInRangeObjectsSet.push_back(pObj);
}

void Object::removeSelfFromInrangeSets()
{
    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
            itr->removeObjectFromInRangeObjectsSet(this);
    }
}

// Objects
std::vector<Object*> Object::getInRangeObjectsSet()
{
    return mInRangeObjectsSet;
}

bool Object::hasInRangeObjects()
{
    return mInRangeObjectsSet.size() > 0;
}

size_t Object::getInRangeObjectsCount()
{
    return mInRangeObjectsSet.size();
}

bool Object::isObjectInInRangeObjectsSet(Object* pObj)
{
    // Do not use std::find here - if something is added to or removed from the in range vector at the same time
    // the std::find causes a crash because vector iterator is out of range.
    // Tested with transports and gameobjects -Appled
    for (const auto& inRangeObj : mInRangeObjectsSet)
    {
        if (inRangeObj == pObj)
            return true;
    }

    return false;
}

void Object::removeObjectFromInRangeObjectsSet(Object* pObj)
{
    if (pObj != nullptr)
    {
        if (pObj->isPlayer())
            mInRangePlayersSet.erase(std::remove(mInRangePlayersSet.begin(), mInRangePlayersSet.end(), pObj), mInRangePlayersSet.end());

        mInRangeObjectsSet.erase(std::remove(mInRangeObjectsSet.begin(), mInRangeObjectsSet.end(), pObj), mInRangeObjectsSet.end());

        onRemoveInRangeObject(pObj);
    }
    else
    {
        sLogger.failure("Object::removeObjectFromInRangeObjectsSet something tried to remove invalid object pointer!");
    }
}

// Players
std::vector<Object*> Object::getInRangePlayersSet()
{
    return mInRangePlayersSet;
}

size_t Object::getInRangePlayersCount()
{
    return mInRangePlayersSet.size();
}

// Opposite Faction
std::vector<Object*> Object::getInRangeOppositeFactionSet()
{
    return mInRangeOppositeFactionSet;
}

bool Object::isObjectInInRangeOppositeFactionSet(Object* pObj)
{
    auto it = std::find(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), pObj);
    return it != mInRangeOppositeFactionSet.end();
}

void Object::updateInRangeOppositeFactionSet()
{
    mInRangeOppositeFactionSet.clear();

    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
        {
            if (itr->isCreatureOrPlayer() || itr->isGameObject())
            {
                if (isHostile(this, itr))
                {
                    if (!itr->isObjectInInRangeOppositeFactionSet(this))
                        itr->mInRangeOppositeFactionSet.push_back(this);
                    if (!isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeOppositeFactionSet.push_back(itr);
                }
                else
                {
                    if (itr->isObjectInInRangeOppositeFactionSet(this))
                        itr->mInRangeOppositeFactionSet.erase(std::remove(itr->mInRangeOppositeFactionSet.begin(), itr->mInRangeOppositeFactionSet.end(), this), itr->mInRangeOppositeFactionSet.end());
                    if (isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeOppositeFactionSet.erase(std::remove(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), itr), mInRangeOppositeFactionSet.end());
                }
            }
        }
    }
}

void Object::addInRangeOppositeFaction(Object* obj)
{
    mInRangeOppositeFactionSet.push_back(obj);
}

void Object::removeObjectFromInRangeOppositeFactionSet(Object* obj)
{
    mInRangeOppositeFactionSet.erase(std::remove(mInRangeOppositeFactionSet.begin(), mInRangeOppositeFactionSet.end(), obj), mInRangeOppositeFactionSet.end());
}

// Same Faction
std::vector<Object*> Object::getInRangeSameFactionSet()
{
    return mInRangeSameFactionSet;
}

bool Object::isObjectInInRangeSameFactionSet(Object* pObj)
{
    auto it = std::find(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), pObj);
    return it != mInRangeSameFactionSet.end();
}

void Object::updateInRangeSameFactionSet()
{
    mInRangeSameFactionSet.clear();

    for (const auto& itr : mInRangeObjectsSet)
    {
        if (itr)
        {
            if (itr->isCreatureOrPlayer() || itr->isGameObject())
            {
                if (isFriendly(this, itr))
                {
                    if (!itr->isObjectInInRangeSameFactionSet(this))
                        itr->mInRangeSameFactionSet.push_back(this);

                    if (!isObjectInInRangeOppositeFactionSet(itr))
                        mInRangeSameFactionSet.push_back(itr);
                }
                else
                {
                    if (itr->isObjectInInRangeSameFactionSet(this))
                        itr->mInRangeSameFactionSet.erase(std::remove(itr->mInRangeSameFactionSet.begin(), itr->mInRangeSameFactionSet.end(), this), itr->mInRangeSameFactionSet.end());

                    if (isObjectInInRangeSameFactionSet(itr))
                        mInRangeSameFactionSet.erase(std::remove(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), itr), mInRangeSameFactionSet.end());
                }
            }
        }
    }
}

void Object::addInRangeSameFaction(Object* obj)
{
    mInRangeSameFactionSet.push_back(obj);
}

void Object::removeObjectFromInRangeSameFactionSet(Object* obj)
{
    mInRangeSameFactionSet.erase(std::remove(mInRangeSameFactionSet.begin(), mInRangeSameFactionSet.end(), obj), mInRangeSameFactionSet.end());
}

//////////////////////////////////////////////////////////////////////////////////////////
// Owner
Player* Object::getPlayerOwner() { return nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////
// Misc
void Object::sendGameobjectDespawnAnim()
{
    SendMessageToSet(SmsgGameobjectDespawnAnim(this->getGuid()).serialise().get(), true);
}

//////////////////////////////////////////////////////////////////////////////////////////
// AGPL Starts

::DBC::Structures::AreaTableEntry const* Object::GetArea()
{
    if (!this->IsInWorld()) return nullptr;

    auto map_mgr = this->GetMapMgr();
    if (!map_mgr) return nullptr;

    auto area_flag = map_mgr->GetAreaFlag(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ());
    auto at = MapManagement::AreaManagement::AreaStorage::GetAreaByFlag(area_flag);
    if (!at)
        at = MapManagement::AreaManagement::AreaStorage::GetAreaByMapId(this->GetMapId());

    return at;
}

void Object::_Create(uint32 mapid, float x, float y, float z, float ang)
{
    m_mapId = mapid;
    m_position.ChangeCoords({ x, y, z, ang });
    m_spawnLocation.ChangeCoords({ x, y, z, ang });
    m_lastMapUpdatePosition.ChangeCoords({ x, y, z, ang });
}

void Object::BuildFieldUpdatePacket(Player* Target, uint32 Index, uint32 Value)
{
    ByteBuffer buf(500);
    buf << uint8(UPDATETYPE_VALUES);
#if VERSION_STRING < Mop
    buf << GetNewGUID();
#else
    buf.append(GetNewGUID());
#endif

    uint32 mBlocks = Index / 32 + 1;
    buf << uint8(mBlocks);

    for (uint32 dword_n = mBlocks - 1; dword_n; dword_n--)
        buf << uint32(0);

    buf << (((uint32)(1)) << (Index % 32));
    buf << Value;

    Target->getUpdateMgr().pushUpdateData(&buf, 1);
}

void Object::BuildFieldUpdatePacket(ByteBuffer* buf, uint32 Index, uint32 Value)
{
    *buf << uint8(UPDATETYPE_VALUES);
#if VERSION_STRING < Mop
    *buf << GetNewGUID();
#else
    buf->append(GetNewGUID());
#endif

    uint32 mBlocks = Index / 32 + 1;
    *buf << uint8(mBlocks);

    for (uint32 dword_n = mBlocks - 1; dword_n; dword_n--)
        *buf << uint32(0);

    *buf << (((uint32)(1)) << (Index % 32));
    *buf << Value;
}

uint32 Object::BuildValuesUpdateBlockForPlayer(ByteBuffer* data, Player* target)
{
    UpdateMask updateMask;
    updateMask.SetCount(m_valuesCount);
    _SetUpdateBits(&updateMask, target);
    for (uint32 x = 0; x < m_valuesCount; ++x)
    {
        if (updateMask.GetBit(x))
        {
            if (m_wowGuid.GetNewGuidLen() > 0)
            {
                *data << uint8(UPDATETYPE_VALUES);              // update type == update
                *data << m_wowGuid;

                buildValuesUpdate(data, &updateMask, target);
#if VERSION_STRING == Mop
                * data << uint8_t(0);
#endif
                return 1;
            }

            sLogger.failure("Object::BuildValuesUpdateBlockForPlayer tried to add data for invalid guid!");
            return 0;
        }
    }

    return 0;
}

uint32 Object::BuildValuesUpdateBlockForPlayer(ByteBuffer* buf, UpdateMask* mask)
{
    // returns: update count
    // update type == update
    if (m_wowGuid.GetNewGuidLen() > 0)
    {
        *buf << uint8(UPDATETYPE_VALUES);
        *buf << m_wowGuid;

        buildValuesUpdate(buf, mask, nullptr);
#if VERSION_STRING == Mop
        * buf << uint8_t(0);
#endif
        // 1 update.
        return 1;
    }

    sLogger.failure("Object::BuildValuesUpdateBlockForPlayer tried to add data for invalid guid!");
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Build the Movement Data portion of the update packet Fills the data with this object's movement/speed info
///\todo rewrite this stuff, document unknown fields and flags


// MIT Start
#if VERSION_STRING == Classic
void Object::buildMovementUpdate(ByteBuffer* data, uint8_t updateFlags, Player* target)
{
    *data << uint8(updateFlags);

    if (updateFlags & UPDATEFLAG_LIVING)  //0x20
    {
        *data << uint32(obj_movement_info.getMovementFlags());

        *data << Util::getMSTime();

        *data << float(m_position.x);
        *data << float(m_position.y);
        *data << float(m_position.z);
        *data << float(m_position.o);

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_TRANSPORT)) //0x0200
        {
            *data << obj_movement_info.transport_guid;
            *data << float(GetTransOffsetX());
            *data << float(GetTransOffsetY());
            *data << float(GetTransOffsetZ());
            *data << float(GetTransOffsetO());
        }

        if (obj_movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)))   // 0x2000000+0x0200000 flying/swimming, || sflags & SMOVE_FLAG_ENABLE_PITCH
        {
            *data << obj_movement_info.pitch_rate;
        }

        *data << obj_movement_info.fall_time;

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_FALLING))   // 0x00001000
        {

            *data << obj_movement_info.jump_info.velocity;
            *data << obj_movement_info.jump_info.cosAngle;
            *data << obj_movement_info.jump_info.sinAngle;
            *data << obj_movement_info.jump_info.xyspeed;
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
            *data << float(obj_movement_info.spline_elevation);

        if (Unit* unit = static_cast<Unit*>(this))
        {
            *data << unit->getSpeedRate(TYPE_WALK, true);
            *data << unit->getSpeedRate(TYPE_RUN, true);
            *data << unit->getSpeedRate(TYPE_RUN_BACK, true);
            *data << unit->getSpeedRate(TYPE_SWIM, true);
            *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);
            *data << unit->getSpeedRate(TYPE_TURN_RATE, true);
        }
        else                                //\todo Zyres: this is ridiculous... only units have these types, but this function is a mess so don't breake anything.
        {
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(4.722222f);
            *data << float(2.5f);
            *data << float(3.141594f);
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ENABLED))   //VLack: On Mangos this is a nice spline movement code, but we never had such... Also, at this point we haven't got this flag, that's for sure, but fail just in case...
        {
            if (Unit* unit = static_cast<Unit*>(this))
                MovementNew::PacketBuilder::WriteCreate(*unit->movespline, *data);
        }
    }
    else        // No UPDATEFLAG_LIVING
    {
        if (updateFlags & UPDATEFLAG_HAS_POSITION)  //0x40
        {
            if (updateFlags & UPDATEFLAG_TRANSPORT)
            {
                *data << float(GetTransOffsetX());
                *data << float(GetTransOffsetY());
                *data << float(GetTransOffsetZ());
                *data << float(GetTransOffsetO());
            }
            else
            {
                *data << float(m_position.x);
                *data << float(m_position.y);
                *data << float(m_position.z);
                *data << float(m_position.o);
            }
        }
    }

    if (updateFlags & UPDATEFLAG_HIGHGUID)
        *data << uint32(GetNewGUID().getGuidHighPart());

    if (updateFlags & UPDATEFLAG_ALL)
        *data << uint32(0x1);

    if (updateFlags & UPDATEFLAG_HAS_TARGET)  //0x04
    {
        if (isCreatureOrPlayer())
            FastGUIDPack(*data, static_cast<Unit*>(this)->getTargetGuid()); //some compressed GUID
        else
            *data << uint64(0);
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)   //0x2
    {
        GameObject const* go = static_cast<GameObject*>(this);
        if (go && go->ToTransport())
            *data << uint32_t(go->GetTransValues()->PathProgress);
        else
            *data << Util::getMSTime();
    }
}
#endif

#if VERSION_STRING == TBC
void Object::buildMovementUpdate(ByteBuffer* data, uint8_t updateFlags, Player* target)
{
    *data << uint8(updateFlags);

    if (updateFlags & UPDATEFLAG_LIVING)  //0x20
    {
        *data << uint32(obj_movement_info.getMovementFlags());

        *data << uint8(obj_movement_info.getMovementFlags2());

        *data << Util::getMSTime();

        *data << float(m_position.x);
        *data << float(m_position.y);
        *data << float(m_position.z);
        *data << float(m_position.o);

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_TRANSPORT)) //0x0200
        {
            *data << obj_movement_info.transport_guid;
            *data << float(GetTransOffsetX());
            *data << float(GetTransOffsetY());
            *data << float(GetTransOffsetZ());
            *data << float(GetTransOffsetO());
            *data << uint32(GetTransTime());
        }

        if (obj_movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || obj_movement_info.hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))   // 0x2000000+0x0200000 flying/swimming, || sflags & SMOVE_FLAG_ENABLE_PITCH
        {
            *data << obj_movement_info.pitch_rate;
        }

        *data << obj_movement_info.fall_time;

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_FALLING))   // 0x00001000
        {

            *data << obj_movement_info.jump_info.velocity;
            *data << obj_movement_info.jump_info.cosAngle;
            *data << obj_movement_info.jump_info.sinAngle;
            *data << obj_movement_info.jump_info.xyspeed;
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
            *data << float(obj_movement_info.spline_elevation);

        if (Unit* unit = static_cast<Unit*>(this))
        {
            *data << unit->getSpeedRate(TYPE_WALK, true);
            *data << unit->getSpeedRate(TYPE_RUN, true);
            *data << unit->getSpeedRate(TYPE_RUN_BACK, true);
            *data << unit->getSpeedRate(TYPE_SWIM, true);
            *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);
            *data << unit->getSpeedRate(TYPE_FLY, true);
            *data << unit->getSpeedRate(TYPE_FLY_BACK, true);
            *data << unit->getSpeedRate(TYPE_TURN_RATE, true);
        }
        else                                //\todo Zyres: this is ridiculous... only units have these types, but this function is a mess so don't breake anything.
        {
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(4.722222f);
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(3.141594f);
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ENABLED))   //VLack: On Mangos this is a nice spline movement code, but we never had such... Also, at this point we haven't got this flag, that's for sure, but fail just in case...
        {
            if (Unit* unit = static_cast<Unit*>(this))
                MovementNew::PacketBuilder::WriteCreate(*unit->movespline, *data);
        }
    }
    else        // No UPDATEFLAG_LIVING
    {
        if (updateFlags & UPDATEFLAG_HAS_POSITION)  //0x40
        {
            if (updateFlags & UPDATEFLAG_TRANSPORT)
            {
                *data << float(GetTransOffsetX());
                *data << float(GetTransOffsetY());
                *data << float(GetTransOffsetZ());
                *data << float(GetTransOffsetO());
            }
            else
            {
                *data << float(m_position.x);
                *data << float(m_position.y);
                *data << float(m_position.z);
                *data << float(m_position.o);
            }
        }
    }

    if (updateFlags & UPDATEFLAG_LOWGUID)    //0x10
    {
        switch (getOType())
        {
        case TYPEID_OBJECT:
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
        case TYPEID_GAMEOBJECT:
        case TYPEID_DYNAMICOBJECT:
        case TYPEID_CORPSE:
            *data << uint32(GetNewGUID().getGuidLowPart());
            break;
        case TYPEID_UNIT:
            *data << uint32(0x0000000B);                // unk
            break;
        case TYPEID_PLAYER:
            if (updateFlags & UPDATEFLAG_SELF)
                *data << uint32(0x00000015);            // unk
            else
                *data << uint32(0x00000008);            // unk
            break;
        default:
            *data << uint32(0x00000000);                // unk
            break;
        }
    }

    if (updateFlags & UPDATEFLAG_HIGHGUID)
    {
        switch (getOType())
        {
        case TYPEID_OBJECT:
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
        case TYPEID_GAMEOBJECT:
        case TYPEID_DYNAMICOBJECT:
        case TYPEID_CORPSE:
            *data << uint32(GetNewGUID().getGuidHighPart()); // GetGUIDHigh()
            break;
        default:
            *data << uint32(0x00000000);                // unk
            break;
        }
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)  //0x04
    {
        if (isCreatureOrPlayer())
            FastGUIDPack(*data, static_cast<Unit*>(this)->getTargetGuid()); //some compressed GUID
        else
            *data << uint64(0);
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)   //0x2
    {
        GameObject const* go = static_cast<GameObject*>(this);
        if (go && go->ToTransport())
            *data << uint32_t(go->GetTransValues()->PathProgress);
        else
            *data << Util::getMSTime();
    }
}
#endif

#if VERSION_STRING == WotLK
void Object::buildMovementUpdate(ByteBuffer* data, uint16_t updateFlags, Player* /*target*/)
{
    *data << uint16(updateFlags);

    if (updateFlags & UPDATEFLAG_LIVING)  //0x20
    {
        *data << uint32(obj_movement_info.getMovementFlags());

        *data << uint16(obj_movement_info.getMovementFlags2());

        *data << Util::getMSTime();

        *data << float(m_position.x);
        *data << float(m_position.y);
        *data << float(m_position.z);
        *data << float(m_position.o);

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_TRANSPORT)) //0x0200
        {
            *data << WoWGuid(obj_movement_info.transport_guid);
            *data << float(GetTransOffsetX());
            *data << float(GetTransOffsetY());
            *data << float(GetTransOffsetZ());
            *data << float(GetTransOffsetO());
            *data << uint32(GetTransTime());
            *data << uint8(GetTransSeat());

            if (obj_movement_info.hasMovementFlag2(MOVEFLAG2_INTERPOLATED_MOVE))
                *data << uint32(obj_movement_info.transport_time2);
        }

        if (obj_movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || obj_movement_info.hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))   // 0x2000000+0x0200000 flying/swimming, || sflags & SMOVE_FLAG_ENABLE_PITCH
        {
            *data << obj_movement_info.pitch_rate;
        }

        *data << obj_movement_info.fall_time;

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_FALLING))   // 0x00001000
        {

                *data << obj_movement_info.jump_info.velocity;
                *data << obj_movement_info.jump_info.sinAngle;
                *data << obj_movement_info.jump_info.cosAngle;
                *data << obj_movement_info.jump_info.xyspeed;
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
            *data << float(obj_movement_info.spline_elevation);

        if (Unit* unit = static_cast<Unit*>(this))
        {
            *data << unit->getSpeedRate(TYPE_WALK, true);
            *data << unit->getSpeedRate(TYPE_RUN, true);
            *data << unit->getSpeedRate(TYPE_RUN_BACK, true);
            *data << unit->getSpeedRate(TYPE_SWIM, true);
            *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);
            *data << unit->getSpeedRate(TYPE_FLY, true);
            *data << unit->getSpeedRate(TYPE_FLY_BACK, true);
            *data << unit->getSpeedRate(TYPE_TURN_RATE, true);
            *data << unit->getSpeedRate(TYPE_PITCH_RATE, true);
        }
        else                                //\todo Zyres: this is ridiculous... only units have these types, but this function is a mess so don't breake anything.
        {
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(4.722222f);
            *data << float(2.5f);
            *data << float(7.0f);
            *data << float(4.5f);
            *data << float(3.141594f);
            *data << float(3.14f);
        }

        if (obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ENABLED))   //VLack: On Mangos this is a nice spline movement code, but we never had such... Also, at this point we haven't got this flag, that's for sure, but fail just in case...
        {
            if (Unit* unit = static_cast<Unit*>(this))
                MovementNew::PacketBuilder::WriteCreate(*unit->movespline, *data);
        }

    }
    else        // No UPDATEFLAG_LIVING
    {
        if (updateFlags & UPDATEFLAG_POSITION)        //0x0100
        {
            Transporter* transport = GetTransport();

            if (transport)
                *data << WoWGuid(transport->getGuid());
            else
                *data << uint8(0);

            *data << float(m_position.x);
            *data << float(m_position.y);
            *data << float(m_position.z);

            if (transport)
            {
                *data << float(GetTransOffsetX());
                *data << float(GetTransOffsetY());
                *data << float(GetTransOffsetZ());
            }
            else
            {
                *data << float(m_position.x);
                *data << float(m_position.y);
                *data << float(m_position.z);
            }

            *data << float(m_position.o);

            if (isCorpse())
                *data << float(m_position.o);   //VLack: repeat the orientation!
            else
                *data << float(0);
        }
        else if (updateFlags & UPDATEFLAG_HAS_POSITION)  //0x40
        {
            *data << float(m_position.x);
            *data << float(m_position.y);
            *data << float(m_position.z);
            *data << float(m_position.o);
        }
    }

    if (updateFlags & UPDATEFLAG_UNKNOWN)     //0x08
        *data << uint32(0);

    if (updateFlags & UPDATEFLAG_LOWGUID)    //0x10
    {
        switch (getOType())
        {
        case TYPEID_OBJECT:
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
        case TYPEID_GAMEOBJECT:
        case TYPEID_DYNAMICOBJECT:
        case TYPEID_CORPSE:
            *data << uint32(GetNewGUID().getGuidLowPart());
            break;
        case TYPEID_UNIT:
            *data << uint32(0x0000000B);                // unk
            break;
        case TYPEID_PLAYER:
            if (updateFlags & UPDATEFLAG_SELF)
                *data << uint32(0x0000002F);            // unk
            else
                *data << uint32(0x00000008);            // unk
            break;
        default:
            *data << uint32(0x00000000);                // unk
            break;
        }
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)  //0x04
    {
        if (isCreatureOrPlayer())
            FastGUIDPack(*data, static_cast<Unit*>(this)->getTargetGuid()); //some compressed GUID
        else
            *data << uint64(0);
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)   //0x2
    {
        GameObject const* go = static_cast<GameObject*>(this);
        if (go && go->ToTransport())
            *data << uint32_t(go->GetTransValues()->PathProgress);
        else
            *data << Util::getMSTime();
    }

    if (updateFlags & UPDATEFLAG_VEHICLE)
    {
        uint32 vehicleid = 0;

        if (isCreature())
            vehicleid = static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid;
        else
            if (isPlayer())
                vehicleid = static_cast<Player*>(this)->mountvehicleid;

        *data << uint32(vehicleid);
        *data << float(GetTransOffsetO());
    }

    if (updateFlags & UPDATEFLAG_ROTATION)   //0x0200
    {
        if (isGameObject())
            *data << static_cast<GameObject*>(this)->GetRotation();
    }
}
#endif

#if VERSION_STRING == Cata
void Object::buildMovementUpdate(ByteBuffer* data, uint16_t updateFlags, Player* /*target*/)
{
    ObjectGuid Guid = getGuid();
    uint32_t movementFlags = 0;
    uint16_t movementFlagsExtra = 0;

    bool hasTransportTime2 = false;
    bool hasVehicleId = false;
    bool hasFallDirection = false;
    bool hasFallData = false;
    bool hasPitch = false;
    bool hasSplineElevation = false;
    bool hasAIAnimKit = false;
    bool hasMovementAnimKit = false;
    bool hasMeleeAnimKit = false;

    uint32_t stopFrameCount = 0;

    data->writeBit(updateFlags & UPDATEFLAG_PLAY_HOVER_ANIM);
    data->writeBit(updateFlags & UPDATEFLAG_SUPPRESSED_GREETINGS);
    data->writeBit(updateFlags & UPDATEFLAG_ROTATION);
    data->writeBit(updateFlags & UPDATEFLAG_ANIM_KITS);
    data->writeBit(updateFlags & UPDATEFLAG_HAS_TARGET);
    data->writeBit(updateFlags & UPDATEFLAG_SELF);
    data->writeBit(updateFlags & UPDATEFLAG_VEHICLE);
    data->writeBit(updateFlags & UPDATEFLAG_LIVING);
    data->writeBits(stopFrameCount, 24);
    data->writeBit(updateFlags & UPDATEFLAG_NO_BIRTH_ANIM);
    data->writeBit(updateFlags & UPDATEFLAG_POSITION); //UPDATEFLAG_GO_TRANSPORT_POSITION
    data->writeBit(updateFlags & UPDATEFLAG_HAS_POSITION);   //UPDATEFLAG_STATIONARY_POSITION
    data->writeBit(updateFlags & UPDATEFLAG_AREATRIGGER);
    data->writeBit(updateFlags & UPDATEFLAG_ENABLE_PORTALS);
    data->writeBit(updateFlags & UPDATEFLAG_TRANSPORT);

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;
        movementFlags = obj_movement_info.getMovementFlags();
        movementFlagsExtra = obj_movement_info.getMovementFlags2();

        hasTransportTime2 = obj_movement_info.transport_guid != 0 && obj_movement_info.transport_time2 != 0;
        hasVehicleId = unit->getCurrentVehicle() && unit->getCurrentVehicle()->GetVehicleInfo();
        hasPitch = obj_movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || obj_movement_info.hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING);
        hasFallDirection = obj_movement_info.hasMovementFlag2(MOVEFLAG2_INTERPOLATED_TURN);
        hasFallData = hasFallDirection || obj_movement_info.fall_time != 0;
        hasSplineElevation = obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION);

        data->writeBit(!movementFlags);
        data->writeBit(G3D::fuzzyEq(GetOrientation(), 0.0f));

        data->writeBit(Guid[7]);
        data->writeBit(Guid[3]);
        data->writeBit(Guid[2]);

        if (movementFlags)
            data->writeBits(movementFlags, 30);

        data->writeBit(unit->isSplineEnabled() && !isPlayer());
        data->writeBit(!hasPitch);
        data->writeBit(unit->isSplineEnabled());
        data->writeBit(hasFallData);
        data->writeBit(!hasSplineElevation);
        data->writeBit(Guid[5]);
        data->writeBit(obj_movement_info.transport_guid != 0);
        data->writeBit(0);

        if (obj_movement_info.transport_guid != 0)
        {
            ObjectGuid tGuid = obj_movement_info.transport_guid;

            data->writeBit(tGuid[1]);
            data->writeBit(hasTransportTime2);
            data->writeBit(tGuid[4]);
            data->writeBit(tGuid[0]);
            data->writeBit(tGuid[6]);
            data->writeBit(hasVehicleId);
            data->writeBit(tGuid[7]);
            data->writeBit(tGuid[5]);
            data->writeBit(tGuid[3]);
            data->writeBit(tGuid[2]);
        }

        data->writeBit(Guid[4]);

        if (unit->isSplineEnabled())
            MovementNew::PacketBuilder::WriteCreateBits(*unit->movespline, *data);

        data->writeBit(Guid[6]);

        if (hasFallData)
            data->writeBit(hasFallDirection);

        data->writeBit(Guid[0]);
        data->writeBit(Guid[1]);

        data->writeBit(0);
        data->writeBit(!movementFlagsExtra);

        if (movementFlagsExtra)
            data->writeBits(movementFlagsExtra, 12);
    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid = obj_movement_info.transport_guid;

        data->writeBit(transGuid[5]);
        data->writeBit(hasVehicleId);
        data->writeBit(transGuid[0]);
        data->writeBit(transGuid[3]);
        data->writeBit(transGuid[6]);
        data->writeBit(transGuid[1]);
        data->writeBit(transGuid[4]);
        data->writeBit(transGuid[2]);
        data->writeBit(hasTransportTime2);
        data->writeBit(transGuid[7]);
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->writeBit(victimGuid[2]);
            data->writeBit(victimGuid[7]);
            data->writeBit(victimGuid[0]);
            data->writeBit(victimGuid[4]);
            data->writeBit(victimGuid[5]);
            data->writeBit(victimGuid[6]);
            data->writeBit(victimGuid[1]);
            data->writeBit(victimGuid[3]);
        }
        else
            data->writeBits(0, 8);
    }

    if (updateFlags & UPDATEFLAG_ANIM_KITS)
    {
        data->writeBit(true);
        data->writeBit(true);
        data->writeBit(true);
    }

    data->flushBits();

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;

        data->WriteByteSeq(Guid[4]);

        *data << unit->getSpeedRate(TYPE_RUN_BACK, true);

        if (hasFallData)
        {
            if (hasFallDirection)
            {
                *data << float(obj_movement_info.jump_info.xyspeed);
                *data << float(obj_movement_info.jump_info.sinAngle);
                *data << float(obj_movement_info.jump_info.cosAngle);
            }

            *data << uint32_t(obj_movement_info.fall_time);
            *data << float(obj_movement_info.jump_info.velocity);
        }

        *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);

        if (hasSplineElevation)
            *data << float(obj_movement_info.spline_elevation);

        if (unit->isSplineEnabled())
        {
            MovementNew::PacketBuilder::WriteCreateData(*unit->movespline, *data);
        }

        *data << float(unit->GetPositionZ());
        data->WriteByteSeq(Guid[5]);

        if (obj_movement_info.transport_guid)
        {
            ObjectGuid tGuid = obj_movement_info.transport_guid;

            data->WriteByteSeq(tGuid[5]);
            data->WriteByteSeq(tGuid[7]);

            *data << uint32(obj_movement_info.transport_time);
            *data << float(normalizeOrientation(GetTransOffsetO()));

            if (hasTransportTime2)
                *data << uint32_t(obj_movement_info.transport_time2);

            *data << float(GetTransOffsetY());
            *data << float(GetTransOffsetX());

            data->WriteByteSeq(tGuid[3]);

            *data << float(GetTransOffsetZ());

            data->WriteByteSeq(tGuid[0]);

            if (hasVehicleId)
                *data << uint32_t(unit->getCurrentVehicle()->GetVehicleInfo()->ID);

            *data << int8_t(obj_movement_info.transport_seat);

            data->WriteByteSeq(tGuid[1]);
            data->WriteByteSeq(tGuid[6]);
            data->WriteByteSeq(tGuid[2]);
            data->WriteByteSeq(tGuid[4]);
        }

        *data << float(GetPositionX());
        *data << float(unit->getSpeedRate(TYPE_PITCH_RATE, true));

        data->WriteByteSeq(Guid[3]);
        data->WriteByteSeq(Guid[0]);

        *data << float(unit->getSpeedRate(TYPE_SWIM, true));
        *data << float(unit->GetPositionY());

        data->WriteByteSeq(Guid[7]);
        data->WriteByteSeq(Guid[1]);
        data->WriteByteSeq(Guid[2]);

        *data << float(unit->getSpeedRate(TYPE_WALK, true));

        *data << uint32_t(Util::getMSTime());

        *data << float(unit->getSpeedRate(TYPE_TURN_RATE, true));

        data->WriteByteSeq(Guid[6]);

        *data << float(unit->getSpeedRate(TYPE_FLY, true));

        if (!G3D::fuzzyEq(GetOrientation(), 0.0f))
            *data << float(normalizeOrientation(GetOrientation()));

        *data << unit->getSpeedRate(TYPE_RUN, true);

        if (hasPitch)
            *data << float(obj_movement_info.pitch_rate);

        *data << float(unit->getSpeedRate(TYPE_FLY_BACK, true));
    }

    if (updateFlags & UPDATEFLAG_VEHICLE)
    {
        uint32_t vehicleid = 0;

        if (isCreature())
        {
            vehicleid = static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid;
        }
        else
        {
            if (isPlayer())
                vehicleid = static_cast<Player*>(this)->mountvehicleid;
        }

        *data << float(normalizeOrientation(GetOrientation()));
        *data << uint32_t(vehicleid);
    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid;

        data->WriteByteSeq(transGuid[0]);
        data->WriteByteSeq(transGuid[5]);

        if (hasVehicleId)
            *data << uint32_t(static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid);

        data->WriteByteSeq(transGuid[3]);

        *data << float(GetTransOffsetX());

        data->WriteByteSeq(transGuid[4]);
        data->WriteByteSeq(transGuid[6]);
        data->WriteByteSeq(transGuid[1]);

        *data << uint32_t(obj_movement_info.transport_time);
        *data << float(GetTransOffsetY());

        data->WriteByteSeq(transGuid[2]);
        data->WriteByteSeq(transGuid[7]);

        *data << float(GetTransOffsetZ());
        *data << int8_t(obj_movement_info.transport_seat);
        *data << float(GetTransOffsetO());

        if (hasTransportTime2)
            *data << uint32_t(obj_movement_info.transport_time2);
    }

    if (updateFlags & UPDATEFLAG_ROTATION)
    {
        if (isGameObject())
            *data << int64_t(static_cast<GameObject*>(this)->GetRotation());
    }

    if (updateFlags & UPDATEFLAG_AREATRIGGER)
    {
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << uint8_t(0);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
        *data << float(0.0f);
    }

    if (updateFlags & UPDATEFLAG_HAS_POSITION)
    {
        *data << float(normalizeOrientation(GetOrientation()));
        *data << float(GetPositionX());
        *data << float(GetPositionY());
        *data << float(GetPositionZ());
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->WriteByteSeq(victimGuid[4]);
            data->WriteByteSeq(victimGuid[0]);
            data->WriteByteSeq(victimGuid[3]);
            data->WriteByteSeq(victimGuid[5]);
            data->WriteByteSeq(victimGuid[7]);
            data->WriteByteSeq(victimGuid[6]);
            data->WriteByteSeq(victimGuid[2]);
            data->WriteByteSeq(victimGuid[1]);
        }
        else
        {
            for (uint8_t i = 0; i < 8; ++i)
            {
                *data << uint8_t(0);
            }
        }
    }

    if (updateFlags & UPDATEFLAG_ANIM_KITS)
    {
        if (hasAIAnimKit)
            *data << uint16_t(0);
        if (hasMovementAnimKit)
            *data << uint16_t(0);
        if (hasMeleeAnimKit)
            *data << uint16_t(0);
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)
    {
        GameObject const* go = static_cast<GameObject*>(this);
        if (go && go->ToTransport())
            *data << uint32_t(go->GetTransValues()->PathProgress);
        else
            *data << uint32_t(Util::getMSTime());
    }
}
#endif

#if VERSION_STRING == Mop
void Object::buildMovementUpdate(ByteBuffer* data, uint16_t updateFlags, Player* /*target*/)
{
    ObjectGuid Guid = getGuid();

    data->writeBit(false);
    data->writeBit(false);                                      // updateFlags & UPDATEFLAG_ANIM_KITS
    data->writeBit(updateFlags & UPDATEFLAG_LIVING);
    data->writeBit(false);
    data->writeBit(false);
    data->writeBits(0, 22);
    data->writeBit(false);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_TRANSPORT);
    data->writeBit(updateFlags & UPDATEFLAG_ROTATION);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_SELF);
    data->writeBit(updateFlags & UPDATEFLAG_HAS_TARGET);
    data->writeBit(false);
    data->writeBit(false);
    data->writeBit(false);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_POSITION);
    data->writeBit(false);
    data->writeBit(updateFlags & UPDATEFLAG_HAS_POSITION);

    bool hasTransport = false;
    bool isSplineEnabled = false;
    bool hasPitch = false;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasElevation = false;
    bool hasOrientation = !IsType(TYPE_ITEM);
    bool hasTimeStamp = true;
    bool hasTransportTime2 = false;
    bool hasTransportTime3 = false;

    if (IsType(TYPE_UNIT))
    {
        Unit* unit = (Unit*)this;
        hasTransport = obj_movement_info.transport_guid != 0;
        isSplineEnabled = false; // unit->IsSplineEnabled();

        if (getObjectTypeId() == TYPEID_PLAYER)
        {
            hasPitch = obj_movement_info.getMovementStatusInfo().hasPitch;
            hasFallData = obj_movement_info.getMovementStatusInfo().hasFallData;
            hasFallDirection = obj_movement_info.getMovementStatusInfo().hasFallDirection;
            hasElevation = obj_movement_info.getMovementStatusInfo().hasSplineElevation;
            hasTransportTime2 = obj_movement_info.getMovementStatusInfo().hasTransportTime2;
            hasTransportTime3 = obj_movement_info.getMovementStatusInfo().hasTransportTime3;
        }
        else
        {
            hasPitch = obj_movement_info.hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) ||
                obj_movement_info.hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING);
            hasFallData = obj_movement_info.hasMovementFlag2(MOVEFLAG2_INTERPOLATED_TURN);
            hasFallDirection = obj_movement_info.hasMovementFlag(MOVEFLAG_FALLING);
            hasElevation = obj_movement_info.hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION);
        }
    }

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;

        data->writeBit(Guid[2]);
        data->writeBit(false);
        data->writeBit(!hasPitch);
        data->writeBit(hasTransport);
        data->writeBit(false);

        if (hasTransport)
        {
            ObjectGuid tGuid = obj_movement_info.transport_guid;

            data->writeBit(tGuid[4]);
            data->writeBit(tGuid[2]);
            data->writeBit(hasTransportTime3);
            data->writeBit(tGuid[0]);
            data->writeBit(tGuid[1]);
            data->writeBit(tGuid[3]);
            data->writeBit(tGuid[6]);
            data->writeBit(tGuid[7]);
            data->writeBit(hasTransportTime2);
            data->writeBit(tGuid[5]);
        }

        data->writeBit(!hasTimeStamp);
        data->writeBit(Guid[6]);
        data->writeBit(Guid[4]);
        data->writeBit(Guid[3]);

        data->writeBit(G3D::fuzzyEq(GetOrientation(), 0.0f));

        data->writeBit(false);
        data->writeBit(Guid[5]);
        data->writeBits(0, 22);
        data->writeBit(!obj_movement_info.getMovementFlags());
        data->writeBits(0, 19);
        data->writeBit(hasFallData);

        if (obj_movement_info.getMovementFlags())
            data->writeBits(obj_movement_info.getMovementFlags(), 30);

        data->writeBit(!hasElevation);
        data->writeBit(isSplineEnabled);
        data->writeBit(false);
        data->writeBit(Guid[0]);
        data->writeBit(Guid[7]);
        data->writeBit(Guid[1]);

        if (isSplineEnabled)
        {
            //Movement::PacketBuilder::WriteCreateBits(*unit->movespline, *data);
        }

        data->writeBit(!obj_movement_info.getMovementFlags2());

        if (hasFallData)
            data->writeBit(hasFallDirection);

        if (obj_movement_info.getMovementFlags2())
            data->writeBits(obj_movement_info.getMovementFlags2(), 13);
    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid = obj_movement_info.transport_guid;

        data->writeBit(transGuid[4]);
        data->writeBit(transGuid[1]);
        data->writeBit(transGuid[0]);
        data->writeBit(hasTransportTime2);
        data->writeBit(transGuid[6]);
        data->writeBit(transGuid[5]);
        data->writeBit(transGuid[3]);
        data->writeBit(transGuid[2]);
        data->writeBit(transGuid[7]);
        data->writeBit(hasTransportTime3);
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->writeBit(victimGuid[4]);
            data->writeBit(victimGuid[6]);
            data->writeBit(victimGuid[5]);
            data->writeBit(victimGuid[2]);
            data->writeBit(victimGuid[0]);
            data->writeBit(victimGuid[1]);
            data->writeBit(victimGuid[3]);
            data->writeBit(victimGuid[7]);
        }
        else
            data->writeBits(0, 8);
    }

    data->flushBits();

    if (updateFlags & UPDATEFLAG_LIVING)
    {
        Unit* unit = (Unit*)this;
        ;
        if (hasTransport)
        {
            ObjectGuid tGuid = obj_movement_info.transport_guid;

            data->WriteByteSeq(tGuid[7]);
            *data << float(GetTransOffsetX());

            if (hasTransportTime3)
                *data << uint32_t(obj_movement_info.fall_time);

            *data << float(GetTransOffsetO());
            *data << float(GetTransOffsetY());
            data->WriteByteSeq(tGuid[4]);
            data->WriteByteSeq(tGuid[1]);
            data->WriteByteSeq(tGuid[3]);
            *data << float(GetTransOffsetZ());
            data->WriteByteSeq(tGuid[5]);

            if (hasTransportTime2)
                *data << uint32_t(obj_movement_info.transport_time2);

            data->WriteByteSeq(tGuid[0]);
            *data << int8_t(obj_movement_info.transport_seat);
            data->WriteByteSeq(tGuid[6]);
            data->WriteByteSeq(tGuid[2]);
            *data << uint32_t(obj_movement_info.transport_time);
        }

        data->WriteByteSeq(Guid[4]);

        if (isSplineEnabled)
        {
            //Movement::PacketBuilder::WriteCreateBytes(*unit->movespline, *data);
        }

        *data << float(unit->getSpeedRate(TYPE_FLY, true));

        //todo movementcounter

        data->WriteByteSeq(Guid[2]);

        if (hasFallData)
        {
            if (hasFallDirection)
            {
                *data << float(obj_movement_info.jump_info.xyspeed);
                *data << float(obj_movement_info.jump_info.cosAngle);
                *data << float(obj_movement_info.jump_info.sinAngle);
            }

            *data << uint32_t(obj_movement_info.fall_time);
            *data << float(obj_movement_info.jump_info.velocity);
        }

        data->WriteByteSeq(Guid[1]);
        *data << float(unit->getSpeedRate(TYPE_TURN_RATE, true));

        if (obj_movement_info.update_time)
            *data << uint32_t(obj_movement_info.update_time);

        *data << unit->getSpeedRate(TYPE_RUN_BACK, true);

        if (hasElevation)
            *data << float(obj_movement_info.spline_elevation);

        data->WriteByteSeq(Guid[7]);
        *data << float(unit->getSpeedRate(TYPE_PITCH_RATE, true));
        *data << float(GetPositionX());

        if (hasPitch)
            *data << float(obj_movement_info.pitch_rate);

        if (!G3D::fuzzyEq(GetOrientation(), 0.0f))
            *data << float(normalizeOrientation(GetOrientation()));

        *data << float(unit->getSpeedRate(TYPE_WALK, true));
        *data << float(GetPositionY());
        *data << float(unit->getSpeedRate(TYPE_FLY_BACK, true));
        data->WriteByteSeq(Guid[3]);
        data->WriteByteSeq(Guid[5]);
        data->WriteByteSeq(Guid[6]);
        data->WriteByteSeq(Guid[0]);
        *data << unit->getSpeedRate(TYPE_SWIM_BACK, true);
        *data << float(unit->getSpeedRate(TYPE_RUN, true));
        *data << float(unit->getSpeedRate(TYPE_SWIM, true));
        *data << float(GetPositionZ());
    }

    if (updateFlags & UPDATEFLAG_POSITION)
    {
        ObjectGuid transGuid = getGuid();

        if (obj_movement_info.transport_time2 && obj_movement_info.transport_guid)
            *data << obj_movement_info.transport_time2;

        *data << float(GetTransOffsetY());
        *data << int8_t(GetTransSeat());
        *data << float(GetTransOffsetX());
        data->writeBit(transGuid[2]);
        data->writeBit(transGuid[4]);
        data->writeBit(transGuid[1]);

        /*if (obj->obj_movement_info.transport_time3 && obj->obj_movement_info.transport_guid)
            *data << uint32(obj->obj_movement_info.transport_time3);*/

        *data << uint32_t(GetTransTime());

        *data << float(GetTransOffsetO());
        *data << float(GetTransOffsetZ());

        data->writeBit(transGuid[6]);
        data->writeBit(transGuid[0]);
        data->writeBit(transGuid[5]);
        data->writeBit(transGuid[3]);
        data->writeBit(transGuid[7]);
    }

    if (updateFlags & UPDATEFLAG_HAS_TARGET)
    {
        if (isCreatureOrPlayer())
        {
            ObjectGuid victimGuid = static_cast<Unit*>(this)->getTargetGuid();

            data->WriteByteSeq(victimGuid[7]);
            data->WriteByteSeq(victimGuid[1]);
            data->WriteByteSeq(victimGuid[5]);
            data->WriteByteSeq(victimGuid[2]);
            data->WriteByteSeq(victimGuid[6]);
            data->WriteByteSeq(victimGuid[3]);
            data->WriteByteSeq(victimGuid[0]);
            data->WriteByteSeq(victimGuid[4]);
        }
        else
        {
            for (uint8_t i = 0; i < 8; ++i)
            {
                *data << uint8_t(0);
            }
        }
    }

    if (updateFlags & UPDATEFLAG_VEHICLE)
    {
        uint32_t vehicleid = 0;

        if (isCreature())
        {
            vehicleid = static_cast<Creature*>(this)->GetCreatureProperties()->vehicleid;
        }
        else
        {
            if (isPlayer())
                vehicleid = static_cast<Player*>(this)->mountvehicleid;
        }

        *data << uint32_t(vehicleid);
        *data << float(GetOrientation());
    }

    if (updateFlags & UPDATEFLAG_HAS_POSITION)
    {
        *data << float(GetPositionY());
        *data << float(GetPositionZ());
        *data << float(normalizeOrientation(GetOrientation()));
        *data << float(GetPositionX());
    }

    if (updateFlags & UPDATEFLAG_TRANSPORT)
    {
        if (Transporter* trans = static_cast<Transporter*>(this))
            *data << uint32_t(trans->getAnimationProgress());   //pathProgress
        else
            *data << uint32_t(Util::getMSTime());
    }

    if (updateFlags & UPDATEFLAG_ROTATION)
    {
        if (isGameObject())
            *data << uint64_t(static_cast<GameObject*>(this)->GetRotation());
    }
}
#endif

void Object::buildValuesUpdate(ByteBuffer* data, UpdateMask* updateMask, Player* target)
{
    if (!updateMask)
    {
        sLogger.failure("Object::buildValuesUpdate invalid updateMask (nullptr)");
        return;
    }

    if (updateMask->GetCount() != m_valuesCount)
    {
        sLogger.failure("Object::buildValuesUpdate values count in update mask is not equal to object values count!");
        return;
    }

    auto activate_quest_object = false, reset = false;
    uint32_t old_flags = 0;

    // Create a new object
    if (updateMask->GetBit(getOffsetForStructuredField(WoWObject, guid)) && target)
    {
        if (isCreature() || isCorpse())
        {
            auto this_creature = static_cast<Creature*>(this);
            if (this_creature->isTagged() && !this_creature->loot.isLooted())
            {
                uint32_t current_flags;
                old_flags = current_flags = this_creature->getDynamicFlags();
                if (this_creature->getTaggerGuid() == target->getGuid())
                {
                    old_flags = U_DYN_FLAG_TAGGED_BY_OTHER;
                    if (current_flags & U_DYN_FLAG_TAGGED_BY_OTHER)
                        current_flags &= ~old_flags;

                    if (!(current_flags & U_DYN_FLAG_LOOTABLE) && this_creature->HasLootForPlayer(target))
                        current_flags |= U_DYN_FLAG_LOOTABLE;
                }
                else if (!this_creature->loot.isLooted())
                {
                    old_flags = U_DYN_FLAG_LOOTABLE;

                    if (!(current_flags & U_DYN_FLAG_TAGGED_BY_OTHER))
                        current_flags |= U_DYN_FLAG_TAGGED_BY_OTHER;

                    if (current_flags & U_DYN_FLAG_LOOTABLE)
                        current_flags &= ~old_flags;
                }

                this_creature->setDynamicFlags(current_flags);
                reset = true;
            }
        }

        if (isGameObject())
        {
            const auto this_go = static_cast<GameObject*>(this);
            if (this_go->isQuestGiver())
            {
                auto this_qg = static_cast<GameObject_QuestGiver*>(this);
                if (this_qg->HasQuests())
                {
                    for (auto quest_relation : this_qg->getQuestList())
                    {
                        if (!quest_relation)
                            continue;

                        if (const auto quest_props = quest_relation->qst)
                        {
                            activate_quest_object = (quest_relation->type & QUESTGIVER_QUEST_START && !target->hasQuestInQuestLog(quest_props->id))
                                || (quest_relation->type & QUESTGIVER_QUEST_END && !target->hasQuestInQuestLog(quest_props->id));
                        }
                    }
                }
                else
                {
                    if (const auto go_props = this_go->GetGameObjectProperties())
                    {
                        if (go_props->goMap.size() > 0 || go_props->itemMap.size() > 0)
                        {
                            for (const auto quest_go : go_props->goMap)
                            {
                                if (auto* const questLog = target->getQuestLogByQuestId(quest_go.first->id))
                                {
                                    const auto quest = questLog->getQuestProperties();
                                    if (quest->count_required_mob == 0)
                                        continue;

                                    for (uint8_t i = 0; i < 4; ++i)
                                    {
                                        if (quest->required_mob_or_go[i] == static_cast<int32_t>(this_go->getEntry()))
                                        {
                                            if (questLog->getMobCountByIndex(i) < quest->required_mob_or_go_count[i])
                                            {
                                                activate_quest_object = true;
                                                break;
                                            }
                                        }
                                    }
                                }

                                if (activate_quest_object)
                                    break;
                            }

                            if (!activate_quest_object)
                            {
                                for (auto quest_props : go_props->itemMap)
                                {
                                    for (const auto item_pair : quest_props.second)
                                    {
                                        if (auto* const questLog = target->getQuestLogByQuestId(quest_props.first->id))
                                        {
                                            if (target->getItemInterface()->GetItemCount(item_pair.first) < item_pair.second)
                                            {
                                                activate_quest_object = true;
                                                break;
                                            }
                                        }
                                    }

                                    if (activate_quest_object)
                                        break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // We already checked if GameObject, but do it again in case logic path changes in future
    if (activate_quest_object && isGameObject())
    {
        const auto this_go = static_cast<GameObject*>(this);
        old_flags = this_go->getDynamic();
        // Show sparkles
        this_go->setDynamic(1 | 8);
        reset = true;
    }

    uint32_t block_count, values_count;
    if (m_valuesCount > 2 * 0x20)
    {
        block_count = updateMask->GetUpdateBlockCount();
        values_count = std::min<uint32_t>(block_count * 0x20, m_valuesCount);
    }
    else
    {
        block_count = updateMask->GetBlockCount();
        values_count = m_valuesCount;
    }

    *data << uint8_t(block_count);
    data->append(updateMask->GetMask(), block_count * 4);
    for (uint32_t idx = 0; idx < values_count; ++idx)
    {
        if (updateMask->GetBit(idx))
            *data << m_uint32Values[idx];
    }

    if (reset)
    {
        skipping_updates = true;
        if (isCreatureOrPlayer())
            static_cast<Unit*>(this)->setDynamicFlags(old_flags);
        else if (isGameObject())
            static_cast<GameObject*>(this)->setDynamic(old_flags);
        skipping_updates = false;
    }
}
// MIT End

bool Object::SetPosition(const LocationVector & v, [[maybe_unused]]bool allowPorting /* = false */)
{
    bool updateMap = false, result = true;

    if (m_position.x != v.x || m_position.y != v.y)
        updateMap = true;

    m_position = v;

#if VERSION_STRING < Cata
    if (!allowPorting && v.z < -500)
    {
        m_position.z = 500;
        sLogger.failure("setPosition: fell through map; height ported");

        result = false;
    }
#endif

    if (IsInWorld() && updateMap)
    {
        m_mapMgr->ChangeObjectLocation(this);
    }

    return result;
}

bool Object::SetPosition(float newX, float newY, float newZ, float newOrientation, [[maybe_unused]]bool allowPorting)
{
    bool updateMap = false, result = true;

    if (!std::isnan(newX) && !std::isnan(newY) && !std::isnan(newOrientation))
    {
        //It's a good idea to push through EVERY transport position change, no matter how small they are. By: VLack aka. VLsoft
        if (isGameObject() && static_cast<GameObject*>(this)->GetGameObjectProperties()->type == GAMEOBJECT_TYPE_MO_TRANSPORT)
            updateMap = true;

        //if (m_position.x != newX || m_position.y != newY)
        //updateMap = true;
        if (m_lastMapUpdatePosition.Distance2DSq({ newX, newY }) > 4.0f) /* 2.0f */
            updateMap = true;

        m_position.ChangeCoords({ newX, newY, newZ, newOrientation });

#if VERSION_STRING < Cata
        if (!allowPorting && newZ < -500)
        {
            m_position.z = 500;
            sLogger.failure("setPosition: fell through map; height ported");

            result = false;
        }
#endif

        if (IsInWorld() && updateMap)
        {
            m_lastMapUpdatePosition.ChangeCoords({ newX, newY, newZ, newOrientation });
            m_mapMgr->ChangeObjectLocation(this);

            if (isPlayer() && static_cast<Player*>(this)->getGroup() && static_cast<Player*>(this)->m_last_group_position.Distance2DSq(m_position) > 25.0f)       // distance of 5.0
            {
                static_cast<Player*>(this)->AddGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);
            }
        }

        if (isCreatureOrPlayer())
        {
            Unit* u = static_cast<Unit*>(this);
            if (u->getVehicleComponent() != nullptr)
                u->getVehicleComponent()->MovePassengers(newX, newY, newZ, newOrientation);
        }

        return result;
    }

    sLogger.failure("Object::SetPosition one of the position values in NaN, returning false!");
    return false;
}

void Object::_SetUpdateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    *updateMask = m_updateMask;
}

void Object::_SetCreateBits(UpdateMask* updateMask, Player* /*target*/) const
{
    for (uint32 i = 0; i < m_valuesCount; ++i)
        if (m_uint32Values[i] != 0)
            updateMask->SetBit(i);
}

void Object::AddToWorld()
{
    MapMgr* mapMgr = sInstanceMgr.GetInstance(this);
    if (mapMgr == nullptr)
    {
        sLogger.failure("AddToWorld() failed for Object with GUID " I64FMT " MapId %u InstanceId %u", getGuid(), GetMapId(), GetInstanceID());
        return;
    }

    if (isPlayer())
    {
        Player* plr = static_cast<Player*>(this);
        if (mapMgr->pInstance != nullptr && !plr->isGMFlagSet())
        {
            // Player limit?
            if (mapMgr->GetMapInfo()->playerlimit && mapMgr->GetPlayerCount() >= mapMgr->GetMapInfo()->playerlimit)
                return;
            Group* group = plr->getGroup();
            // Player in group?
            if (group == nullptr && mapMgr->pInstance->m_creatorGuid == 0)
                return;
            // If set: Owns player the instance?
            if (mapMgr->pInstance->m_creatorGuid != 0 && mapMgr->pInstance->m_creatorGuid != plr->getGuidLow())
                return;

            if (group != nullptr)
            {
                // Is instance empty or owns our group the instance?
                if (mapMgr->pInstance->m_creatorGroup != 0 && mapMgr->pInstance->m_creatorGroup != group->GetID())
                {
                    // Player not in group or another group is already playing this instance.
                    sChatHandler.SystemMessage(plr->GetSession(), "Another group is already inside this instance of the dungeon.");
                    if (plr->GetSession()->GetPermissionCount() > 0)
                        sChatHandler.BlueSystemMessage(plr->GetSession(), "Enable your GameMaster flag to ignore this rule.");
                    return;
                }
                else if (mapMgr->pInstance->m_creatorGroup == 0)
                    // Players group now "owns" the instance.
                    mapMgr->pInstance->m_creatorGroup = group->GetID();
            }
        }
    }

    m_mapMgr = mapMgr;
    m_inQueue = true;

    // correct incorrect instance id's
    m_instanceId = m_mapMgr->GetInstanceID();
    m_mapId = m_mapMgr->GetMapId();
    mapMgr->AddObject(this);
}

void Object::AddToWorld(MapMgr* pMapMgr)
{
    if (!pMapMgr || (pMapMgr->GetMapInfo()->playerlimit && this->isPlayer() && pMapMgr->GetPlayerCount() >= pMapMgr->GetMapInfo()->playerlimit))
        return; //instance add failed

    m_mapMgr = pMapMgr;
    m_inQueue = true;

    pMapMgr->AddObject(this);

    // correct incorrect instance id's
    m_instanceId = pMapMgr->GetInstanceID();
    m_mapId = m_mapMgr->GetMapId();
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Unlike addtoworld it pushes it directly ignoring add pool this can
/// only be called from the thread of mapmgr!
//////////////////////////////////////////////////////////////////////////////////////////
void Object::PushToWorld(MapMgr* mgr)
{
    if (t_currentMapContext.get() == mgr)
    {
        if (mgr == nullptr)
        {
            sLogger.failure("Invalid push to world of Object " I64FMT " ", getGuid());
            return; // instance add failed
        }

        m_mapId = mgr->GetMapId();
        //there's no need to set the InstanceId before calling PushToWorld() because it's already set here.
        m_instanceId = mgr->GetInstanceID();

        m_mapMgr = mgr;
        OnPrePushToWorld();

        mgr->PushObject(this);

        // correct incorrect instance id's
        m_inQueue = false;

        event_Relocate();

        // call virtual function to handle stuff.. :P
        OnPushToWorld();
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Remove object from world
//////////////////////////////////////////////////////////////////////////////////////////
void Object::RemoveFromWorld(bool free_guid)
{
    if (m_mapMgr != nullptr)
    {
        OnPreRemoveFromWorld();

        MapMgr* m = m_mapMgr;
        m_mapMgr = nullptr;

        m->RemoveObject(this, free_guid);

        OnRemoveFromWorld();

        //shouldnt need to clear, spell destructor will erase
        //m_pendingSpells.clear();

        m_instanceId = INSTANCEID_NOT_IN_WORLD;
        m_mapId = MAPID_NOT_IN_WORLD;
        //m_inQueue is set to true when AddToWorld() is called. AddToWorld() queues the Object to be pushed, but if it's not pushed and RemoveFromWorld()
        //is called, m_inQueue will still be true even if the Object is no more inworld, nor queued.
        m_inQueue = false;

        // update our event holder
        event_Relocate();
    }
    else
    {
        sLogger.failure("Object::RemoveFromWorld tried to remove object without a valid mapMgr (nullptr)");
    }
}

float Object::CalcDistance(Object* Ob)
{
    if (Ob != nullptr)
        return CalcDistance(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ(), Ob->GetPositionX(), Ob->GetPositionY(), Ob->GetPositionZ());

    return 0xFFFF;
}

float Object::CalcDistance(float ObX, float ObY, float ObZ)
{
    return CalcDistance(this->GetPositionX(), this->GetPositionY(), this->GetPositionZ(), ObX, ObY, ObZ);
}

float Object::CalcDistance(Object* Oa, Object* Ob)
{
    if (Oa != nullptr && Ob != nullptr)
        return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Oa->GetPositionZ(), Ob->GetPositionX(), Ob->GetPositionY(), Ob->GetPositionZ());

    return 0xFFFF;
}

float Object::CalcDistance(Object* Oa, float ObX, float ObY, float ObZ)
{
    if (Oa != nullptr)
        return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Oa->GetPositionZ(), ObX, ObY, ObZ);

    return 0xFFFF;
}

float Object::CalcDistance(float OaX, float OaY, float OaZ, float ObX, float ObY, float ObZ)
{
    float xdest = OaX - ObX;
    float ydest = OaY - ObY;
    float zdest = OaZ - ObZ;
    return sqrtf(zdest * zdest + ydest * ydest + xdest * xdest);
}

bool Object::IsWithinDistInMap(Object* obj, const float dist2compare) const
{
    if (obj != nullptr)
    {
        float xdest = this->GetPositionX() - obj->GetPositionX();
        float ydest = this->GetPositionY() - obj->GetPositionY();
        float zdest = this->GetPositionZ() - obj->GetPositionZ();
        return sqrtf(zdest * zdest + ydest * ydest + xdest * xdest) <= dist2compare;
    }
    return false;
}

bool Object::IsWithinLOSInMap(Object* obj)
{
    if (obj == nullptr)
        return false;

    if (!IsInMap(obj))
        return false;

    LocationVector location = obj->GetPosition();
    return IsWithinLOS(location);
}

bool Object::IsWithinLOS(LocationVector location)
{
    LocationVector location2;
    location2 = GetPosition();

    if (worldConfig.terrainCollision.isCollisionEnabled)
    {
        VMAP::IVMapManager* mgr = VMAP::VMapFactory::createOrGetVMapManager();
        return mgr->isInLineOfSight(GetMapId(), location2.x, location2.y, location2.z + 2.0f, location.x, location.y, location.z + 2.0f);
    }
    else
    {
        return true;
    }
}

float Object::calcAngle(float Position1X, float Position1Y, float Position2X, float Position2Y)
{
    float dx = Position2X - Position1X;
    float dy = Position2Y - Position1Y;
    double angle = 0.0f;

    // Calculate angle
    if (dx == 0.0)
    {
        if (dy == 0.0)
            angle = 0.0;
        else if (dy > 0.0)
            angle = M_PI * 0.5 /* / 2 */;
        else
            angle = M_PI * 3.0 * 0.5/* / 2 */;
    }
    else if (dy == 0.0)
    {
        if (dx > 0.0)
            angle = 0.0;
        else
            angle = M_PI;
    }
    else
    {
        if (dx < 0.0)
            angle = atanf(dy / dx) + M_PI;
        else if (dy < 0.0)
            angle = atanf(dy / dx) + (2 * M_PI);
        else
            angle = atanf(dy / dx);
    }

    // Convert to degrees
    angle = angle * float(180 / M_PI);

    // Return
    return float(angle);
}

float Object::calcRadAngle(float Position1X, float Position1Y, float Position2X, float Position2Y)
{
    double dx = double(Position2X - Position1X);
    double dy = double(Position2Y - Position1Y);
    double angle = 0.0;

    // Calculate angle
    if (dx == 0.0)
    {
        if (dy == 0.0)
            angle = 0.0;
        else if (dy > 0.0)
            angle = M_PI * 0.5/*/ 2.0*/;
        else
            angle = M_PI * 3.0 * 0.5/*/ 2.0*/;
    }
    else if (dy == 0.0)
    {
        if (dx > 0.0)
            angle = 0.0;
        else
            angle = M_PI;
    }
    else
    {
        if (dx < 0.0)
            angle = atan(dy / dx) + M_PI;
        else if (dy < 0.0)
            angle = atan(dy / dx) + (2 * M_PI);
        else
            angle = atan(dy / dx);
    }

    // Return
    return float(angle);
}

float Object::getEasyAngle(float angle)
{
    while (angle < 0)
    {
        angle = angle + 360;
    }
    while (angle >= 360)
    {
        angle = angle - 360;
    }
    return angle;
}

bool Object::inArc(float Position1X, float Position1Y, float FOV, float Orientation, float Position2X, float Position2Y)
{
    float angle = calcAngle(Position1X, Position1Y, Position2X, Position2Y);
    float lborder = getEasyAngle((Orientation - (FOV * 0.5f/*/2*/)));
    float rborder = getEasyAngle((Orientation + (FOV * 0.5f/*/2*/)));
    //sLogger.debug("Orientation: %f Angle: %f LeftBorder: %f RightBorder %f",Orientation,angle,lborder,rborder);
    if (((angle >= lborder) && (angle <= rborder)) || ((lborder > rborder) && ((angle < rborder) || (angle > lborder))))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Object::isInFront(Object* target)
{
    // check if we facing something (is the object within a 180 degree slice of our positive y axis)

    double x = target->GetPositionX() - m_position.x;
    double y = target->GetPositionY() - m_position.y;

    double angle = atan2(y, x);
    angle = (angle >= 0.0) ? angle : 2.0 * M_PI + angle;
    angle -= m_position.o;

    while (angle > M_PI)
        angle -= 2.0 * M_PI;

    while (angle < -M_PI)
        angle += 2.0 * M_PI;

    // replace M_PI in the two lines below to reduce or increase angle

    double left = -1.0 * (M_PI / 2.0);
    double right = (M_PI / 2.0);

    return((angle >= left) && (angle <= right));
}

bool Object::isInBack(Object* target)
{
    // check if we are behind something (is the object within a 180 degree slice of our negative y axis)

    double x = m_position.x - target->GetPositionX();
    double y = m_position.y - target->GetPositionY();

    double angle = atan2(y, x);
    angle = (angle >= 0.0) ? angle : 2.0 * M_PI + angle;

    // if we are a creature and have a UNIT_FIELD_TARGET then we are always facing them
    if (isCreature() && static_cast<Creature*>(this)->getTargetGuid() != 0)
    {
        Unit* pTarget = static_cast<Creature*>(this)->getAIInterface()->getCurrentTarget();
        if (pTarget != nullptr)
            angle -= double(Object::calcRadAngle(target->m_position.x, target->m_position.y, pTarget->m_position.x, pTarget->m_position.y));
        else
            angle -= target->GetOrientation();
    }
    else
        angle -= target->GetOrientation();

    while (angle > M_PI)
        angle -= 2.0 * M_PI;

    while (angle < -M_PI)
        angle += 2.0 * M_PI;

    // replace M_H_PI in the two lines below to reduce or increase angle

    double left = -1.0 * (M_H_PI / 2.0);
    double right = (M_H_PI / 2.0);

    return((angle <= left) && (angle >= right));
}

bool Object::isInArc(Object* target, float angle) // angle in degrees
{
    return inArc(GetPositionX(), GetPositionY(), angle, GetOrientation(), target->GetPositionX(), target->GetPositionY());
}

bool Object::HasInArc(float degrees, Object* target)
{
    return isInArc(target, degrees);
}

bool Object::isInRange(Object* target, float range)
{
    if (!this->IsInWorld() || !target) return false;
    float dist = CalcDistance(target);
    return(dist <= range);
}

void Object::setServersideFaction()
{
    DBC::Structures::FactionTemplateEntry const* faction_template = nullptr;

    if (isCreatureOrPlayer())
    {
        faction_template = sFactionTemplateStore.LookupEntry(static_cast<Unit*>(this)->getFactionTemplate());
        if (faction_template == nullptr)
            sLogger.failure("Unit does not have a valid faction. Faction: %u set to Entry: %u", static_cast<Unit*>(this)->getFactionTemplate(), getEntry());
    }
    else if (isGameObject())
    {
        uint32 go_faction_id = static_cast<GameObject*>(this)->getFactionTemplate();
        faction_template = sFactionTemplateStore.LookupEntry(go_faction_id);
        if (go_faction_id != 0)         // faction = 0 means it has no faction.
        {
            if (faction_template == nullptr)
            {
                sLogger.failure("GameObject does not have a valid faction. Faction: %u set to Entry: %u", static_cast<GameObject*>(this)->getFactionTemplate(), getEntry());
            }
        }
    }

    //this solution looks a bit off, but our db is not perfect and this prevents some crashes.
    m_factionTemplate = faction_template;
    if (m_factionTemplate == nullptr)
    {
        m_factionTemplate = sFactionTemplateStore.LookupEntry(0);
        m_factionEntry = sFactionStore.LookupEntry(0);
    }
    else
    {
        m_factionEntry = sFactionStore.LookupEntry(m_factionTemplate->Faction);
    }
}

uint32 Object::getServersideFaction()
{
    return m_factionTemplate->Faction;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// SpellLog packets just to keep the code cleaner and better to read
//////////////////////////////////////////////////////////////////////////////////////////
void Object::SendSpellLog(Object* Caster, Object* Target, uint32 Ability, uint8 SpellLogType)
{
    if (Caster == nullptr || Target == nullptr || Ability == 0)
        return;

    Caster->SendMessageToSet(SmsgSpellLogMiss(Ability, Caster->getGuid(), Target->getGuid(), SpellLogType).serialise().get(), true);
}

int32 Object::event_GetInstanceID()
{
    // \return -1 for non-inworld.. so we get our shit moved to the right thread
    // \return default value is -1, if it's something else then we are/will be soon InWorld.
    return m_instanceId;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Object has an active state
//////////////////////////////////////////////////////////////////////////////////////////
bool Object::CanActivate()
{
    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
    {
        if (!isPet())
            return true;
    }
    break;

    case TYPEID_GAMEOBJECT:
    {
        if (static_cast<GameObject*>(this)->getGoType() != GAMEOBJECT_TYPE_TRAP)
            return true;
    }
    break;
    }

    return false;
}

void Object::Activate(MapMgr* mgr)
{
    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
        mgr->activeCreatures.insert(static_cast<Creature*>(this));
        break;

    case TYPEID_GAMEOBJECT:
        mgr->activeGameObjects.insert(static_cast<GameObject*>(this));
        break;
    }
    // Objects are active so set to true.
    Active = true;
}

void Object::Deactivate(MapMgr* mgr)
{
    if (mgr == nullptr)
        return;

    switch (m_objectTypeId)
    {
    case TYPEID_UNIT:
        // check iterator
        if (mgr->creature_iterator != mgr->activeCreatures.end() && (*mgr->creature_iterator)->getGuid() == getGuid())
            ++mgr->creature_iterator;
        mgr->activeCreatures.erase(static_cast<Creature*>(this));
        break;

    case TYPEID_GAMEOBJECT:
        mgr->activeGameObjects.erase(static_cast<GameObject*>(this));
        break;
    }
    Active = false;
}

void Object::SetZoneId(uint32 newZone)
{
    m_zoneId = newZone;

    if (isPlayer())
    {
        if (static_cast<Player*>(this)->getGroup())
            static_cast<Player*>(this)->AddGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);
    }
}

void Object::PlaySoundToSet(uint32 sound_entry)
{
    SendMessageToSet(SmsgPlaySound(sound_entry).serialise().get(), true);
}

bool Object::IsInBg()
{
    MySQLStructure::MapInfo const* pMapinfo = sMySQLStore.getWorldMapInfo(GetMapId());

    if (pMapinfo != nullptr)
    {
        return (pMapinfo->isBattleground());
    }

    return false;
}

uint32 Object::GetTeam()
{

    switch (m_factionEntry->ID)
    {
        // Human
    case 1:
        // Dwarf
    case 3:
        // Nightelf
    case 4:
        // Gnome
    case 8:
        // Draenei
    case 927:
        return TEAM_ALLIANCE;

        // Orc
    case 2:
        // Undead
    case 5:
        // Tauren
    case 6:
        // Troll
    case 9:
        // Bloodelf
    case 914:
        return TEAM_HORDE;
    }

    return static_cast<uint32>(-1);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Manipulates the phase value, see "enum PHASECOMMANDS" in
/// Object.h for a longer explanation!
//////////////////////////////////////////////////////////////////////////////////////////
void Object::Phase(uint8 command, uint32 newphase)
{
    switch (command)
    {
    case PHASE_SET:
        m_phase = newphase;
        break;
    case PHASE_ADD:
        m_phase |= newphase;
        break;
    case PHASE_DEL:
        m_phase &= ~newphase;
        break;
    case PHASE_RESET:
        m_phase = 1;
        break;
    default:
        sLogger.failure("Object::Phase called with invalid command %u", command);
        break;
    }
}

void Object::OutPacketToSet(uint16 Opcode, uint16 Len, const void* Data, bool /*self*/)
{
    if (!IsInWorld())
        return;

    // We are on Object level, which means we can't send it to ourselves so we only send to Players inrange
    for (const auto& itr : mInRangePlayersSet)
    {
        if (itr)
            itr->OutPacket(Opcode, Len, Data);
    }
}

void Object::SendMessageToSet(WorldPacket* data, bool /*bToSelf*/, bool /*myteam_only*/)
{
    if (!IsInWorld())
        return;

    uint32 myphase = GetPhase();
    for (const auto& itr : mInRangePlayersSet)
    {
        if (itr && (itr->GetPhase() & myphase) != 0)
            itr->SendPacket(data);
    }
}

void Object::SendCreatureChatMessageInRange(Creature* creature, uint32_t textId, Unit* target/* = nullptr*/)
{
    uint32 myphase = GetPhase();
    for (const auto& itr : mInRangePlayersSet)
    {
        Object* object = itr;
        if (object && (object->GetPhase() & myphase) != 0)
        {
            if (object->isPlayer())
            {
                Player* player = static_cast<Player*>(object);
                uint32_t sessionLanguage = player->GetSession()->language;

                std::string message;
                MySQLStructure::NpcScriptText const* npcScriptText = sMySQLStore.getNpcScriptText(textId);
                if (npcScriptText == nullptr)
                {
                    sLogger.failure("Invalid textId: %u. This text is send by a script but not in table npc_script_text!", textId);
                    return;
                }

                MySQLStructure::LocalesNpcScriptText const* lnpct = (sessionLanguage > 0) ? sMySQLStore.getLocalizedNpcScriptText(textId, sessionLanguage) : nullptr;
                if (lnpct != nullptr)
                    message = lnpct->text;
                else
                    message = npcScriptText->text;

                if (npcScriptText->emote != 0)
                    creature->eventAddEmote((EmoteType)npcScriptText->emote, npcScriptText->duration);

                if (npcScriptText->sound != 0)
                    creature->PlaySoundToSet(npcScriptText->sound);

                const auto data = creature->createChatPacket(npcScriptText->type, npcScriptText->language, message, target, sessionLanguage);
                player->SendPacket(data.get());
            }
        }
    }
}

Object* Object::GetMapMgrObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    return GetMapMgr()->_GetObject(guid);
}

Pet* Object::GetMapMgrPet(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetPet(wowGuid.getGuidLowPart());
}

Unit* Object::GetMapMgrUnit(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    return GetMapMgr()->GetUnit(guid);
}

Player* Object::GetMapMgrPlayer(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetPlayer(wowGuid.getGuidLowPart());
}

Creature* Object::GetMapMgrCreature(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetCreature(wowGuid.getGuidLowPart());
}

GameObject* Object::GetMapMgrGameObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetGameObject(wowGuid.getGuidLowPart());
}

DynamicObject* Object::GetMapMgrDynamicObject(const uint64 & guid)
{
    if (!IsInWorld())
        return nullptr;

    WoWGuid wowGuid;
    wowGuid.Init(guid);

    return GetMapMgr()->GetDynamicObject(wowGuid.getGuidLowPart());
}

MapCell* Object::GetMapCell() const
{
    if (m_mapMgr)
        return m_mapMgr->GetCell(m_mapCell_x, m_mapCell_y);
    return nullptr;
}

void Object::SetMapCell(MapCell* cell)
{
    if (cell == nullptr)
    {
        //mapcell coordinates are uint16, so using uint32(-1) will always make GetMapCell() return NULL.
        m_mapCell_x = m_mapCell_y = uint32(-1);
    }
    else
    {
        m_mapCell_x = cell->GetPositionX();
        m_mapCell_y = cell->GetPositionY();
    }
}

void Object::SendAIReaction(uint32 reaction)
{
    SendMessageToSet(SmsgAiReaction(getGuid(), reaction).serialise().get(), false);
}

void Object::SendDestroyObject()
{
    SendMessageToSet(AscEmu::Packets::SmsgDestroyObject(getGuid()).serialise().get(), false);
}

bool Object::GetPoint(float angle, float rad, float & outx, float & outy, float & outz, bool sloppypath)
{
    if (!IsInWorld())
        return false;
    outx = GetPositionX() + rad * cos(angle);
    outy = GetPositionY() + rad * sin(angle);
    outz = GetMapMgr()->GetLandHeight(outx, outy, GetPositionZ() + 2);
    float waterz;
    uint32 watertype;
    GetMapMgr()->GetLiquidInfo(outx, outy, GetPositionZ() + 2, waterz, watertype);
    outz = std::max(waterz, outz);

    MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
    dtNavMesh* nav = const_cast<dtNavMesh*>(mmap->GetNavMesh(GetMapId()));
    dtNavMeshQuery* nav_query = const_cast<dtNavMeshQuery*>(mmap->GetNavMeshQuery(GetMapId(), GetInstanceID()));
    //NavMeshData* nav = CollideInterface.GetNavMesh(GetMapId());

    if (nav != nullptr)
    {
        //if we can path there, go for it
        if (!isCreatureOrPlayer() || !sloppypath /*|| !static_cast<Unit*>(this)->getAIInterface()->CanCreatePath(outx, outy, outz)*/)
        {
            //raycast nav mesh to see if this place is valid
            float start[3] = { GetPositionY(), GetPositionZ() + 0.5f, GetPositionX() };
            float end[3] = { outy, outz + 0.5f, outx };
            float extents[3] = { 3, 5, 3 };
            dtQueryFilter filter;
            filter.setIncludeFlags(NAV_GROUND | NAV_WATER | NAV_MAGMA_SLIME);

            dtPolyRef startref;
            nav_query->findNearestPoly(start, extents, &filter, &startref, nullptr);

            float point;
            float hitNormal[3];
            float result[3];
            int numvisited;
            dtPolyRef visited[MAX_PATH_LENGTH];

            dtStatus rayresult = nav_query->raycast(startref, start, end, &filter, &point, hitNormal, visited, &numvisited, MAX_PATH_LENGTH);

            if (point <= 1.0f)
            {
                if (numvisited == 0 || rayresult == DT_FAILURE)
                {
                    result[0] = start[0];
                    result[1] = start[1];
                    result[2] = start[2];
                }
                else
                {
                    result[0] = start[0] + ((end[0] - start[0]) * point);
                    result[1] = start[1] + ((end[1] - start[1]) * point);
                    result[2] = start[2] + ((end[2] - start[2]) * point);
                    nav_query->getPolyHeight(visited[numvisited - 1], result, &result[1]);
                }

                //copy end back to function floats
                outy = result[0];
                outz = result[1];
                outx = result[2];
            }
        }
    }
    else //test against vmap if mmap isn't available
    {
        float testx, testy, testz;

        VMAP::IVMapManager* mgr = VMAP::VMapFactory::createOrGetVMapManager();
        bool isHittingObject = mgr->getObjectHitPos(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ() + 2, outx, outy, outz + 2, testx, testy, testz, -0.5f);

        if (isHittingObject)
        {
            //hit something
            outx = testx;
            outy = testy;
            outz = testz;
        }

        outz = GetMapMgr()->GetLandHeight(outx, outy, outz + 2);
    }

    return true;
}

void Object::getNearPoint2D(Object* searcher, float& x, float& y, float distance2d, float absAngle)
{
    float effectiveReach = getCombatReach();

    if (searcher)
    {
        effectiveReach += searcher->getCombatReach();

#if VERSION_STRING >= WotLK
        if (this != searcher)
        {
            float myHover = 0.0f, searcherHover = 0.0f;
            if (Unit const* unit = ToUnit())
                myHover = unit->getHoverHeight();
            if (Unit const* searchUnit = searcher->ToUnit())
                searcherHover = searchUnit->getHoverHeight();

            float hoverDelta = myHover - searcherHover;
            if (hoverDelta != 0.0f)
                effectiveReach = std::sqrt(std::max(effectiveReach * effectiveReach - hoverDelta * hoverDelta, 0.0f));
        }
#endif
    }

    x = GetPositionX() + (effectiveReach + distance2d) * std::cos(absAngle);
    y = GetPositionY() + (effectiveReach + distance2d) * std::sin(absAngle);
}

void Object::getNearPoint(Object* searcher, float& x, float& y, float& z, float distance2d, float absAngle)
{
    getNearPoint2D(searcher, x, y, distance2d, absAngle);
    z = GetPositionZ();
    z = GetMapMgr()->GetLandHeight(x, y, z);

    // if detection disabled, return first point
    if (!worldConfig.terrainCollision.isCollisionEnabled)
        return;

    // return if the point is already in LoS
    if (IsWithinLOS(LocationVector(x, y, z)))
        return;

    // remember first point
    float first_x = x;
    float first_y = y;
    float first_z = z;

    // loop in a circle to look for a point in LoS using small steps
    for (float angle = float(M_PI) / 8; angle < float(M_PI) * 2; angle += float(M_PI) / 8)
    {
        getNearPoint2D(searcher, x, y, distance2d, absAngle + angle);
        z = GetPositionZ();
        z = GetMapMgr()->GetLandHeight(x, y, z);
        if (IsWithinLOS(LocationVector(x,y,z)))
            return;
    }

    // still not in LoS, give up and return first position found
    x = first_x;
    y = first_y;
    z = first_z;

    normalizeMapCoord(x);
    normalizeMapCoord(y);
}

void Object::getClosePoint(float& x, float& y, float& z, float size, float distance2d /*= 0*/, float relAngle /*= 0*/)
{
    // angle calculated from current orientation
    getNearPoint(nullptr, x, y, z, distance2d + size, GetOrientation() + relAngle);
}

LocationVector Object::getHitSpherePointFor(LocationVector const& dest)
{
    G3D::Vector3 vThis(GetPositionX(), GetPositionY(), GetPositionZ() + 2.0f);
    G3D::Vector3 vObj(dest.getPositionX(), dest.getPositionY(), dest.getPositionZ());
    G3D::Vector3 contactPoint = vThis + (vObj - vThis).directionOrZero() * std::min(dest.getExactDist(GetPosition()), getCombatReach());

    return LocationVector(contactPoint.x, contactPoint.y, contactPoint.z, getAbsoluteAngle(contactPoint.x, contactPoint.y));
}

void Object::getHitSpherePointFor(LocationVector const& dest, float& x, float& y, float& z) const
{
    LocationVector pos = getHitSpherePointFor(dest);
    x = pos.getPositionX();
    y = pos.getPositionY();
    z = pos.getPositionZ();
}

LocationVector Object::getHitSpherePointFor(LocationVector const& dest) const
{
    G3D::Vector3 vThis(GetPositionX(), GetPositionY(), GetPositionZ() + 2.0f);
    G3D::Vector3 vObj(dest.getPositionX(), dest.getPositionY(), dest.getPositionZ());
    G3D::Vector3 contactPoint = vThis + (vObj - vThis).directionOrZero() * std::min(dest.getExactDist(GetPosition()), getCombatReach());

    return LocationVector(contactPoint.x, contactPoint.y, contactPoint.z, getAbsoluteAngle(contactPoint.x, contactPoint.y));
}

void Object::updateAllowedPositionZ(float x, float y, float &z, float* groundZ)
{
    // TODO: Allow transports to be part of dynamic vmap tree
    if (GetTransport())
    {
        if (groundZ)
            *groundZ = z + 2.0f; // dont clip inside our transport :)

        return;
    }

    if (Unit* unit = ToUnit())
    {
        if (!unit->canFly())
        {
            bool canSwim = unit->canSwim();
            float ground_z = z;
            float max_z;
            if (canSwim)
                max_z = getMapWaterOrGroundLevel(x, y, z, &ground_z);
            else
                max_z = ground_z = GetMapMgr()->GetLandHeight(x, y, z + 5.0f);

            if (max_z > INVALID_HEIGHT)
            {
#if VERSION_STRING >= WotLK
                // hovering units cannot go below their hover height
                float hoverOffset = unit->getHoverHeight();
                max_z += hoverOffset;
                ground_z += hoverOffset;
#endif

                if (z > max_z)
                    z = max_z;
                else if (z < ground_z)
                    z = ground_z;
            }

            if (groundZ)
                *groundZ = ground_z;
        }
        else
        {
            float ground_z = GetMapMgr()->GetLandHeight(x, y, z + 5.0f);
#if VERSION_STRING >= WotLK
            ground_z += unit->getHoverHeight();
#endif

            if (z < ground_z)
                z = ground_z;

            if (groundZ)
                *groundZ = ground_z;
        }
    }
    else
    {
        float ground_z = GetMapMgr()->GetLandHeight(x, y, z + 5.0f);
        if (ground_z > INVALID_HEIGHT)
            z = ground_z;

        if (groundZ)
            *groundZ = ground_z;
    }
}

LocationVector Object::getFirstCollisionPosition(float dist, float angle)
{
    LocationVector pos = GetPosition();
    movePositionToFirstCollision(pos, dist, angle);
    return pos;
}

void Object::movePositionToFirstCollision(LocationVector &pos, float dist, float angle)
{
    angle += GetOrientation();
    float destx, desty, destz;
    destx = pos.x + dist * std::cos(angle);
    desty = pos.y + dist * std::sin(angle);
    destz = pos.z;

    // Use a detour raycast to get our first collision point
    PathGenerator path(this);
    path.setUseRaycast(true);
    path.calculatePath(destx, desty, destz, false);

    // Check for valid path types before we proceed
    if (!(path.getPathType() & PATHFIND_NOT_USING_PATH))
        if (path.getPathType() & ~(PATHFIND_NORMAL | PATHFIND_SHORTCUT | PATHFIND_INCOMPLETE | PATHFIND_FARFROMPOLY_END))
            return;

    G3D::Vector3 result = path.getPath().back();
    destx = result.x;
    desty = result.y;
    destz = result.z;

    // check static LOS
    float halfHeight = 2.0f * 0.5f;
    bool col = false;

    // Unit is flying, check for potential collision via vmaps
    if (path.getPathType() & PATHFIND_NOT_USING_PATH)
    {
        col = VMAP::VMapFactory::createOrGetVMapManager()->getObjectHitPos(GetMapId(),
            pos.x, pos.y, pos.z + halfHeight,
            destx, desty, destz + halfHeight,
            destx, desty, destz, -0.5f);

        destz -= halfHeight;

        // Collided with static LOS object, move back to collision point
        if (col)
        {
            destx -= CONTACT_DISTANCE * std::cos(angle);
            desty -= CONTACT_DISTANCE * std::sin(angle);
            dist = std::sqrt((pos.x - destx) * (pos.x - destx) + (pos.y - desty) * (pos.y - desty));
        }
    }

    // check dynamic collision //todo

    destz -= halfHeight;

    // Collided with a gameobject, move back to collision point
    if (col)
    {
        destx -= CONTACT_DISTANCE * std::cos(angle);
        desty -= CONTACT_DISTANCE * std::sin(angle);
        dist = std::sqrt((pos.x - destx)*(pos.x - destx) + (pos.y - desty) * (pos.y - desty));
    }

    float groundZ = VMAP_INVALID_HEIGHT_VALUE;
    normalizeMapCoord(pos.x);
    normalizeMapCoord(pos.y);
    updateAllowedPositionZ(destx, desty, destz, &groundZ);

    pos.o = GetOrientation();
    pos.changeCoords(destx, desty, destz);

    // position has no ground under it (or is too far away)
    if (groundZ <= INVALID_HEIGHT)
    {
        if (Unit* unit = ToUnit())
        {
            // unit can fly, ignore.
            if (unit->canFly())
                return;

            // fall back to gridHeight if any
            float gridHeight = GetMapMgr()->GetADTLandHeight(pos.x, pos.y);
            if (gridHeight > INVALID_HEIGHT)
            {
                pos.z = gridHeight;
#if VERSION_STRING >= WotLK
                pos.z += unit->getHoverHeight();
#endif
            }
        }
    }
}

float Object::getMapWaterOrGroundLevel(float x, float y, float z, float* ground/* = nullptr*/)
{
    return GetMapMgr()->getWaterOrGroundLevel(GetPhase(), x, y, z, ground, getObjectTypeId() == TYPEID_UNIT ? !static_cast<Unit*>(this)->getAuraWithAuraEffect(SPELL_AURA_WATER_WALK) : false);
}

float Object::getDistance(Object const* obj) const
{
    float d = getExactDist(obj->GetPosition()) - getCombatReach() - obj->getCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float Object::getDistance(LocationVector const& pos) const
{
    float d = getExactDist(&pos) - getCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float Object::getDistance(float x, float y, float z) const
{
    float d = getExactDist(x, y, z) - getCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float Object::getDistance2d(Object const* obj) const
{
    float d = getExactDist2d(obj->GetPosition()) - getCombatReach() - obj->getCombatReach();
    return d > 0.0f ? d : 0.0f;
}

float Object::getDistance2d(float x, float y) const
{
    float d = getExactDist2d(x, y) - getCombatReach();
    return d > 0.0f ? d : 0.0f;
}

void MovementInfo::readMovementInfo(ByteBuffer& data, [[maybe_unused]]uint16_t opcode)
{
#if VERSION_STRING == Classic

    data >> flags >> update_time >> position >> position.o;

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
        data >> transport_guid >> transport_position >> transport_position.o;

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)))
        data >> pitch_rate;

    data >> fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data >> jump_info.velocity >> jump_info.sinAngle >> jump_info.cosAngle >> jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data >> spline_elevation;

#elif VERSION_STRING == TBC

    data >> flags >> flags2 >> update_time >> position >> position.o;

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
        data >> transport_guid >> transport_position >> transport_position.o >> transport_time;

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))
        data >> pitch_rate;

    data >> fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data >> jump_info.velocity >> jump_info.sinAngle >> jump_info.cosAngle >> jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data >> spline_elevation;

#elif VERSION_STRING == WotLK

    data >> guid >> flags >> flags2 >> update_time >> position >> position.o;

    sLogger.debug("guid: %u, flags: %u, flags2: %u, updatetime: %u, position: (%f, %f, %f, %f)",
        guid.getGuidLow(), flags, flags2, update_time, position.x, position.y, position.z, position.o);

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
    {
        WoWGuid tguid;
        data >> tguid >> transport_position >> transport_position.o >> transport_time >> transport_seat;

        transport_guid = tguid.getGuidLow();

        if (hasMovementFlag2(MOVEFLAG2_INTERPOLATED_MOVE))
            data >> transport_time2;

        sLogger.debug("tguid: %u, tposition: (%f, %f, %f, %f)", transport_guid, transport_position.x, transport_position.y, transport_position.z, transport_position.o);
    }

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))
        data >> pitch_rate;

    data >> fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data >> jump_info.velocity >> jump_info.sinAngle >> jump_info.cosAngle >> jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data >> spline_elevation;

#elif VERSION_STRING >= Cata
    bool hasTransportData = false,
        hasMovementFlags = false,
        hasMovementFlags2 = false;

    MovementStatusElements* sequence = GetMovementStatusElementsSequence(sOpcodeTables.getInternalIdForHex(opcode));
    if (!sequence)
    {
        sLogger.failure("Unsupported MovementInfo::Read for 0x%X (%s)!", opcode);
        return;
    }

    for (uint32_t i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEGuidBit0 && element <= MSEGuidBit7)
        {
            guid[element - MSEGuidBit0] = data.readBit();
            continue;
        }

        if (element >= MSEGuid2Bit0 && element <= MSEGuid2Bit7)
        {
            guid2[element - MSEGuid2Bit0] = data.readBit();
            continue;
        }

        if (element >= MSETransportGuidBit0 && element <= MSETransportGuidBit7)
        {
            if (hasTransportData)
                transport_guid[element - MSETransportGuidBit0] = data.readBit();
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            if (guid[element - MSEGuidByte0])
                guid[element - MSEGuidByte0] ^= data.readUInt8();
            continue;
        }

        if (element >= MSEGuid2Byte0 && element <= MSEGuid2Byte7)
        {
            if (guid2[element - MSEGuid2Byte0])
                guid2[element - MSEGuid2Byte0] ^= data.readUInt8();
            continue;
        }

        if (element >= MSETransportGuidByte0 && element <= MSETransportGuidByte7)
        {
            if (hasTransportData && transport_guid[element - MSETransportGuidByte0])
                transport_guid[element - MSETransportGuidByte0] ^= data.readUInt8();
            continue;
        }

        switch (element)
        {
            case MSEFlags:
                if (hasMovementFlags)
                    flags = data.readBits(30);
                break;
            case MSEFlags2:
                if (hasMovementFlags2)
                    flags2 = static_cast<uint16_t>(data.readBits(12));
                break;
            case MSEHasUnknownBit:
                data.readBit();
                break;
            case MSETimestamp:
                if (status_info.hasTimeStamp)
                    data >> update_time;
                break;
            case MSEHasTimestamp:
                status_info.hasTimeStamp = !data.readBit();
                break;
            case MSEHasOrientation:
                status_info.hasOrientation = !data.readBit();
                break;
            case MSEHasMovementFlags:
                hasMovementFlags = !data.readBit();
                break;
            case MSEHasMovementFlags2:
                hasMovementFlags2 = !data.readBit();
                break;
            case MSEHasPitch:
                status_info.hasPitch = !data.readBit();
                break;
            case MSEHasFallData:
                status_info.hasFallData = data.readBit();
                break;
            case MSEHasFallDirection:
                if (status_info.hasFallData)
                    status_info.hasFallDirection = data.readBit();
                break;
            case MSEHasTransportData:
                hasTransportData = data.readBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    status_info.hasTransportTime2 = data.readBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    status_info.hasTransportTime3 = data.readBit();
                break;
            case MSEHasSpline:
                status_info.hasSpline = data.readBit();
                break;
            case MSEHasSplineElevation:
                status_info.hasSplineElevation = !data.readBit();
                break;
            case MSEPositionX:
                data >> position.x;
                break;
            case MSEPositionY:
                data >> position.y;
                break;
            case MSEPositionZ:
                data >> position.z;
                break;
            case MSEPositionO:
                if (status_info.hasOrientation)
                    data >> position.o;
                break;
            case MSEPitch:
                if (status_info.hasPitch)
                    data >> pitch_rate;
                break;
            case MSEFallTime:
                if (status_info.hasFallData)
                    data >> fall_time;
                break;
            case MSESplineElevation:
                if (status_info.hasSplineElevation)
                    data >> spline_elevation;
                break;
            case MSEFallHorizontalSpeed:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.xyspeed;
                break;
            case MSEFallVerticalSpeed:
                if (status_info.hasFallData)
                    data >> jump_info.velocity;
                break;
            case MSEFallCosAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.cosAngle;
                break;
            case MSEFallSinAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data >> jump_info.sinAngle;
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> transport_seat;
                break;
            case MSETransportPositionO:
                if (hasTransportData)
                    data >> transport_position.o;
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> transport_position.x;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> transport_position.y;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> transport_position.z;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> transport_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && status_info.hasTransportTime2)
                    data >> transport_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && status_info.hasTransportTime3)
                    data >> fall_time;
                break;
            case MSEMovementCounter:
                data.read_skip<uint32_t>();
                break;
            case MSEByteParam:
                data >> byte_parameter;
                break;
            default:
                sLogger.failure("Wrong movement status element");
                break;
        }
    }
#endif
}

void MovementInfo::writeMovementInfo(ByteBuffer& data, [[maybe_unused]]uint16_t opcode, [[maybe_unused]]float custom_speed) const
{
#if VERSION_STRING == Classic

    data << guid << flags << update_time << position << position.o;

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
        data << transport_guid << transport_position << transport_position.o;

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)))
        data << pitch_rate;

    data << fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data << jump_info.velocity << jump_info.sinAngle << jump_info.cosAngle << jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data << spline_elevation;

#elif VERSION_STRING == TBC

    data << guid << flags << flags2 << update_time << position << position.o;

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
        data << transport_guid << transport_position << transport_position.o << transport_time;

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))
        data << pitch_rate;

    data << fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data << jump_info.velocity << jump_info.sinAngle << jump_info.cosAngle << jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data << spline_elevation;

#elif VERSION_STRING == WotLK

    data << guid << flags << flags2 << update_time << position << position.o;

    if (hasMovementFlag(MOVEFLAG_TRANSPORT))
    {
        data << transport_guid << transport_position << transport_position.o << transport_time << transport_seat;

        if (hasMovementFlag2(MOVEFLAG2_INTERPOLATED_MOVE))
            data << transport_time2;
    }

    if (hasMovementFlag(MovementFlags(MOVEFLAG_SWIMMING | MOVEFLAG_FLYING)) || hasMovementFlag2(MOVEFLAG2_ALLOW_PITCHING))
        data << pitch_rate;

    data << fall_time;

    if (hasMovementFlag(MOVEFLAG_FALLING))
        data << jump_info.velocity << jump_info.sinAngle << jump_info.cosAngle << jump_info.xyspeed;

    if (hasMovementFlag(MOVEFLAG_SPLINE_ELEVATION))
        data << spline_elevation;

#elif VERSION_STRING >= Cata
    bool hasTransportData = !transport_guid.IsEmpty();

    MovementStatusElements* sequence = GetMovementStatusElementsSequence(opcode);
    if (!sequence)
    {
        sLogger.failure("Unsupported MovementInfo::Write for 0x%X!", opcode);
        return;
    }

    for (uint32_t i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];

        if (element == MSEEnd)
            break;

        if (element >= MSEGuidBit0 && element <= MSEGuidBit7)
        {
            data.writeBit(guid[element - MSEGuidBit0]);
            continue;
        }

        if (element >= MSETransportGuidBit0 && element <= MSETransportGuidBit7)
        {
            if (hasTransportData)
                data.writeBit(transport_guid[element - MSETransportGuidBit0]);
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            if (guid[element - MSEGuidByte0])
                data << uint8_t((guid[element - MSEGuidByte0] ^ 1));
            continue;
        }

        if (element >= MSETransportGuidByte0 && element <= MSETransportGuidByte7)
        {
            if (hasTransportData && transport_guid[element - MSETransportGuidByte0])
                data << uint8_t((transport_guid[element - MSETransportGuidByte0] ^ 1));
            continue;
        }

        switch (element)
        {
            case MSEHasMovementFlags:
                data.writeBit(!flags);
                break;
            case MSEHasMovementFlags2:
                data.writeBit(!flags2);
                break;
            case MSEFlags:
                if (flags)
                    data.writeBits(flags, 30);
                break;
            case MSEFlags2:
                if (flags2)
                    data.writeBits(flags2, 12);
                break;
            case MSETimestamp:
                if (status_info.hasTimeStamp)
                    data << Util::getMSTime();
                break;
            case MSEHasPitch:
                data.writeBit(!status_info.hasPitch);
                break;
            case MSEHasTimestamp:
                data.writeBit(!status_info.hasTimeStamp);
                break;
            case MSEHasUnknownBit:
                data.writeBit(false);
                break;
            case MSEHasFallData:
                data.writeBit(status_info.hasFallData);
                break;
            case MSEHasFallDirection:
                if (status_info.hasFallData)
                    data.writeBit(status_info.hasFallDirection);
                break;
            case MSEHasTransportData:
                data.writeBit(hasTransportData);
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    data.writeBit(status_info.hasTransportTime2);
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    data.writeBit(status_info.hasTransportTime3);
                break;
            case MSEHasSpline:
                data.writeBit(status_info.hasSpline);
                break;
            case MSEHasSplineElevation:
                data.writeBit(!status_info.hasSplineElevation);
                break;
            case MSEPositionX:
                data << float(position.x);
                break;
            case MSEPositionY:
                data << float(position.y);
                break;
            case MSEPositionZ:
                data << float(position.z);
                break;
            case MSEPositionO:
                if (status_info.hasOrientation)
                    data << float(normalizeOrientation(position.o));
                break;
            case MSEPitch:
                if (status_info.hasPitch)
                    data << float(pitch_rate);
                break;
            case MSEHasOrientation:
                data.writeBit(!status_info.hasOrientation);
                break;
            case MSEFallTime:
                if (status_info.hasFallData)
                    data << uint32_t(fall_time);
                break;
            case MSESplineElevation:
                if (status_info.hasSplineElevation)
                    data << float(spline_elevation);
                break;
            case MSEFallHorizontalSpeed:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.xyspeed);
                break;
            case MSEFallVerticalSpeed:
                if (status_info.hasFallData)
                    data << float(jump_info.velocity);
                break;
            case MSEFallCosAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.cosAngle);
                break;
            case MSEFallSinAngle:
                if (status_info.hasFallData && status_info.hasFallDirection)
                    data << float(jump_info.sinAngle);
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data << int8_t(transport_seat);
                break;
            case MSETransportPositionO:
                if (hasTransportData)
                    data << float(normalizeOrientation(transport_position.o));
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data << float(transport_position.x);
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data << float(transport_position.y);
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data << float(transport_position.z);
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data << uint32_t(transport_time);
                break;
            case MSETransportTime2:
                if (hasTransportData && status_info.hasTransportTime2)
                    data << uint32_t(transport_time2);
                break;
            case MSETransportTime3:
                if (hasTransportData && status_info.hasTransportTime3)
                    data << uint32_t(fall_time);
                break;
            case MSEMovementCounter:
                data << uint32_t(0);
                break;
            case MSECustomSpeed:
                data << float(custom_speed);
                break;
            default:
                sLogger.failure("Wrong movement status element");
                break;
        }
    }
#endif
}
