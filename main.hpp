#pragma once

#include "paper2_scotland2/shared/logger.hpp"
#include "scotland2/shared/loader.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

// MOD_ID and VERSION are defined by the template's CMakeLists from mod.template.json
static constexpr auto PaperLogger = Paper::ConstLoggerContext("CustomBackgrounds");
