#ifndef CONSOLEINPUT_H
#define CONSOLEINPUT_H

#include <string>
#include <list>


class ConsoleInput
{
private:
    std::string prev_cmd;
    std::string history_file;
    int history_max_entries;

public:
    ConsoleInput();
    std::list<std::string> readLine();
};

#endif // CONSOLEINPUT_H
