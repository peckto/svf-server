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

#include "Util/SVFUtil.h"
#include "svf-server.h"
#include "WPA/Andersen.h"
#include <list>
#include <regex>
#include <boost/algorithm/string/predicate.hpp>

using namespace llvm;
using namespace std;
using namespace SVF;


class MyPlugin : public Plugin {
    ICFG* icfg;
    SVFG *svfg;

public:
    void init(ICFG* icfg, SVFG *svfg) {
        this->icfg = icfg;
        this->svfg = svfg;
    }
    void help() {
        SVFUtil::outs() << "plugin help\n";
        SVFUtil::outs() << "    findAllMemcpys           finds and prints all memcpy calls\n";
        SVFUtil::outs() << "    findArrayMemcpys         finds and prints all memcpy calls with array type dst buffer\n";
        SVFUtil::outs() << "    findMissingRetCodeCheck  [ignore func, ...] finds missing return code checks\n";
    }

    void run(string funcName, list<string> &args) {
        if (funcName == "findAllMemcpys") {
            findAllMemcpys(args);
        } else if (funcName == "findArrayMemcpys") {
            findArrayMemcpys(args);
        } else if (funcName == "findMissingRetCodeCheck") {
            findMissingRetCodeCheck(args);
        } else {
            SVFUtil::outs() << "Error: Could not find function name: " << funcName << "\n";
            help();
        }
    }

   llvm::Type *getBufferTypeInter(const ICFGNode *node, const Value *value, llvm::Type::TypeID *typeId) {
        auto func = node->getFun();
        //SVFUtil::outs() << "  dst var name: " << varName << " uses: " << value->getNumUses() << "\n";

        node = (*node->getInEdges().begin())->getSrcNode();
        bool load = false;
        while (node->getInEdges().size() == 1) {
            if (node->getFun() != func) {
                // we do only intra-procedural analysis
                *typeId = value->getType()->getTypeID();
                return value->getType();
            }
            //SVFUtil::outs() << "    " << node->toString() << "\n";
            for (auto &vnode : node->getSVFStmts()) {
                auto inst = vnode->getInst();
                if (auto g = dyn_cast<llvm::GetElementPtrInst>(inst)) {
                    if (*g->use_begin() == value) {
                        //SVFUtil::outs() << "  GetElementPtrInst name: " << v << "\n";
                        auto t = g->getSourceElementType();
                        *typeId = t->getTypeID();
                        if (auto st = dyn_cast<llvm::StructType>(t)) {
                            if (auto nr = dyn_cast<llvm::ConstantInt>(g->getOperand(2))) {
                                auto el = st->getElementType(nr->getSExtValue());
                                *typeId = el->getTypeID();
                                return el;
                            }
                        }
                        return t;
                    }
                } else if (auto g = dyn_cast<llvm::SExtInst>(inst)) {
                    if (load) {
                        *typeId = value->getType()->getTypeID();
                        return value->getType();
                    }
                    if (*g->use_begin() == value) {
                        *typeId = value->getType()->getTypeID();
                        //SVFUtil::outs() << "  SExtInst name: " << v << "\n";
                        return value->getType();
                    }
                } else if (auto g = dyn_cast<llvm::ZExtInst>(inst)) {
                    if (load) {
                        *typeId = value->getType()->getTypeID();
                        return value->getType();
                    }
                    if (*g->use_begin() == value) {
                        *typeId = value->getType()->getTypeID();
                        //SVFUtil::outs() << "  ZExtInst name: " << v << "\n";
                        return value->getType();
                    }
                } else if (auto g = dyn_cast<llvm::LoadInst>(inst)) {
                    if (load) {
                        *typeId = value->getType()->getTypeID();
                        return value->getType();
                    }
                    if (*g->use_begin() == value) {
                        //SVFUtil::outs() << "  LoadInst name: " << v << " -> " << d << "\n";
                        value = g->getPointerOperand();
                        load = true;
                    }
                } else {
                    // unsupported instruction for value tracing, abort
                    *typeId = value->getType()->getTypeID();
                    return value->getType();
                }
            }
            node = (*node->getInEdges().begin())->getSrcNode();
        }
        *typeId = value->getType()->getTypeID();
        return value->getType();
    }

    void findAllMemcpys(list<string> &args) {
        (void)args;
        int count = 0;
        for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
            ICFGNode *n = i->second;
            if (n->getNodeKind() == ICFGNode::FunCallBlock) {
                CallBlockNode *c = static_cast<CallBlockNode *>(n);
                if (c->toString().find("@llvm.memcpy") != string::npos) {
                    SVFUtil::outs() << "* " << c->toString() << "\n";
                    auto opt1 = c->getActualParms()[0]->getValue();
                    llvm::Type::TypeID typeId;
                    getBufferTypeInter(n, opt1, &typeId);
                    if (typeId == llvm::Type::ArrayTyID) {
                        SVFUtil::outs() << "  array type\n";
                    } else if (typeId == llvm::Type::StructTyID) {
                        SVFUtil::outs() << "  struct type\n";
                    } else if (typeId == llvm::Type::PointerTyID) {
                        SVFUtil::outs() << "  pointer type\n";
                    } else {
                        SVFUtil::outs() << "  other type\n";
                    }
                    count++;
                }
            }
        }
        SVFUtil::outs() << "Found " << count << " memcpy calls\n";
    }

    void findArrayMemcpys(list<string> &args) {
        (void)args;
        int count = 0;
        for(ICFG::iterator i = icfg->begin(); i != icfg->end(); i++) {
            ICFGNode *n = i->second;
            if (n->getNodeKind() == ICFGNode::FunCallBlock) {
                CallBlockNode *c = static_cast<CallBlockNode *>(n);
                if (c->toString().find("@llvm.memcpy") != string::npos) {
                    auto opt1 = c->getActualParms()[0]->getValue();
                    llvm::Type::TypeID typeId;
                    auto bufferType = getBufferTypeInter(n, opt1, &typeId);
                    if (typeId == llvm::Type::ArrayTyID) {
                        auto op2 = c->getActualParms()[2]->getValue();
                        int bufferSize = bufferType->getArrayNumElements();
                        if (auto i = dyn_cast<const llvm::ConstantInt>(op2)) {
                            auto len = i->getSExtValue();
                            if (len > bufferSize) {
                                SVFUtil::outs() << "! " << c->getFun()->getName() << " " << SVFUtil::getSourceLoc((*c->getSVFStmts().begin())->getInst()) << "\n";
                                SVFUtil::outs() << "   len: " << len << " buffer size: " << bufferSize << "\n";
                                count++;
                            } else {
                                // buffer size ok
                            }
                        } else {
                            SVFUtil::outs() << "* " << c->getFun()->getName() << " " << SVFUtil::getSourceLoc((*c->getSVFStmts().begin())->getInst()) << "\n";
                            count++;
                        }
                    }
                }
            }
        }
        SVFUtil::outs() << "Found " << count << " memcpy calls\n";
    }

    void findMissingRetCodeCheck(list<string> &args) {
        int count = 0;
        for (auto i = this->icfg->begin(); i != this->icfg->end(); i++) {
            auto n = i->second;
            if (n->getNodeKind() == ICFGNode::FunRetBlock) {
                auto en = static_cast<RetBlockNode*>(n);
                auto callSite = en->getCallBlockNode()->getCallSite();
                if (auto call = dyn_cast<CallInst>(callSite)) {
                    auto func = call->getCalledFunction();
                    string funcName = "???";
                    if (func != NULL) {
                        funcName = string(func->getName());
                    }
                    if (find(args.begin(), args.end(), funcName) != args.end()) {
                        // ignore this function
                        continue;
                    }
                    auto retVar = en->getActualRet();
                    if (retVar != NULL) {
                        const VFGNode* vNode = this->svfg->getDefSVFGNode(retVar);
                        if (vNode->getOutEdges().size() == 0) {
                            SVFUtil::outs() << "* " << n->getFun()->getName() << " -> " << funcName << " " << SVFUtil::getSourceLoc(retVar->getValue()) << "\n";
                            count++;
                        }
                    }
                } else {
                    SVFUtil::outs() << "Error: could not cast CallSite to CallInst\n";
                }
            }

        }
        SVFUtil::outs() << "Found " << count << " missing return code checks\n";
    }
};


extern "C" Plugin* getPlugin() {
    return static_cast<Plugin*>(new MyPlugin);
}
