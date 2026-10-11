#pragma once

#include <string>

namespace config::tool {

bool Read_File(const char* path, std::string* content);
bool Write_File_If_Different(const char* path, const std::string& content,
                             std::string* diagnostic);

}