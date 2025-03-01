/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Setup.h"
#include "Instance_BlackrockDepths.h"
#include "Server/Script/CreatureAIScript.h"

class BlackrockDepthsInstanceScript : public InstanceScript
{
public:
    explicit BlackrockDepthsInstanceScript(MapMgr* pMapMgr) : InstanceScript(pMapMgr){}
    static InstanceScript* Create(MapMgr* pMapMgr) { return new BlackrockDepthsInstanceScript(pMapMgr); }
};

void SetupBlackrockDepths(ScriptMgr* mgr)
{
    mgr->register_instance_script(MAP_BLACKROCK_DEPTHS, &BlackrockDepthsInstanceScript::Create);
}
