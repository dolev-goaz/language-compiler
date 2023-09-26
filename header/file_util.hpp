#pragma once

#include <fstream>
#include <iostream>
#include <vector>

std::string read_file(const std::string& path);

void write_file(std::string filename, std::string contents);