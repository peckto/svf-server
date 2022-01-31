#ifndef CONSOLEINPUT_H
#define CONSOLEINPUT_H

#include <string>
#include <list>


class ConsoleInput
{
public:
    ConsoleInput();
    std::list<std::string> readLine();
};

#endif // CONSOLEINPUT_H
