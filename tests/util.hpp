#pragma once

#include <string>

// Returns the absolute path (starting from the root) for a relative path
std::string path(const std::string &relpath);

// Execute a command (equivalent to `system(command > /tmp/file)` and ifstream(/tmp/file)),
// Populate data with stdout, return the return value 
int exec(const std::string &command, std::string &data);

void trim_end(std::string &str);