//
// Created by anonymized on 12/7/21.
//

#ifndef ZKUNSAT_NEW_CLAUSERAM_H
#define ZKUNSAT_NEW_CLAUSERAM_H


//
// Created by anonymized on 10/4/21.
//

#include "emp-zk/emp-zk.h"
#include "emp-zk/extensions/ram-zk/ostriple.h"
#include "clause.h"
#include "commons.h"
#define INDEX_SZ 20
typedef vector<uint64_t> clause_raw;

#include "emp-zk/emp-zk.h"
#include "emp-zk/extensions/ram-zk/ostriple.h"


inline void get_raw(vector<uint64_t>& raw, const clause& input){
    raw = input.literals;
}

template<typename IO>
class clauseRAM {
public:
    double	check1 = 0, check2 = 0, check3 = 0;
    int party;
    int index_sz;
    int deg;
    uint64_t step = 0;
    vector<clause_raw> clear_mem;
    vector<pair<uint64_t, clause_raw>> clear_access_record;
    vector<pair<Integer, clause>> access_record;
    vector<block> hash_block;
    pair<block, block> hash_pair;
    IO * io;
    block Delta;
    F2kOSTriple<IO> *ostriple = nullptr;

    clauseRAM(int _party, int index_sz, int deg): party(_party), index_sz(index_sz) {
        ZKBoolCircExec<IO> *exec = (ZKBoolCircExec<IO>*)(CircuitExecution::circ_exec);
        io = exec->ostriple->io;
        Delta = exec->ostriple->delta;
        this->deg = deg;
        ostriple = new F2kOSTriple<IO>(party, exec->ostriple->threads, exec->ostriple->ios, exec->ostriple->ferret, exec->ostriple->pool);
        for(int i = 0; i < deg; i ++){
            hash_block.push_back(one_block);
        }
        hash_pair.first = zero_block;
        hash_pair.second = zero_block;
    }

    ~clauseRAM() {
        delete ostriple;
    }

    void init(vector<clause> &data) {
        clause_raw val(deg);
        for(size_t i = 0; i < data.size(); ++i) {
            get_raw(val, data[i]);
            clear_mem.push_back(val);
            clear_access_record.push_back(make_pair((uint64_t)i, val));
            access_record.push_back(make_pair(Integer(index_sz + 1,i, ALICE), data[i]));
        }
    }

    clause get(const Integer & index) {
        uint64_t clear_index = index.reveal<uint64_t>(ALICE);
        clause_raw tmp(deg);
        if(party == ALICE) {
            tmp = clear_mem[clear_index];
        }
        clause res(tmp, deg);
        clear_access_record.push_back(make_pair(clear_index, tmp));
        access_record.push_back(make_pair(index, res));
        ++step;
        if(step == clear_mem.size()* 2)
            check();
        return res;
    }

    void check() {
        vector<pair<uint64_t, clause_raw>> sorted_clear_access;
        sorted_clear_access = vector<pair<uint64_t, clause_raw>>(this->clear_access_record.begin(), this->clear_access_record.end());
        if(party == ALICE) {
            sort(sorted_clear_access.begin(), sorted_clear_access.end());
        }
        vector<Integer> sorted_index;
        vector<clause> sorted_clause;
        for (int i = 0; i < access_record.size(); i ++){
            auto item  = sorted_clear_access[i];
            sorted_index.push_back(Integer(index_sz, item.first, ALICE));
            clause c(item.second, deg);
            sorted_clause.push_back(c);
        }

        update_hash();
        vector<__uint128_t> HRecord;
        vector<block> HRecord_mac;
        sync_zk_bool<IO>();

        for (int i = 0; i < access_record.size(); i ++){
            Integer h = getHash(access_record[i].second);
            block v, m;
            hash_and_mac(v, m, access_record[i].first, h);
            HRecord.emplace_back((__uint128_t)v);
            HRecord_mac.push_back(m);
        }


        vector<__uint128_t> sorted_HRecord;
        vector<block> sorted_HRecord_mac;
        vector<Integer> sorted_hash_value;

        for(int i = 0; i < sorted_clear_access.size(); i++){
            Integer h = getHash(sorted_clause[i]);
            sorted_hash_value.push_back(h);
            block v, m;
            hash_and_mac(v, m, sorted_index[i], h);
            sorted_HRecord.emplace_back((__uint128_t)v);
            sorted_HRecord_mac.push_back(m);
        }

        bool cheat = true;
        for(size_t i = 0; i < sorted_index.size()-1; ++i) {
            Bit eq = !(sorted_index[i].geq(sorted_index[i+1])) | (sorted_index[i].equal(sorted_index[i+1]) & sorted_hash_value[i].equal(sorted_hash_value[i+1]));
            bool res = eq.reveal<bool>(PUBLIC);
            cheat = cheat and res;
        }
        if(!cheat) error("cheat!");

        sync_zk_bool<IO>();
        check_set_euqality(sorted_HRecord, sorted_HRecord_mac, HRecord, HRecord_mac);
        access_record.resize(clear_mem.size());
        clear_access_record.resize(clear_mem.size());
        step = 0;
    }

    void update_hash(){
        io->flush();
        for (int i  =0; i < deg; i ++){
            block r = io->get_hash_block();
            this->hash_block[i] = r;
            io -> flush();
        }

        block r = io->get_hash_block();
        this->hash_pair.first = r;
        io -> flush();
        r = io->get_hash_block();
        this->hash_pair.second = r;
        io -> flush();
    }

    Integer getHash(clause& c){
        vector<Integer> literals;
        block hash = zero_block;
        block hash_mac = zero_block;
        c.get_literals(literals);
        for (int i=0; i < literals.size(); i ++){
            block val, mac;
            val = (block)pack(literals[i].reveal<uint64_t>(ALICE));
            get_mac(mac, literals[i], literals[i].size());
            multiply_const(val, mac, val, mac, hash_block[i], ostriple->party);
            hash = hash ^ val;
            hash_mac = hash_mac ^ mac;
        }
        Integer hash_value(128, &hash, ALICE);
        block hash_mac_get;
        get_mac(hash_mac_get, hash_value, 128);
        check_zero_MAC(hash_mac_get ^ hash_mac);
        return hash_value;
    }


    Integer getHash(clause_raw& c) {
        block hash = zero_block;
        for (int i =0; i < c.size(); i ++){
            block val = (block)pack(c[i]);
            block tmp;
            gfmul(val, hash_block[i], &tmp);
            hash = hash ^ tmp;
        }
//        uint64_t  h = _mm_extract_epi64(hash, 0);
        return Integer(128, &hash, ALICE);
    }

    void hash_and_mac(block& hash, block& mac,const Integer& index, const Integer& val ){

        block mac1, mac2;
        get_mac(mac1, val, 128);
        get_mac(mac2, index, index.size());
        __uint128_t _res1;
        val.reveal<__uint128_t>(&_res1, ALICE);
        block res1 = (block)_res1;
        block res2 = (block)pack(index.reveal<uint64_t>(ALICE));
        gfmul(mac1, hash_pair.first, &mac1);
        gfmul(mac2, hash_pair.second, &mac2);
        mac = mac1 ^ mac2;
        gfmul(res1, hash_pair.first, &res1);
        gfmul(res2, hash_pair.second, &res2);
        hash = res1 ^ res2;
    }




    void vector_inn_prdt(block &xx, block &mm, vector<__uint128_t> &X, vector<block> &MAC, block r) {
        block x, m;
        size_t i = 1;
        block tmp = (block)X[0];
        ostriple->compute_add_const(xx, mm, tmp, MAC[0], r);
        while(i < X.size()) {
            tmp = (block)X[i];
            ostriple->compute_add_const(x, m, tmp, MAC[i], r);
            ostriple->compute_mul(xx, mm, xx, mm, x, m);
            ++i;
        }
    }

    void vector_inn_prdt_bch2(block &xx, block &mm, vector<__uint128_t> &X, vector<block> &MAC, block r) {
        block x[2], m[2], t[2];
        size_t i = 1;
        block tmp = (block)X[0];
        ostriple->compute_add_const(xx, mm, tmp, MAC[0], r);
        while(i < X.size()-1) {
            for(int j = 0; j < 2; ++j) {
                t[j] = (block)X[i+j];
                ostriple->compute_add_const(x[j], m[j], t[j], MAC[i+j], r);
            }
            ostriple->compute_mul3(xx, mm, x[0], m[0],
                                   x[1], m[1], xx, mm);
            i += 2;
        }
        while(i < X.size()) {
            t[0] = (block)X[i];
            ostriple->compute_add_const(x[0], m[0], t[0], MAC[i], r);
            ostriple->compute_mul(xx, mm, xx, mm, x[0], m[0]);
            ++i;
        }
    }

    void vector_inn_prdt_bch3(block &xx, block &mm, vector<__uint128_t> &X, vector<block> &MAC, block r) {
        block x[3], m[3], t[3];
        size_t i = 1;
        block tmp = (block)X[0];
        ostriple->compute_add_const(xx, mm, tmp, MAC[0], r);
        while(i < X.size()-2) {
            for(int j = 0; j < 3; ++j) {
                t[j] = (block)X[i+j];
                ostriple->compute_add_const(x[j], m[j], t[j], MAC[i+j], r);
            }
            ostriple->compute_mul4(xx, mm, x[0], m[0],
                                   x[1], m[1], x[2], m[2], xx, mm);
            i += 3;
        }
        while(i < X.size()) {
            t[0] = (block)X[i];
            ostriple->compute_add_const(x[0], m[0], t[0], MAC[i], r);
            ostriple->compute_mul(xx, mm, xx, mm, x[0], m[0]);
            ++i;
        }
    }

    void vector_inn_prdt_bch4(block &xx, block &mm, vector<__uint128_t> &X, vector<block> &MAC, block r) {
        block x[4], m[4], t[4];
        size_t i = 1;
        block tmp = (block)X[0];
        ostriple->compute_add_const(xx, mm, tmp, MAC[0], r);
        while(i < X.size()-3) {
            for(int j = 0; j < 4; ++j) {
                t[j] = (block)X[i+j];
                ostriple->compute_add_const(x[j], m[j], t[j], MAC[i+j], r);
            }
            ostriple->compute_mul5(xx, mm, x[0], m[0],
                                   x[1], m[1], x[2], m[2], x[3], m[3], xx, mm);
            i += 4;
        }
        while(i < X.size()) {
            t[0] = (block)X[i];
            ostriple->compute_add_const(x[0], m[0], t[0], MAC[i], r);
            ostriple->compute_mul(xx, mm, xx, mm, x[0], m[0]);
            ++i;
        }
    }

    // mult batch 4
    void check_set_euqality(vector<__uint128_t> & sorted_X, vector<block>& sorted_MAC, vector<__uint128_t>& check_X, vector<block>& check_MAC) {
        block r, val[2], mac[2];
        r = io->get_hash_block();
        vector_inn_prdt_bch4(val[0], mac[0], sorted_X, sorted_MAC, r);
        vector_inn_prdt_bch4(val[1], mac[1], check_X, check_MAC, r);

        // TODO comparison
        if(party == ALICE) {
            io->send_data(mac, 2*sizeof(block));
            io->flush();
        } else {
            block macrecv[2];
            io->recv_data(macrecv, 2*sizeof(block));
            mac[0] ^= macrecv[0];
            mac[1] ^= macrecv[1];
            if(memcmp(mac, mac+1, 16) != 0) {
                error("check set equality failed!\n");
            }
        }
    }

    void check_MAC_valid(block X, block MAC) {
        if(party == ALICE) {
            io->send_data(&X, 16);
            io->send_data(&MAC, 16);
        } else {
            block M = zero_block, x = zero_block;
            io->recv_data(&x, 16);
            io->recv_data(&M, 16);
            gfmul(x, Delta, &x);
            x = x ^ MAC;
            if (memcmp(&x, &M, 16)!=0) {
                error("check_MAC failed!\n");
            }
        }
    }

    void get_mac(block& mac, const Integer& val, int size) {
        block res;
        assert(!(val.size() > size));
        vector_inn_prdt_sum_red(&res, (block*)(val.bits.data()), gp.base, size);
        mac = res;
    }

    __uint128_t pack(uint64_t value) {
        return (((__uint128_t)value)) ;
    }
};

// This function checks if the GCD of clause a's polynomial and clause b's converse polynomial is equal to the pivot's polynomial.
inline void check_non_tautological_resolution(clause& a, clause& b, clause& pivot) {
    vector<block> zero_coeff{zero_block, zero_block, zero_block};
    polynomial zero_p = polynomial(zero_coeff, 3);
    GF2EX GF2EX_a = get_GF2EX_with_roots(a.literals);
    vector<uint64_t> neg_b_lits;
    for (auto lit: b.literals){
        neg_b_lits.push_back(get_negate(lit));
    }
    GF2EX GF2EX_neg_b = get_GF2EX_with_roots(neg_b_lits);
    //void XGCD(GF2EX& d, GF2EX& s, GF2EX& t, const GF2EX& a, const GF2EX& b);
    // d = gcd(a,b), a s + b t = d 
    GF2EX res_gcd, res_s, res_t;
    polynomial res_gcd_poly, res_s_poly, res_t_poly, neg_b;
    XGCD(res_gcd, res_s, res_t, GF2EX_a, GF2EX_neg_b);
    GF2EX2polynomial(res_gcd, res_gcd_poly, a.poly.deg);
    GF2EX2polynomial(res_s, res_s_poly, a.poly.deg);
    GF2EX2polynomial(res_t, res_t_poly, a.poly.deg);
    GF2EX2polynomial(GF2EX_neg_b, neg_b, a.poly.deg);
    assert(deg(res_t) <= a.poly.deg);
    assert(deg(res_s) <= a.poly.deg);
    zero_p.GCDCheck(res_gcd_poly, res_s_poly, res_t_poly, a.poly, neg_b);
    neg_b.ConverseCheck(b.poly);
    pivot.poly.Equal(res_gcd_poly);
}

inline void check_qres(clause& a, clause& b, clause& res, clause& pivot, clause& neg_pivot, int& degree) {
    //TODO: Does this weakened resolution check work here? need to prove it remains sound as long as there are no tautologies
    int deg = a.literals.size();
    assert(deg == b.literals.size());

    //This checks weakened resolution
    vector<polynomial> witness = witness_generator(a, b, res, pivot, neg_pivot, 2*deg);
    vector<block> zero_coeff{zero_block, zero_block, zero_block};

    polynomial zero_p = polynomial(zero_coeff, 3);
    vector<polynomial> a_res {a.poly, res.poly};
    vector<polynomial> b_res {b.poly, res.poly};
    vector<polynomial> witness_pivot{witness[0], pivot.poly};
    vector<polynomial> witness_neg_pivot{witness[1], neg_pivot.poly};
    zero_p.InnerProductEqual(a_res, witness_pivot);
    zero_p.InnerProductEqual(b_res, witness_neg_pivot);
    neg_pivot.poly.ConverseCheck(pivot.poly);
    vector<uint64_t> empty_lits;
    padding(empty_lits, 1);
    clause empty_clause(empty_lits, 2);
    check_non_tautological_resolution(res, res, empty_clause);
}

inline polynomial prove_clausea_subset_clauseb(clause& a, clause& b) {
    GF2EX w;
    GF2EX GF2EX_a = get_GF2EX_with_roots(a.literals);
    GF2EX GF2EX_b = get_GF2EX_with_roots(b.literals);

    int party = ostriple->party;
    if (party == ALICE) {
        assert(divide(w, GF2EX_b, GF2EX_a));
    }

    polynomial quotient;
    GF2EX2polynomial(w, quotient, b.poly.deg);
    return quotient;
}

/*This function checks if the forall-reduction step was valid, and if there are any tautologies in the 
*raw resolved clause. 
*/
inline pair<double, double> check_forall_reduction(clause& a, clause& reduced_a, clause& removed_from_a, ROZKRAM<BoolIO<NetIO>>* quantifier_list, bool last_clause) {
    double cost_access = 0;
    double cost_reduction = 0;

    auto timer_0 = chrono::high_resolution_clock::now();
    vector<block> zero_coeff{zero_block, zero_block, zero_block};
    polynomial zero_p = polynomial(zero_coeff, reduced_a.poly.deg);
    vector<polynomial> result;
    vector<polynomial> inputs;
    result.push_back(a.poly);
    inputs.push_back(reduced_a.poly);
    inputs.push_back(removed_from_a.poly);
    zero_p.ProdOfPolysEqual(result, inputs);
    vector<uint64_t> superset_of_lits_removed;
    vector<uint64_t> superset_of_lits_in_reduced_a;

    //Alice gets a superset of literals in removed_from_a
    for (uint64_t lit: removed_from_a.literals){
        if (lit == 0UL) {
            superset_of_lits_removed.push_back(pad_lit);
            continue;
        }
        superset_of_lits_removed.push_back(lit);
    }
    clause superset_of_lits_removed_clause(superset_of_lits_removed, removed_from_a.poly.deg+1);
    vector<block> block_superset_of_lits_removed;
    vector<block> mac_block_superset_of_lits_removed;
    commit_and_prove_literals(superset_of_lits_removed_clause, block_superset_of_lits_removed, mac_block_superset_of_lits_removed);
    vector<Integer> Literals_superset_of_lits_removed;
    get_Integers_consistent_with_committed_blocks(block_superset_of_lits_removed, mac_block_superset_of_lits_removed, Literals_superset_of_lits_removed);
    block temp = (block)((__uint128_t)(nvars));
    Integer min_in_removed = Integer(128, &temp, ALICE);
    Integer forall_quantifier = Integer(4, 2, PUBLIC);
    for (int i = 0; i < Literals_superset_of_lits_removed.size(); i++){
        Integer Index_of_this_lit = index_retriever_Integer&Literals_superset_of_lits_removed[i];
        auto timer_2 = chrono::high_resolution_clock::now();
        if (!(quantifier_list->read(Index_of_this_lit)).geq(forall_quantifier).reveal())    error("Cheat! Removed an existentially quantified literal.\n");
        auto timer_3 = chrono::high_resolution_clock::now();
        cost_access += chrono::duration<double>(timer_3 - timer_2).count();
        if (!(Index_of_this_lit.geq(min_in_removed)).reveal())    min_in_removed = Index_of_this_lit;
    }
    //Alice gets a superset of literals in reduced_a
    for (uint64_t lit: reduced_a.literals){
        if (lit == 0UL) {
            superset_of_lits_in_reduced_a.push_back(pad_lit); //This will always execute at least once
            continue;
        }
        superset_of_lits_in_reduced_a.push_back(lit);
    }
    clause superset_of_lits_in_reduced_a_clause(superset_of_lits_in_reduced_a, reduced_a.poly.deg+1);
    vector<block> block_superset_of_lits_in_reduced_a;
    vector<block> mac_block_superset_of_lits_in_reduced_a;
    commit_and_prove_literals(superset_of_lits_in_reduced_a_clause, block_superset_of_lits_in_reduced_a, mac_block_superset_of_lits_in_reduced_a);
    vector<Integer> Literals_superset_of_lits_in_reduced_a;
    get_Integers_consistent_with_committed_blocks(block_superset_of_lits_in_reduced_a, mac_block_superset_of_lits_in_reduced_a, Literals_superset_of_lits_in_reduced_a);
    temp = zero_block;
    Integer max_in_reduced_a = Integer(128, &temp, ALICE);
    Integer max2_in_reduced_a = Integer(128, &temp, ALICE);
    int index_of_max2_in_reduced_a = 0;
    int index_of_max_in_reduced_a = 0;
    for (int i = 0; i < Literals_superset_of_lits_in_reduced_a.size(); i++){
        Integer Index_of_this_lit = index_retriever_Integer&Literals_superset_of_lits_in_reduced_a[i];
        //cout << "Index_of_this_lit: " << (block)(__uint128_t) Index_of_this_lit.reveal<uint64_t>(ALICE) << endl;
        if (!(max_in_reduced_a.geq(Index_of_this_lit).reveal())) {
            max2_in_reduced_a = max_in_reduced_a;
            max_in_reduced_a = Index_of_this_lit;
            index_of_max2_in_reduced_a = index_of_max_in_reduced_a;
            index_of_max_in_reduced_a = i;
        }
    }
    auto timer_2 = chrono::high_resolution_clock::now();
    assert(quantifier_list->read(max_in_reduced_a).equal(Integer(4, 3, PUBLIC)).reveal());
    auto timer_3 = chrono::high_resolution_clock::now();
    cost_access += chrono::duration<double>(timer_3 - timer_2).count();

    // cout << "min_in_removed: " << (block)(__uint128_t) min_in_removed.reveal<uint64_t>(ALICE) << endl;
    // cout << "max_in_reduced_a: " << (block)(__uint128_t) max_in_reduced_a.reveal<uint64_t>(ALICE) << endl;
    // cout << "max2_in_reduced_a: " << (block)(__uint128_t) max2_in_reduced_a.reveal<uint64_t>(ALICE) << endl;
    
    //assert min_in_removed > max2_in_reduced_a
    assert(!(max2_in_reduced_a.geq(min_in_removed)).reveal());

    //Assert that the literal with the max_in_reduced_a index is existentially quantified.
    //Note: We should skip this check if we are checking the last clause because the last
    //clause can be empty (it will be empty for correct proofs of unsat. This is checked later)
    if (!last_clause) {
        auto timer_2 = chrono::high_resolution_clock::now();
        if ((!(quantifier_list->read(max2_in_reduced_a).equal(Integer(4, 1, PUBLIC))).reveal())) error("Cheat! Highest indexed literal in the reduced clause is not existentially quantified.\n");
        auto timer_3 = chrono::high_resolution_clock::now();
        cost_access += chrono::duration<double>(timer_3 - timer_2).count();
        vector<uint64_t> highest_existential_lit;
        highest_existential_lit.push_back(Literals_superset_of_lits_in_reduced_a[index_of_max2_in_reduced_a].reveal<uint64_t>(ALICE));
        clause highest_existential(highest_existential_lit, 2);
        vector<block> block_highest_existential;
        vector<block> mac_block_highest_existential;
        commit_and_prove_literals(highest_existential, block_highest_existential, mac_block_highest_existential);
        vector<Integer> Literals_highest_existential;
        get_Integers_consistent_with_committed_blocks(block_highest_existential, mac_block_highest_existential, Literals_highest_existential);
        Integer index_of_highest_existential = index_retriever_Integer&Literals_highest_existential[0];
        assert(index_of_highest_existential.equal(max2_in_reduced_a).reveal());
        polynomial w3 = prove_clausea_subset_clauseb(highest_existential, reduced_a);
        reduced_a.poly.ProductEqual(highest_existential.poly, w3);
    }
    assert(superset_of_lits_removed_clause.literals.size() == removed_from_a.poly.deg);
    polynomial w1 = prove_clausea_subset_clauseb(reduced_a, superset_of_lits_in_reduced_a_clause);
    superset_of_lits_in_reduced_a_clause.poly.ProductEqual(reduced_a.poly, w1);
    polynomial w2 = prove_clausea_subset_clauseb(removed_from_a, superset_of_lits_removed_clause);
    superset_of_lits_removed_clause.poly.ProductEqual(removed_from_a.poly, w2);
    auto timer_1 = chrono::high_resolution_clock::now();
    cost_reduction = chrono::duration<double>(timer_1 - timer_0).count();
    cost_reduction -= cost_access;
    return pair<double, double>{cost_access, cost_reduction};
}


inline vector<double> check_chain(vector<Integer>& indice, vector<uint64_t> pivots, int ptr, clauseRAM<BoolIO<NetIO>>* reduced_formula, vector<int64_t>& removed_literals, ROZKRAM<BoolIO<NetIO>>* quantifier_list, bool last_clause, int deg){
    double cost_resolve = 0;
    double cost_access = 0;
    double cost_forallred = 0;
    double cost_quantifier_list_access = 0;
    auto timer_0 = chrono::high_resolution_clock::now();
    clause raw_resolvent;
    Integer Ptr = Integer(INDEX_SZ, ptr, PUBLIC);
    clause reduced_resolvent = reduced_formula->get(Ptr);
    
    vector<clause> resource;
    for (Integer index : indice){
        if (index.geq(Ptr).reveal())  error("cheat!");
        resource.push_back(reduced_formula->get(index));
    }

    auto timer_1 = chrono::high_resolution_clock::now();

    cost_access = chrono::duration<double>(timer_1 - timer_0).count();

    //clause r = removed->get(Ptr);
    vector<uint64_t> rem_lits;
    for (int64_t lit: removed_literals) {
        rem_lits.push_back(wrap(lit));
    }
    padding(rem_lits, reduced_resolvent.poly.deg);
    clause r(rem_lits, reduced_resolvent.poly.deg);

    auto timer_2 = chrono::high_resolution_clock::now();
    assert(resource.size() <= 2);
    if (resource.size() == 2) {
        clause a = resource[0];

        clause b = resource[1];

        raw_resolvent = get_res_f2k(a, b, pivots[1], deg);

        //This checks if the pivot variable is existentially quantified
        vector<uint64_t> pivot_v{pivots[1]};
        vector<uint64_t> neg_pivot_v{get_negate(pivots[1])};
        padding(pivot_v, 1);
        padding(neg_pivot_v, 1);
        clause pivot_clause(pivot_v, 2);
        clause neg_pivot_clause(neg_pivot_v, 2);
        vector<block> pivots_list;
        vector<block> pivots_list_mac;
        vector<block> neg_pivots_list;
        vector<block> neg_pivots_list_mac;
        commit_and_prove_literals(pivot_clause, pivots_list, pivots_list_mac);
        commit_and_prove_literals(neg_pivot_clause, neg_pivots_list, neg_pivots_list_mac);
        assert(pivots_list.size() == 1);
        assert(neg_pivots_list.size() == 1);
        vector<Integer> pivot_Integer;
        vector<Integer> neg_pivot_Integer;
        get_Integers_consistent_with_committed_blocks(pivots_list, pivots_list_mac, pivot_Integer);
        get_Integers_consistent_with_committed_blocks(neg_pivots_list, neg_pivots_list_mac, neg_pivot_Integer);
        Integer index_of_pivot = index_retriever_Integer&pivot_Integer[0];
        Integer index_of_neg_pivot = index_retriever_Integer&neg_pivot_Integer[0];
        assert(index_of_pivot.equal(index_of_neg_pivot).reveal());
        auto timer_2 = chrono::high_resolution_clock::now();
        if (!(quantifier_list->read(index_of_pivot)).equal(Integer(4, 1, PUBLIC)).reveal()) error("Cheat! Pivot is not an existentially quantified literal.\n");
        auto timer_3 = chrono::high_resolution_clock::now();
        cost_quantifier_list_access += chrono::duration<double>(timer_3 - timer_2).count();
        check_qres(a, b, raw_resolvent, pivot_clause, neg_pivot_clause, deg);
        
        auto cost = check_forall_reduction(raw_resolvent, reduced_resolvent, r, quantifier_list, last_clause);
    
        cost_quantifier_list_access += cost.first;
        cost_forallred += cost.second;
    }
    // This should not be necessary because we require that all Q-Resolution steps are 
    // followed by a universal reduction step, so (assuming all the input clauses are 
    // universal reduced,) no clause should need a universal reduction in this isolated
    // fashion
    else if (resource.size() == 1) {
        error("Cheat! Only one clause in the resolution step. We don't allow for purely forall reduction lines.\n");
    }
    else {
        error ("Cheat! Invalid number of supports");
    }
    if (last_clause) {
        vector <uint64_t> empty_literals;
        clause empty_clause(empty_literals, deg);
        reduced_resolvent.poly.Equal(empty_clause.poly);
    }
    auto timer_3 = chrono::high_resolution_clock::now();
    cost_resolve = chrono::duration<double>(timer_3 - timer_2).count();
    vector<double> costs;
    costs.push_back(cost_access);
    costs.push_back(cost_forallred);
    costs.push_back(cost_resolve - cost_quantifier_list_access - cost_forallred);
    costs.push_back(cost_quantifier_list_access);
    return costs;
}

/*
using claim 3.1 of : https://eprint.iacr.org/2020/315.pdf, assuming sub and sup are both sorted; 
*/
inline void assert_f_submultiset_t(vector<Integer>& f, vector<Integer>& t){
    io->flush();
    block gamma = io->get_hash_block();
    block beta = io->get_hash_block();
   
   // evaluate at  F 
    vector<block> f_product; 
    vector<block> f_mproduct;

    for (int i = 0; i < f.size(); i++){
        block tmp; 
        block xx, mx; 
        pack(tmp, f[i], f[i].size());
        xx = (block)get_128uint_from_uint64(f[i].reveal<uint64_t>(ALICE));
       //  cout << "=======5 =======\n";
        check_MAC_valid(xx, tmp);
        if (ostriple -> party == ALICE){
            xx = gamma ^ xx; 
            mx = tmp;
        }
        if (ostriple -> party == BOB){
            gfmul(ostriple->delta, gamma, &mx);
            mx  = tmp^mx;
        }
        f_product.push_back(xx);
        f_mproduct.push_back(mx);
       //  cout << "=======4 =======\n";

        check_MAC_valid(xx, mx);  
    }

    for (int i = 0; i < t.size() -1; i++){
        block t0, t1, mt0, mt1; 
        pack(mt0, t[i], t[i].size());
        pack(mt1, t[i + 1], t[i+1].size());
        t0 = (block)get_128uint_from_uint64(t[i].reveal<uint64_t>(ALICE));
        t1 = (block)get_128uint_from_uint64(t[i+1].reveal<uint64_t>(ALICE));
       //  cout << "======= 6 =======\n";
  
        check_MAC_valid(t0, mt0);
        check_MAC_valid(t1, mt1);

        block xx, mx;  
        gfmul(beta, t1 , &xx);
        gfmul(beta, mt1 , &mx);
        //  cout << "=======8 =======\n";
        check_MAC_valid(xx, mx);
        mx = mx ^ mt0;
        xx = xx ^ t0;  
        block yy; 
        gfmul(gamma, one_block^beta, &yy);
        if (ostriple -> party == ALICE){
            xx = yy ^ xx; 
        }
        if (ostriple -> party == BOB){
            block tmp; 
            gfmul(ostriple->delta, yy, &tmp);
            mx = tmp ^ mx; 
        }
        f_product.push_back(xx);
        f_mproduct.push_back(mx);
       //  cout << "======= 3 =======\n";

        check_MAC_valid(xx, mx);
     
    }

    vector<uint64_t> _s;
    vector<block> s;
    vector<block> ms;
    for(int i = 0; i < f.size(); i ++) _s.push_back((f[i].reveal<uint64_t>(ALICE)));
    for(int i = 0; i < t.size(); i ++) _s.push_back((t[i].reveal<uint64_t>(ALICE)));
    sort(_s.begin(), _s.end());
    for (int i = 0; i < _s.size(); i++){
        //cout << (_s[i]) << endl; 

        block m,d; 
        fill_data_and_mac(d, m);
        block xx, mx; 
        xx = (block)get_128uint_from_uint64(_s[i]);
        if (ostriple->party == ALICE) {
            block diff_root;
            diff_root= d^(xx);
            ostriple->io ->send_data(&diff_root, sizeof(block));
            mx = (m);
        }
        if (ostriple->party == BOB){
            block tmp;
            ostriple->io ->recv_data(&tmp, sizeof(block));
            gfmul(ostriple->delta, tmp, &tmp);
            xx = (d);
            mx = (m ^tmp);
        }
        s.push_back(xx);
        ms.push_back(mx);
    //  cout << "======= 2 =======\n";

        check_MAC_valid(xx, mx);
    }

    

    vector<block> g_product; 
    vector<block> g_mproduct;


    for (int i = 0; i < s.size()-1; i ++) {
        block t0, t1, mt0, mt1; 
        t0 = s[i];
        t1 = s[i+1];
        mt0 = ms[i]; 
        mt1 = ms[i+1];
        
        block xx, mx;  
        gfmul(beta, t1 , &xx);
        gfmul(beta, mt1 , &mx);
        mx = mx ^ mt0;
        xx = xx ^ t0;  
        
        block yy; 
        gfmul(gamma, one_block^beta, &yy);

        if (ostriple -> party == ALICE){
            xx = yy ^ xx; 
            g_product.push_back(xx);
            g_mproduct.push_back(mx);
        }
        if (ostriple -> party == BOB){
            block tmp; 
            gfmul(ostriple->delta, yy, &tmp);
            mx = tmp ^ mx; 
            g_product.push_back(xx);
            g_mproduct.push_back(mx);
        }
       //  cout << "=======1 =======\n";
        check_MAC_valid(xx, mx);  
     
    }
    block lres, mlres, rres, mrres; 
    lres = f_product[0];
    mlres = f_mproduct[0];
    for (int  i = 1; i < f_product.size(); i ++){
        ostriple->compute_mul(lres, mlres, lres, mlres, f_product[i], f_mproduct[i]);
    }  
 
    block overhead = one_block; 
    for (int i = 0; i < f.size(); i ++) gfmul(overhead, one_block^beta, &overhead);
    gfmul(overhead, lres, &lres);
    gfmul(overhead, mlres, &mlres);
    check_MAC_valid(lres, mlres);    

    rres = g_product[0];
    mrres = g_mproduct[0];
    for (int  i = 1; i < g_product.size(); i ++){
        ostriple->compute_mul(rres, mrres, rres, mrres, g_product[i], g_mproduct[i]);
    }
    check_MAC_valid(rres, mrres);    
    check_zero_MAC(mrres ^ mlres);
}


inline clause get_clause(const vector<Integer>& roots){
    vector<uint64_t> roots_raw;
    for(int i = 0; i < roots.size(); i++){
        roots_raw.push_back(roots[i].reveal<uint64_t>(ALICE));
    }
    return clause(roots_raw, roots.size()+1);
}


inline void assert_set_equal(vector<Integer>& s0, vector<Integer>& s1) {
    assert(s0.size() == s1.size());
    clause c0 = get_clause(s0);
    clause c1 = get_clause(s1); 
    vector<block> literals0;
    vector<block> mac_literals0;    
    vector<block> literals1;
    vector<block> mac_literals1;
    commit_and_prove_literals(c0, literals0, mac_literals0);
    commit_and_prove_literals(c1, literals1, mac_literals1);
    vector<Integer> Integer_lits_in_c0;
    vector<Integer> Integer_lits_in_c1;
    get_Integers_consistent_with_committed_blocks(literals0, mac_literals0, Integer_lits_in_c0);
    get_Integers_consistent_with_committed_blocks(literals1, mac_literals1, Integer_lits_in_c1);
    for (int i = 0; i < s0.size(); i++) {
        if (!(s0[i].equal(Integer_lits_in_c0[i]).reveal())) {
            error("Cheat! Clauses do not represent the Integers passed.\n");
        }
        if (!(s1[i].equal(Integer_lits_in_c1[i]).reveal())) {
            error("Cheat! Clauses do not represent the Integers passed.\n");
        }
    }
    c0.poly.Equal(c1.poly);
}

// f_sorted and t_sorted are sorted versions of f and t respectively
// ensure that f_sorted and t_sorted are empty when called
inline void sort_Integers(vector<Integer>& f, vector<Integer>& t, vector<Integer>& f_sorted, vector<Integer>& t_sorted) {
    vector<uint64_t> _f_sorted;
    for (int i = 0; i < f.size(); i++) _f_sorted.push_back(f[i].reveal<uint64_t>(ALICE));

    vector<uint64_t> _t_sorted;
    for (int i = 0; i < t.size(); i++) _t_sorted.push_back(t[i].reveal<uint64_t>(ALICE));
    
    sort(_f_sorted.begin(), _f_sorted.end());
    sort(_t_sorted.begin(), _t_sorted.end());  
  
    for (int i = 0; i < _t_sorted.size(); i++)  t_sorted.push_back(Integer(128, _t_sorted[i], ALICE));
    for (int i = 0; i < _f_sorted.size(); i++)  f_sorted.push_back(Integer(128, _f_sorted[i], ALICE));
  
    assert_set_equal(f, f_sorted);
    assert_set_equal(t, t_sorted);
}


/**************************checking*************************************************/

/**************************bottleneck*************************************************/






#endif //ZKUNSAT_NEW_CLAUSERAM_H
