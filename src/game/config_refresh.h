#pragma once

#include "config/generated/config_groups.h"

namespace CONFIG_REFRESH {
    void Tech_Names_Loaded();
    void Refresh_After_Config_Group_Reload(config::t_config_group group);
    void Refresh_After_Config_Reload(const bool* reloaded_groups = nullptr);
} // namespace CONFIG_REFRESH