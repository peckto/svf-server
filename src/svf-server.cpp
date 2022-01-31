/*
 * This file is part of the svf-server distribution (https://github.com/peckto/svf-server).
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

#include "llvm/Support/Casting.h"
#include "SVF-FE/LLVMUtil.h"
#include "Util/SVFUtil.h"
#include "Graphs/SVFG.h"
#include "WPA/Andersen.h"
#include "WPA/Steensgaard.h"
#include "WPA/AndersenSFR.h"
#include "WPA/FlowSensitive.h"
#include "WPA/FlowSensitiveTBHC.h"
#include "WPA/VersionedFlowSensitive.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"
#include <dlfcn.h>
#include "svf-plugin.h"

#include <list>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace llvm;
using namespace std;
using namespace SVF;

static llvm::cl::opt<std::string> InputFilename(cl::Positional,
        llvm::cl::desc("<input bitcode>"), llvm::cl::init("-"));


void usage() {
    SVFUtil::outs() << "Available commands:\n";
    SVFUtil::outs() << "    load <lib.so>  load library\n";
    SVFUtil::outs() << "    ls             show available functions provided by loaded library\n";
    SVFUtil::outs() << "    run <func>     run function from loaded library\n";
    SVFUtil::outs() << "    help|?         show this help\n";
    SVFUtil::outs() << "    exit           stop the server\n";
}

int main(int argc, char ** argv)
{
    SVFUtil::outs().SetUnbuffered();

    int arg_num = 0;
    char **arg_value = new char*[argc];
    std::vector<std::string> moduleNameVec;
    SVFUtil::processArguments(argc, argv, arg_num, arg_value, moduleNameVec);
    cl::ParseCommandLineOptions(arg_num, arg_value,
                                "Whole Program Points-to Analysis\n");

    if (Options::WriteAnder == "ir_annotator")
    {
        LLVMModuleSet::getLLVMModuleSet()->preProcessBCs(moduleNameVec);
    }

    SVFUtil::outs() << "init svf module...\n";
    SVFModule* svfModule = LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);
    svfModule->buildSymbolTableInfo();

    /// Build Program Assignment Graph (SVFIR)
    SVFIRBuilder builder;
    SVFUtil::outs() << "build pag...\n";
    SVFIR* pag = builder.build(svfModule);

    /// Create Andersen's pointer analysis
    SVFUtil::outs() << "running pointer analysis...\n";
    PointerAnalysis* ander;
    //ander = FlowSensitive::createFSWPA(pag);
    //ander = Steensgaard::createSteensgaard(pag);
    //ander = FlowSensitiveTBHC::createFSWPA(pag);
    //ander = VersionedFlowSensitive::createVFSWPA(pag); // not supported
    //ander = AndersenSFR::createAndersenSFR(pag); // crash
    //ander = AndersenSCD::createAndersenSCD(pag); // crash
    //ander = AndersenHLCD::createAndersenHLCD(pag);
    //ander = AndersenHCD::createAndersenHCD(pag);
    //ander = AndersenLCD::createAndersenLCD(pag);
    ander = AndersenWaveDiff::createAndersenWaveDiff(pag);

    /// Query aliases
    /// aliasQuery(ander,value1,value2);

    /// Print points-to information
    /// printPts(ander, value1);

    /// Call Graph
    SVFUtil::outs() << "create call graph...\n";
    PTACallGraph* callgraph = ander->getPTACallGraph();

    /// ICFG
    SVFUtil::outs() << "create ICFG...\n";
    ICFG* icfg = pag->getICFG();

    /// Value-Flow Graph (VFG)
    SVFUtil::outs() << "create VFG...\n";
    VFG* vfg = new VFG(callgraph);

    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder;
    SVFUtil::outs() << "create svfg...\n";
    //SVFG* svfg = svfBuilder.buildFullSVFGWithoutOPT((BVDataPTAImpl*)ander);
    SVFG* svfg = svfBuilder.buildFullSVFG((BVDataPTAImpl*)ander);

    // server loop
    void *lib = NULL;
    Plugin *plugin = NULL;
    string line;
    string cmd;
    string opt;
    while (true) {
        SVFUtil::outs() << "> ";
        getline(std::cin, line);
        std::list<std::string> args;
        boost::split(args, line, boost::is_any_of(" "), boost::token_compress_on);
        if (args.size() == 0) {
            continue;
        }
        cmd = *args.cbegin();
        args.pop_front();
        if (args.size() > 0) {
            opt = *args.cbegin();
            args.pop_front();
        }

        if (cmd == "exit") {
            SVFUtil::outs() << "Stopping server...\n";
            break;
        } else if (cmd == "load") {
            // Load dym lib
            if (opt.empty()) {
                SVFUtil::outs() << "Error: please provide argument\n";
                usage();
                continue;
            }
            if (plugin != NULL) {
                delete plugin;
                plugin = NULL;
            }
            if (lib != NULL) {
                dlclose(lib);
                lib = NULL;
            }
            lib = dlopen(opt.c_str(), RTLD_NOW);
            if (lib == NULL) {
                SVFUtil::outs() << "Error: Coudl not load library\n";
                continue;
            }
            void *maker = dlsym(lib, "getPlugin");
            if (maker == NULL) {
                SVFUtil::outs() << "Error: Could not find getPlugin function for plugin\n";
                continue;
            }
            svf_analyzer func = reinterpret_cast<svf_analyzer>(reinterpret_cast<void*>(maker));
            plugin = func();
            if (plugin == NULL) {
                SVFUtil::outs() << "Error: could not instantiate plugin object\n";
                continue;
            }
            plugin->init(icfg, svfg);
            SVFUtil::outs() << "library loaded\n";
        } else if (cmd == "ls") {
            // list help of plugin
            if (plugin == NULL) {
                SVFUtil::outs() << "Error: No library was loaded. Please load library first\n";
                continue;
            }
            plugin->help();
        } else if (cmd == "help") {
            usage();
        } else if (cmd == "?") {
            usage();
        } else if (cmd == "run") {
            // run function from lib
            if (opt.empty()) {
                SVFUtil::outs() << "Error: please provide argument\n";
                usage();
                continue;
            }
            if (plugin == NULL) {
                SVFUtil::outs() << "Error: No library was loaded. Please load library first\n";
                continue;
            }
            plugin->run(opt, args);
        } else {
            SVFUtil::outs() << "Error: Invalid command: " << line << "\n";
            usage();
        }
    }
    if (plugin != NULL) {
        delete plugin;
        plugin = NULL;
    }
    if (lib != NULL) {
        dlclose(lib);
        lib = NULL;
    }

    // clean up memory
    delete vfg;
    delete svfg;
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();

    //LLVMModuleSet::getLLVMModuleSet()->dumpModulesToFile(".svf.bc");
    SVF::LLVMModuleSet::releaseLLVMModuleSet();

    llvm::llvm_shutdown();
    return 0;
}

