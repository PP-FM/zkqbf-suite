//
// Created by anonymized on 12/7/21.
//
#pragma once;
#ifndef ZKUNSAT_NEW_COMMONS_H
#define ZKUNSAT_NEW_COMMONS_H
#include "emp-zk/emp-zk.h"
#include "emp-zk/extensions/ram-zk/ostriple.h"
#include <NTL/GF2EX.h>
#include <NTL/GF2E.h>
#include <NTL/GF2EXFactoring.h>
#include <map>
#include "emp-zk/emp-vole-f2k/base_svole.h"
#include <algorithm>

extern vector<int64_t> quantifiers;
extern uint64_t pad_lit;
extern int nvars;
extern block index_retriever;
extern Integer index_retriever_Integer;
extern SVoleF2k<BoolIO<NetIO>> *svole;
extern F2kOSTriple<BoolIO<NetIO>>* ostriple;
extern BoolIO<NetIO>* io;
extern uint64_t constant;
extern block *data;
extern block *mac;
extern uint64_t data_mac_pointer;
#endif //ZKUNSAT_NEW_COMMONS_H
