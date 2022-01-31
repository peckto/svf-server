#include "ConsoleInput.h"
#include "Util/SVFUtil.h"
#include <iostream>
#include <string>
#include <list>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace llvm;
using namespace std;
using namespace SVF;

ConsoleInput::ConsoleInput()
{

}

std::list<std::string> ConsoleInput::readLine()
{
    string line;
    while (true) {
        SVFUtil::outs() << "> ";
        getline(std::cin, line);
        std::list<std::string> args;
        boost::split(args, line, boost::is_any_of(" "), boost::token_compress_on);
        if (args.size() == 0) {
            continue;
        }
        return args;
    }
}
