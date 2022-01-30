/*
 * This file is part of the sfv-server distribution (https://github.com/peckto/svf-server.
 * Copyright (c) 2022 Tobias Specht.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SVFSERVER_H
#define SVFSERVER_H
#include "Graphs/SVFG.h"
#include <string>
#include <list>

class Plugin {
public:
    virtual void init(SVF::ICFG* icfg, SVF::SVFG *svfg) = 0;
    virtual void help() = 0;
    virtual void run(std::string funcName, std::list<std::string> &args) = 0;
};

typedef Plugin* (*svf_analyzer)();
typedef void (*pluginHelp)();


#endif // SVFSERVER_H
