#include "parser.h"
using namespace std;

vector<string> parse_resp(string_view buffer){
    vector<string> args;

    if(buffer.empty() || buffer[0] != '*'){
        return args;
    }

    size_t pos = buffer.find("\r\n");
    if(pos == string_view :: npos) return args;

    int num_elements = stoi(string(buffer.substr(1, pos-1)));
    pos += 2;

    for(int i=0; i<num_elements; i++){
        if(pos >= buffer.size() || buffer[pos] != '$') break;

        size_t next_run = buffer.find("\r\n", pos);
        if(next_run == string_view ::npos) break;

        int str_len = stoi(string(buffer.substr(pos+1, next_run-pos-1)));
        pos = next_run + 2;

        args.push_back(string(buffer.substr(pos, str_len)));

        pos += str_len+2;
    }

    return args;
}