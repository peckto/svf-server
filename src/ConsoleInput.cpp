#include "ConsoleInput.h"
#include "Util/SVFUtil.h"
#include <iostream>
#include <string>
#include <list>
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <filesystem>

namespace fs = std::filesystem;

using namespace llvm;
using namespace std;
using namespace SVF;

ConsoleInput::ConsoleInput()
{
    fs::path home("/home");
    fs::path user(getenv("USER"));
    fs::path hist_file(".local/share/svf-server-history");
    fs::path hist = home / user.filename() / hist_file;
    history_file = hist;
    history_max_entries = 100;
    read_history(history_file.c_str());
}

std::list<std::string> ConsoleInput::readLine()
{
    string line;
    char *s;
    while (true) {
        s=readline("> ");
        line = s;
        boost::algorithm::trim(line);
        if (line.size() > 0 && line != prev_cmd) {
            add_history(s);
            stifle_history(history_max_entries);
            write_history(history_file.c_str());
        }
        prev_cmd = line;
        free(s);
        if (line.size() == 0) {
            continue;
        }
        std::list<std::string> args;
        boost::split(args, line, boost::is_any_of(" "), boost::token_compress_on);
        return args;
    }
}
