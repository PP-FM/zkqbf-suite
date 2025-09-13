//
// Created by anonymized on 12/7/21.
//

#ifndef ZKUNSAT_NEW_CLAUSE_H
#define ZKUNSAT_NEW_CLAUSE_H

#define INDEX_SZ 20
#include "polynomial.h"

inline uint64_t  get_negate(uint64_t encode){
    if (encode == 0) return encode;
    return constant^encode;
}


class clause {
public:
    polynomial poly;
    vector<uint64_t> literals;
    clause(){
    }


    clause(vector<uint64_t> & ells, int deg){
        assert(!(ells.size() > deg));
        this->poly = polynomial(ells, deg);
        this->literals = ells;
    }

    void get_literals(vector<Integer>& lts) const{
        assert(lts.size() == 0);
        for(int i = 0; i < this->literals.size(); i ++){
            lts.push_back(Integer(VAL_SZ, literals[i], ALICE));
        }
    }

    void print() const{
        for (auto ell : this->literals){
            cout << ell << ",";
        }
        cout << endl;
   }
};


inline std::vector<polynomial> witness_generator(clause& a, clause& b, clause& res, clause& pivot, clause& neg_pivot, int deg) {
    GF2EX ap, bp, resp;
    GF2EX w0, w1;

    ap = get_GF2EX_with_roots(a.literals);
    bp = get_GF2EX_with_roots(b.literals);
    resp = get_GF2EX_with_roots(res.literals);
    GF2EX pivot_p = get_GF2EX_with_roots(pivot.literals);
    GF2EX neg_pivot_p = get_GF2EX_with_roots(neg_pivot.literals);

    int party = ostriple->party;
    if (party == ALICE) {
        assert(divide(w0, resp*pivot_p, ap));
        assert(divide(w1, resp*neg_pivot_p, bp));
    }
    polynomial res0, res1;
    GF2EX2polynomial(w0, res0, deg);
    GF2EX2polynomial(w1, res1, deg);
    std::vector<polynomial> r{res0, res1};
    return  r;
}

inline clause get_res_f2k(const clause& a,  const clause& b, uint64_t pivot, int DEGREE){
    uint64_t  npivot  = get_negate(pivot);

    std::vector<uint64_t> altr = a.literals;
    std::vector<uint64_t> bltr = b.literals;
    vector<uint64_t> res_raw;
    std::set<uint64_t> res_l;

    for (auto e: altr) {
        if (e == pivot or e == 0) continue;
            res_l.insert(e);
    }

    for (auto e: bltr) {
        if (e == npivot or e == 0) continue;
        res_l.insert(e);
    }

    for (auto l: res_l) {
        res_raw.push_back(l);
    }

    if (res_raw.size() > DEGREE) {
        cout << res_raw.size() << endl;
        for (auto i : res_raw) cout << i << " ";
        cout << endl;
        cout <<"overflow error!" << endl;
    }
    padding(res_raw, DEGREE);
    assert(res_raw.size() == DEGREE);
    clause c(res_raw, DEGREE);
    return c;
}

inline void prove_literals_clause_relation(clause& a, vector<block>& literals, vector<block>& mac_literals) {
    io->flush();
    block r = io->get_hash_block();
    block r_mac;
    block res = one_block;
    block mres;
    if (ostriple->party == ALICE) {
        r_mac = zero_block;
        mres = zero_block;
    } else {
        r_mac = ostriple->delta;
        mres = ostriple->delta;
        gfmul(r_mac, r, &r_mac);
    }
    assert(literals.size() == mac_literals.size());
    for (int i = 0; i < literals.size(); i++) {
        block temp_factor, mac_temp_factor;
        compute_xor(temp_factor, mac_temp_factor, r, r_mac, literals[i], mac_literals[i]);
        ostriple->compute_mul(res, mres, res, mres, temp_factor, mac_temp_factor);
    }
    block ares, amres;
    a.poly.Evaluate(ares, amres, r);
    check_zero_MAC(amres^mres);
}

// literals must not be padded with 0UL.
inline void commit_and_prove_literals(clause &a, vector<block>& literals, vector<block>& mac_literals) {
    for (uint64_t lit: a.literals) {
        block d, m;
        block lit_block = (block) get_128uint_from_uint64(lit);
        fill_data_and_mac(d, m);

        if (ostriple->party == ALICE) {
            block diff_lit;
            diff_lit= d^lit_block;
            ostriple->io ->send_data(&diff_lit, sizeof(block));
            literals.push_back(lit_block);
            mac_literals.push_back(m);
        }
        if (ostriple->party == BOB){
            block diff_lit;
            ostriple->io ->recv_data(&diff_lit, sizeof(block));
            gfmul(ostriple->delta, diff_lit, &diff_lit);
            literals.push_back(d);
            mac_literals.push_back(m ^diff_lit);
        }
    }
    prove_literals_clause_relation(a, literals, mac_literals);
}



#endif //ZKUNSAT_NEW_CLAUSE_H
