#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <string_view>
using namespace std;

vector<string> parse_resp(string_view buffer);

#endif