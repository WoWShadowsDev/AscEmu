/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Setup.h"
#include "Raid_EyeOfEternity.h"
#include <Objects/Units/Creatures/Pet.h>
#include "Server/Script/CreatureAIScript.h"

class EyeOfEternityInstanceScript : public InstanceScript
{
public:
    explicit EyeOfEternityInstanceScript(MapMgr* pMapMgr) : InstanceScript(pMapMgr){}
    static InstanceScript* Create(MapMgr* pMapMgr) { return new EyeOfEternityInstanceScript(pMapMgr); }

    void OnLoad() override
    {
        // Load All Cells in Our Instance
        GetInstance()->updateAllCells(true);
    }
};

void SetupEyeOfEternity(ScriptMgr* mgr)
{
    mgr->register_instance_script(MAP_THE_EYE_OF_ETERNITY, &EyeOfEternityInstanceScript::Create);
}
