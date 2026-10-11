#pragma once

#include <string>

#include "schema_model.h"

namespace config::tool {

bool Generate_Cpp(const s_schema* schema, const char* output_directory,
                  std::string* diagnostic);
bool Check_Generated_Cpp(const s_schema* schema, const char* committed_directory,
                         const char* temporary_directory, std::string* diagnostic);

}