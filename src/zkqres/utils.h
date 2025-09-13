//
// Created by anonymized on 12/7/21.
//

#ifndef ZKUNSAT_NEW_UTILS_H
#define ZKUNSAT_NEW_UTILS_H

//
// Created by anonymized on 8/9/21.
//
#pragma  once

#include "commons.h"

using namespace  std;
using  namespace  NTL;
using namespace emp;


#define VAL_SZ 61

#define RAM_VAL_SZ 4

inline void gfmul_test (__m128i a, __m128i b, __m128i *res) {
		__m128i  tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9;

		// mul128(a, b, &tmp3, &tmp6);
		tmp3 = _mm_clmulepi64_si128(a, b, 0x00); 
        tmp4 = _mm_clmulepi64_si128(a, b, 0x10); 
        tmp5 = _mm_clmulepi64_si128(a, b, 0x01); 
        tmp6 = _mm_clmulepi64_si128(a, b, 0x11); 

        tmp4 = _mm_xor_si128(tmp4, tmp5); 
        tmp5 = _mm_slli_si128(tmp4, 8); 
		tmp4 = _mm_srli_si128(tmp4, 8); 
        tmp3 = _mm_xor_si128(tmp3, tmp5); 
        tmp6 = _mm_xor_si128(tmp6, tmp4); 
        
        
        tmp7 = _mm_srli_epi32(tmp3, 31);
		tmp8 = _mm_srli_epi32(tmp6, 31);
		tmp3 = _mm_slli_epi32(tmp3, 1);
		tmp6 = _mm_slli_epi32(tmp6, 1);

		tmp9 = _mm_srli_si128(tmp7, 12);
		tmp8 = _mm_slli_si128(tmp8, 4);
		tmp7 = _mm_slli_si128(tmp7, 4);
		tmp3 = _mm_or_si128(tmp3, tmp7);
		tmp6 = _mm_or_si128(tmp6, tmp8);
		tmp6 = _mm_or_si128(tmp6, tmp9);

		tmp7 = _mm_slli_epi32(tmp3, 31);
		tmp8 = _mm_slli_epi32(tmp3, 30);
		tmp9 = _mm_slli_epi32(tmp3, 25);
		
        tmp7 = _mm_xor_si128(tmp7, tmp8);
		tmp7 = _mm_xor_si128(tmp7, tmp9);
		tmp8 = _mm_srli_si128(tmp7, 4);
		tmp7 = _mm_slli_si128(tmp7, 12);
		tmp3 = _mm_xor_si128(tmp3, tmp7);

		tmp2 = _mm_srli_epi32(tmp3, 1);
		tmp4 = _mm_srli_epi32(tmp3, 2);
		tmp5 = _mm_srli_epi32(tmp3, 7);
		tmp2 = _mm_xor_si128(tmp2, tmp4);
		tmp2 = _mm_xor_si128(tmp2, tmp5);
		tmp2 = _mm_xor_si128(tmp2, tmp8);
		tmp3 = _mm_xor_si128(tmp3, tmp2);
        tmp6 = _mm_xor_si128(tmp6, tmp3);
		*res = tmp5;
	}



/* Encode literal: the highest bit indicates the sign of that literal;
 * the rest bits indicate the value of the index;
 * for example: x2 is encoded as [1,0, ..., 0, 1, 0] , and not x2 is encoded as [0,0, ..., 0, 1, 0];
 * [0,0, ..., 0, 0, 0] and [1,0, ..., 0, 0, 0] are dummy bits;
 * used by prover locally;
 */
const block one_block = makeBlock(0, 1);
inline GaloisFieldPacking gp;

inline uint64_t wrap(int64_t input){
    uint64_t  res = 0UL;
    if(input >  0) {
        res = (uint64_t) input | 1UL<<(VAL_SZ-2);
        res = res | 1UL<<(VAL_SZ-1);
    }
    if(input <  0) {
        res = (uint64_t)(-input);
        res = res | 1UL<<(VAL_SZ-1);
    }
    return  res;
}


//get mac of val

inline void pack(block& mac, const Integer& val, int size = VAL_SZ) {
    block res;
    vector_inn_prdt_sum_red(&res, (block*)(val.bits.data()), gp.base, size);
    mac = res;
}


/*
 * multiply a committed value x by cst; mac of x is m;
 * val = m * cst;
 * mac is the mac of val;
 */

inline void multiply_const(block &val, block &mac,
                           const block& x, const block& m, const block& cst, int party) {
                            
    // cout << "=====before ==== \n";
    // cout << (cst) << endl; 
    //  cout << (x) << endl;     
    // cout << (val) << endl;                       
    if(party == ALICE) {
        gfmul(x, cst, &val);//   cout << mac << endl;
    }
    gfmul(m, cst, &mac);
    // cout << "====after ===== \n";
    // cout << (cst) << endl; 
    // cout << (x) << endl;     
    // cout << (val) << endl; 
 }

/*
 * compute addition of vala and valb;
 * valc = vala + valb; maca is mac of a, macb is mac of b;
 * macc = mac(valc)
 */
inline void compute_xor(block &valc, block &macc,
                        const block vala, const block maca, const block valb, const block macb){
    valc = vala ^ valb;
    macc = maca ^ macb;
}


inline __uint128_t get_128uint_from_uint64(uint64_t value) {
    return (((__uint128_t)value)) ;
}


inline block encode(int64_t input){
    uint64_t  res = 0UL;
    if(input >  0) {
        res = (uint64_t) input | 1UL<<(VAL_SZ-2);
        res = res | 1UL<<(VAL_SZ-1);
    }
    if(input <  0) {
        res = (uint64_t)(-input);
        res = res | 1UL<<(VAL_SZ-1);
    }
    return  (block) get_128uint_from_uint64(res) ;
}

inline void check_MAC_valid(block X, block MAC) {
    BoolIO<NetIO>* io = ostriple->io;
    int party = ostriple->party;
    if(party == ALICE) {
        io->send_data(&X, sizeof(block));
        io->send_data(&MAC, sizeof(block));
        int64_t res = -1;
        io-> recv_data(&res, 8);
        if (res != 1) error("check_MAC failed!\n");
    } else {
        block M = zero_block, x = zero_block;
        io->recv_data(&x, sizeof(block));
        io->recv_data(&M, sizeof(block));
        gfmul(x, ostriple->delta, &x);
        x = x ^ MAC;
        if (memcmp(&x, &M, sizeof(block))!=0) {
            int64_t  res = 0;
            io->send_data(&res, 8);
            error("check_MAC failed!\n");
        }else {
            int64_t  res = 1;
            io->send_data(&res, 8);
        }

    }

}

/*
 * check if a block is a mac of zero.
 * It is an accumulator that takes all mac blocks to be check
 * At the end of the proof, prover by setting end = 1 will hash all blocks, and send it to verifier.
*/

inline void check_zero_MAC(block MAC, int end = 0) {
    static Hash hash;
    hash.put(&MAC, sizeof(block));
    if (end == 1){
        BoolIO<NetIO>* io = ostriple->io;
        int party = ostriple->party;
        char dig[Hash::DIGEST_SIZE];
        hash.digest(dig);

        if(party == ALICE) {
            io->send_data(dig, sizeof(dig));
        } else {
            char receive[Hash::DIGEST_SIZE];
            io->recv_data(receive, sizeof(receive));
            if (memcmp(receive, dig, sizeof(dig))!=0) {
                error("check_zero failed!\n");
            }else {
                cout << "check_zero succeed!!!" << endl;
            }
        }
        return;
    }

}


//get Integers that are consistent with a committed vector of blocks.
inline void get_Integers_consistent_with_committed_blocks(vector<block>& b, vector<block>& mb, vector<Integer>& I) {
    for (int i = 0; i < b.size(); i++) {
        I.push_back(Integer(128, &b[i], ALICE));
        block tmp_mac;
        pack(tmp_mac, I[i], 128);
        check_zero_MAC(tmp_mac^mb[i]);
    }
}


inline block get_index_retriever_block(){
    uint64_t index_retriever = 0UL;
    for (uint64_t i = 0; i < VAL_SZ-2; i++){
        index_retriever = index_retriever | (1UL << i);
    }
    block index_retriever_block = (block)((__uint128_t) index_retriever);
    return index_retriever_block;
}


/*
 * padding a vector of int64 to the size of degree
 * used when prover input a clause
 */
inline void padding(vector<uint64_t>& input, int deg, uint64_t pad = 0UL){
    for (int i = input.size() ; i < deg; i ++){
        input.push_back(pad);
    }
}

/*
 * conversion from block to Galois field element of size 128
 * */

inline void block2GF(GF2E& res, const block a ){
    GF2X raw;
    //  cout <<(a) << endl;
    auto low = _mm_extract_epi64(a, 0);
    auto high = _mm_extract_epi64(a, 1);
    // cout << low << endl;
    //cout << high << endl;

    raw.SetMaxLength(128);
    for(int i = 0; i < 64; i++){
        SetCoeff(raw, long(i), (1 &(low >> (i))));
        SetCoeff(raw, long(i+64), (1 &(high >> (i))));
    }
    res._GF2E__rep = raw;
}


/*
 * get inverse of an block in the field. In particular, inv* input = 1 in F_{2^k}
 * */

inline void inverse(block& inv, block& input){
    GF2E _input, _inv;
    block2GF(_input, input );
    NTL::inv(_inv, _input);
    GF2X raw = _inv._GF2E__rep;
    inv = zero_block;
    for (int i = 0; i < 128; i++) {
        if (IsOne(NTL::coeff(raw, i)))
            inv = set_bit(inv, i);
    }
}

inline void fill_data_and_mac(block& d, block& m){
    if (data_mac_pointer == svole->param.n){
        svole->extend_inplace(data, mac, svole->param.n);
        data_mac_pointer = 0;
    }
    d = data[data_mac_pointer];
    m = mac[data_mac_pointer];
    data_mac_pointer = data_mac_pointer + 1;

}

inline GF2EX get_GF2EX_with_roots(vector<uint64_t>& roots){
    GF2EX res, tmp;
    SetCoeff(res, 0); // res = 1
    for (auto r : roots){
        tmp = GF2EX();
        GF2E coefficient, constant;
        if (r == 0){
            block2GF(coefficient, zero_block);
            block2GF(constant, one_block);
            SetCoeff(tmp, 0, constant);
            SetCoeff(tmp, 1, coefficient);
        }else{
            block2GF(constant, (block)get_128uint_from_uint64(r));
            block2GF(coefficient, one_block);
            SetCoeff(tmp, 0, constant);
            SetCoeff(tmp, 1, coefficient);
        }
        res = tmp * res;
    }
    return res;
}

typedef  vector<int64_t> CLS;
typedef vector<int64_t> SPT;
typedef vector<int64_t> PVT;

//NOTE: We are making the assumption that the variables are numbered 1,...,num_vars.
inline void readproof(string filename, int& d, vector<CLS>& reduced_clauses, vector<SPT>& supports, vector<PVT>& pivots, int& ncls, int& nres, vector<CLS>& removed_literals, /*vector<CLS>& literals_list,*/ vector<int64_t>& quantifiers){
    std::ifstream file(filename);
    std::string str;
    ncls = 0;
    nres = 0;
    int64_t nvars = 0;
    d = 0;
    vector<CLS> raw_clauses;
    quantifiers.push_back(0);

    while (std::getline(file, str)) {
        istringstream ss(str);
        string word;
        while (ss >> word) {
            if (word == "a") {
                ss >> word;
                while (word != "0") {
                    int i = stoi(word);
                    quantifiers.push_back(2);
                    nvars += 1;
                    ss >> word;
                }
            }
            if (word == "e") {
                ss >> word;
                while (word != "0") {
                    int i = stoi(word);
                    quantifiers.push_back(1);
                    nvars += 1;
                    ss >> word;
                }
            }
            if (word == "raw_clause:") {
                CLS clause;
                ss >> word;
                while (word != "reduced_clause:") {
                    int i = stoi(word);
                    clause.push_back(i);
                    ss >> word;
                }
                raw_clauses.push_back(clause);
                ncls ++ ;
            }
            if (word == "reduced_clause:") {
                CLS clause;
                ss >> word;
                while (word != "support:") {
                    int i = stoi(word);
                    clause.push_back(i);
                    ss >> word;
                }
                reduced_clauses.push_back(clause);
            }
            if (word == "support:") {
                SPT support;
                ss >> word;
                while (word != "pivot:") {
                    int i = stoi(word);
                    support.push_back(i);
                    ss >> word;
                }
                if (support.size() > 0) {
                    nres += 1;
                }
                supports.push_back(support);
            }
            if (word == "pivot:") {
                SPT pchain;
                ss >> word;
                while (word != "removed:") {
                    int i = stoi(word);
                    pchain.push_back(wrap(i));
                    ss >> word;
//                    nres = nres + 1;
                }
                pivots.push_back(pchain);
            }
            if (word == "removed:") {
                CLS removed_from_raw;
                ss >> word;
                while (word != "end:") {
                    int i = stoi(word);
                    removed_from_raw.push_back(i);
                    ss >> word;
                }
                removed_literals.push_back(removed_from_raw);
            }
            if (word == "DEGREE:"){
                ss >> word;
                d = stoi(word);
            }
        }
    }
    quantifiers.push_back(3);
}

#endif //ZKUNSAT_NEW_UTILS_H
