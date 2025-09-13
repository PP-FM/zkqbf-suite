//
// Created by anonymized on 12/7/21.
//

#ifndef ZKUNSAT_NEW_CLAUSE_H
#define ZKUNSAT_NEW_CLAUSE_H

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


inline std::vector<polynomial> witness_generator(clause& a, clause& b, clause& res, uint64_t& pivot, int deg) {
    GF2EX ap, bp, resp;
    GF2EX w0, w1;
    vector<uint64_t> pv{pivot};

    uint64_t  neg_pivot = get_negate(pivot);
    vector<uint64_t> npv{neg_pivot};

    ap = get_GF2EX_with_roots(a.literals);
    bp = get_GF2EX_with_roots(b.literals);
    resp = get_GF2EX_with_roots(res.literals);
    GF2EX pivot_p = get_GF2EX_with_roots(pv);
    GF2EX neg_pivot_p = get_GF2EX_with_roots(npv);

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

inline void check_xres(clause& c0, clause& c1, clause cres, uint64_t pivot){
    int deg;
    if (c0.literals.size() < c1.literals.size()){
        deg = c1.literals.size();
        if (c1.literals.size() < cres.literals.size()){
            deg = cres.literals.size();
        }
    }
    else {
        deg = c0.literals.size();
        if (c0.literals.size() < cres.literals.size()){
            deg = cres.literals.size();
        }
    }

    vector<polynomial> witness = witness_generator(c0, c1, cres, pivot, deg);
    vector<block> zero_coeff{zero_block, zero_block, zero_block};
    vector<uint64_t> pivot_v{pivot};
    vector<uint64_t> neg_pivot_v{get_negate(pivot)};

    polynomial zero_p = polynomial(zero_coeff, cres.poly.deg);
    polynomial pivot_polynomial(pivot_v, 3);
    polynomial pivot_neg_polynomial(neg_pivot_v, 3);
    vector<polynomial> c0_res {c0.poly, cres.poly };
    vector<polynomial> c1_res {c1.poly, cres.poly };
    vector<polynomial> witness_pivot{witness[0], pivot_polynomial};
    vector<polynomial> witness_neg_pivot{witness[1], pivot_neg_polynomial};
    zero_p.InnerProductEqual(c0_res, witness_pivot);
    zero_p.InnerProductEqual(c1_res, witness_neg_pivot);
    pivot_neg_polynomial.ConverseCheck(pivot_polynomial);
}


inline clause get_res_f2k(const clause& a,  const clause& b, uint64_t pivot, int deg){
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

    if (res_raw.size() > deg) {
        cout << res_raw.size() << endl;
        for (auto i : res_raw) cout << i << " ";
        cout << endl;
        cout <<"overflow error!" << endl;
    }
    padding(res_raw, deg);
    assert(res_raw.size() == deg);
    clause c(res_raw, deg);
    return c;
}





#endif //ZKUNSAT_NEW_CLAUSE_H
