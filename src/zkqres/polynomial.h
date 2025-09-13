//
// Created by anonymized on 12/7/21.
//
#pragma once
#ifndef ZKUNSAT_NEW_POLYNOMIAL_H
#define ZKUNSAT_NEW_POLYNOMIAL_H
#include "utils.h"
#include "commons.h"

class polynomial {
public:
    vector <block> coefficient;
    vector <block> mcoefficient;
    int deg;
    polynomial(){
    }
    polynomial(vector<block> coefficient, int deg);
    polynomial(vector<uint64_t> roots, int deg);
    void Evaluate(block &res, block &mres, block &input) const ;
    void Equal(const polynomial& lfh) const;
    void InnerProductEqual(vector<polynomial>& p1, vector<polynomial>& p2);
    void ProductEqual(polynomial& p1, polynomial& p2);
    void ConverseCheck(polynomial& lhs);
    void NonConverseCheck(polynomial& lhs);
    void ProdOfPolysEqual(vector<polynomial> &p1, vector<polynomial> &p2);
    void GCDCheck(polynomial &gcd, polynomial &s, polynomial &t, polynomial &a, polynomial &b);

    void print() {
        for (block b: coefficient) {
            cout << "[" << (b) << "]";
        }
        cout << endl;
    }
};
inline void GF2EX2polynomial(GF2EX& a, polynomial& b, int degree){

    long d = deg(a);
    assert(!(d > degree));
    std::vector<block> coeff;
    for (long i = 0; i < degree; i ++){
        GF2E c = NTL::coeff(a, i);
        GF2X raw = c._GF2E__rep;
        block tmp = zero_block;
        for(int i = 0; i < 128; i++){
            if (IsOne(NTL::coeff(raw, i)))
                tmp = set_bit(tmp, i);
        }
        coeff.push_back(tmp);
    }
    b = polynomial(coeff, degree);
}



#endif //ZKUNSAT_NEW_POLYNOMIAL_H
