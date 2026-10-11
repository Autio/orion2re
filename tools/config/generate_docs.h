#pragma once

#include <string>

#include "schema_model.h"

namespace config::tool {

bool Generate_Documentation(const s_schema* schema, const char* output_directory,
                            std::string* diagnostic);

}