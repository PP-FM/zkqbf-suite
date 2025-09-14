#include <iostream>
#include "clause.h"
#include "clauseRAM.h"
#include "commons.h"

int port, party;
const int threads = 8;
vector<int64_t> quantifiers; // if quantifier[i] == 1, then i^th literal is existential, if quantifier[i] == 2, then i^th literal is universal/forall quantified. 0 and 3 are for the padding literals.
int DEGREE = 4;
int nvars = 0;
block *mac, *data;
uint64_t data_mac_pointer;
SVoleF2k <BoolIO<NetIO>> *svole;
F2kOSTriple <BoolIO<NetIO>> *ostriple;
uint64_t constant;
BoolIO <NetIO> *io;
uint64_t pad_lit;
block index_retriever;
Integer index_retriever_Integer;

int main(int argc, char **argv) {
    index_retriever = get_index_retriever_block();
    cout << "---- begin ----" << endl;
    constant = 1UL << (VAL_SZ - 2);
    parse_party_and_port(argv, &party, &port);
    BoolIO <NetIO> *ios[threads];
    for (int i = 0; i < threads; ++i)
        ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE ? nullptr : argv[3], port + i), party == ALICE);       
    char *prooffile = argv[5];
    char *qbffile = argv[4];

    setup_zk_bool < BoolIO < NetIO >> (ios, threads, party);
    ZKBoolCircExec <BoolIO<NetIO>> *exec = (ZKBoolCircExec < BoolIO < NetIO >> *)(CircuitExecution::circ_exec);
    io = exec->ostriple->io;

    ostriple = new F2kOSTriple <BoolIO<NetIO>>(party, exec->ostriple->threads, exec->ostriple->ios,
                                               exec->ostriple->ferret, exec->ostriple->pool);
    svole = ostriple->svole;

    data_mac_pointer = 0;
    uint64_t test_n = svole->param.n;;
    uint64_t mem_need = svole->byte_memory_need_inplace(test_n);

    data = new block[svole->param.n];
    mac = new block[svole->param.n];
    svole->extend_inplace(data, mac, svole->param.n);
    cout << "----set up----" << endl;

    GF2X P;
//    random(P, 128);
    SetCoeff(P, 128, 1);
    SetCoeff(P, 7, 1);
    SetCoeff(P, 2, 1);
    SetCoeff(P, 1, 1);
    SetCoeff(P, 0, 1);
    GF2E::init(P);


    int ncls = 0, nres = 0;

    vector <CLS> reduced_clauses;
    vector <CLS> raw_clauses;
    vector <SPT> supports;
    vector <SPT> pivots;
    vector <CLS> removed_literals;
    vector <Integer> quantifier_list;
    index_retriever_Integer = Integer(128, &index_retriever, PUBLIC);


    if (party == ALICE) {
        readproof(string(prooffile), DEGREE, reduced_clauses, supports, pivots, ncls, nres, removed_literals, quantifiers, raw_clauses);
        cout << string(prooffile) << endl;
        cout << "----input proof----" << endl;
        io->send_data(&nres, 4);
        io->send_data(&ncls, 4);
        io->send_data(&DEGREE, 4);
        nvars = quantifiers.size();
        pad_lit = wrap(nvars-1);
        io->send_data(&nvars, 4);
    }

    if (party == BOB) {
        //TODO: BOB should read the input formula and verify that the literals_list and quantifiers list are correct.
        io->recv_data(&nres, 4);
        io->recv_data(&ncls, 4);
        io->recv_data(&DEGREE, 4);
        io->recv_data(&nvars, 4);

        reduced_clauses = vector<CLS>(ncls);
        supports = vector<SPT>(ncls);
        pivots = vector<SPT>(ncls);
        removed_literals = vector<CLS>(ncls);
        raw_clauses = vector<CLS>(ncls);
        quantifiers = vector<int64_t>(nvars);
        pad_lit = wrap(nvars/2);
    }

    vector<CLS> original_qbf_CLS;
    readqbffile(string(qbffile), original_qbf_CLS);

    cout << "nres " << nres << endl;
    cout << "ncls " << ncls << endl;
    cout << "DEGREE " << DEGREE << endl;
    cout << "nvars " << nvars << endl;
    //if ( ncls > 524287) return 0; 

    double cost_input = 0;
    double cost_resolve = 0;
    double cost_access = 0 ;
    double cost_forallred = 0;
    double cost_quantifier_list_access = 0;

/*Create ROZKRAM for the list of quantifiers 
*/  
    auto timer_0 = chrono::high_resolution_clock::now();
    vector<clause> literals_as_clauses;
    for (int i = 0; i < nvars; i++){
        quantifier_list.push_back(Integer(4, quantifiers[i], ALICE));
    }
    ROZKRAM<BoolIO<NetIO>>* quantifiers_ROZKRAM = new ROZKRAM<BoolIO<NetIO>>(party, INDEX_SZ, 4);
    quantifiers_ROZKRAM->init(quantifier_list);

/*
 * encode the reduced formula, removed literals
 * */
    vector<clause> reduced_formula;
    vector<clause> raw_cubes;
    vector<clause> removed_lits_as_clauses;
    vector<clause> original_qbf_clauses;

    float delta = 0 ;

    for (int i = 0; i < original_qbf_CLS.size(); i++) {
        vector <uint64_t> literals;
        for (int64_t lit: original_qbf_CLS[i]) {
            literals.push_back((wrap(lit)));
        }
        padding(literals, original_qbf_CLS[i].size()+1);
        clause c(literals, original_qbf_CLS[i].size()+1);
        original_qbf_clauses.push_back(c);
    }


    for (int i = 0; i < ncls; i++) {
        delta = delta + 1;
        if ((delta / ncls) > 0.1){
            float  progress = (float(i) / ncls);
            delta = 0;
            int barWidth = 70;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }


        vector <uint64_t> literals;
        for (int64_t lit: reduced_clauses[i]) {
            literals.push_back((wrap(lit)));
        }
        padding(literals, DEGREE);
        clause c(literals, DEGREE);
        reduced_formula.push_back(c);
        if (i < ncls - nres) {
            vector <uint64_t> literals_raw;
            for (int64_t lit: raw_clauses[i]) {
                literals_raw.push_back((wrap(lit)));
            }
            padding(literals_raw, DEGREE);
            clause raw_cube(literals_raw, DEGREE);
            raw_cubes.push_back(raw_cube);
            vector <uint64_t> literals_removed;
            for (int64_t lit: removed_literals[i]) {
                literals_removed.push_back((wrap(lit)));
            }
            padding(literals_removed, DEGREE);
            clause removed_clause(literals_removed, DEGREE);
            removed_lits_as_clauses.push_back(removed_clause);
        }
    }
    clauseRAM<BoolIO<NetIO>>* reduced_formulaCR = new clauseRAM<BoolIO<NetIO>>(party, INDEX_SZ, DEGREE);
    reduced_formulaCR->init(reduced_formula);
    cout <<"finish  input!\n";
    auto timer_1 = chrono::high_resolution_clock::now();
    cost_input = chrono::duration<double>(timer_1 - timer_0).count();

    delta = 0;

    for (int64_t i = 0; i < ncls - nres; i++) {
        delta = delta + 1;
        if ((delta / (ncls - nres)) > 0.1){
            float  progress = (float(i) / (ncls-nres));
            delta = 0;
            int barWidth = 70;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }
        vector <uint64_t> empty_literals;
        clause input_clause = reduced_formulaCR->get(Integer(INDEX_SZ, i, ALICE));
        padding(empty_literals, input_clause.poly.deg);
        clause empty_clause(empty_literals, input_clause.poly.deg);
        auto cost = check_forall_reduction(raw_cubes[i], input_clause, removed_lits_as_clauses[i], quantifiers_ROZKRAM, false);
        cost_forallred = cost_forallred + cost.second;
        cost_quantifier_list_access = cost_quantifier_list_access + cost.first;
    }
    cout <<"finished checking existential reduction of initial cubes!\n";

    auto timer_for_initial_cube_validity_1 = chrono::high_resolution_clock::now();
    for (int64_t i = 0; i < ncls - nres; i++) {
        delta = delta + 1;
        if ((delta / (ncls - nres)) > 0.1){
            float  progress = (float(i) / (ncls-nres));
            delta = 0;
            int barWidth = 70;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }
        vector<uint64_t> intersection_literals;
        if (party == ALICE) {
            std::unordered_set<uint64_t> set1(raw_cubes[i].literals.begin(), raw_cubes[i].literals.end());
            for (int j = 0; j < original_qbf_clauses.size(); j++) {
                for (const auto& elem : original_qbf_clauses[j].literals) {
                    if (elem == 0) continue;
                    if (set1.find(elem) != set1.end()) {
                        intersection_literals.push_back(elem);
                        break; // We only need one literal from each clause for the intersection
                    }
                }
            }
            assert(intersection_literals.size() == original_qbf_clauses.size());
            assert(intersection_literals.size() != 0);
        }
        else {
            intersection_literals = vector<uint64_t>(original_qbf_clauses.size());
        }
        vector<uint64_t> literals;
        vector<uint64_t> empty_literals;
        clause empty_clause(empty_literals, 2);
        vector<polynomial> intersection_polynomials;
        check_non_tautological_resolution(raw_cubes[i], raw_cubes[i], empty_clause);
        vector<uint64_t> all_lits;
        for (int j = 0; j < intersection_literals.size(); j++) {
            literals.push_back(intersection_literals[j]);
            all_lits.push_back(intersection_literals[j]);
            padding(literals, 2);
            clause c(literals, 3);
            literals.clear();
            prove_clausea_subset_clauseb(c, original_qbf_clauses[j]);
            intersection_polynomials.push_back(c.poly);
        }
        vector<polynomial> all_intersection_lits_polynomial;
        clause all_lits_clause(all_lits, intersection_literals.size() + 1);
        all_intersection_lits_polynomial.push_back(all_lits_clause.poly);
        empty_clause.poly.ProdOfPolysEqual(all_intersection_lits_polynomial, intersection_polynomials);
        vector<block> all_lits_in_intersections;
        vector<block> all_lits_in_intersections_mac;
        commit_and_prove_literals(all_lits_clause, all_lits_in_intersections, all_lits_in_intersections_mac);
        vector<Integer> all_lits_in_intersections_Integer;
        vector<Integer> all_lits_in_intersections_Integer_sorted;
        get_Integers_consistent_with_committed_blocks(all_lits_in_intersections, all_lits_in_intersections_mac, all_lits_in_intersections_Integer);
        vector<block> super_cube_literals;
        vector<block> super_cube_literals_mac;
        vector<uint64_t> super_cube_literals_raw;
        vector<uint64_t> super_cube_witness_literals;
        vector<uint64_t> super_cube_witness_literals_neg;
        for (int j = 0; j < raw_cubes[i].literals.size(); j++) {
            if (raw_cubes[i].literals[j] == 0) {
                super_cube_literals_raw.push_back(pad_lit);
                super_cube_witness_literals.push_back(pad_lit);
                super_cube_witness_literals_neg.push_back(get_negate(pad_lit));
                continue;
            }
            super_cube_literals_raw.push_back(raw_cubes[i].literals[j]);
        }
        padding(super_cube_witness_literals, raw_cubes[i].poly.deg);
        padding(super_cube_witness_literals_neg, raw_cubes[i].poly.deg);
        clause super_cube(super_cube_literals_raw, raw_cubes[i].poly.deg + 1);
        clause super_cube_witness(super_cube_witness_literals, raw_cubes[i].poly.deg + 1);
        clause super_cube_witness_neg(super_cube_witness_literals_neg, raw_cubes[i].poly.deg + 1);
        commit_and_prove_literals(super_cube, super_cube_literals, super_cube_literals_mac);
        vector<Integer> super_cube_literals_Integer;
        vector<Integer> super_cube_literals_Integer_sorted;
        get_Integers_consistent_with_committed_blocks(super_cube_literals, super_cube_literals_mac, super_cube_literals_Integer);
        sort_Integers(super_cube_literals_Integer, all_lits_in_intersections_Integer, super_cube_literals_Integer_sorted, all_lits_in_intersections_Integer_sorted);
        assert_f_submultiset_t(all_lits_in_intersections_Integer_sorted, super_cube_literals_Integer_sorted);
        super_cube.poly.ProductEqual(raw_cubes[i].poly, super_cube_witness.poly);
        super_cube_witness.poly.ConverseCheck(super_cube_witness_neg.poly);
        check_non_tautological_resolution(all_lits_clause, super_cube_witness_neg, empty_clause);
    }
    auto timer_for_initial_cube_validity_2 = chrono::high_resolution_clock::now();
    auto cost_for_initial_cube_validity = chrono::duration<double>(timer_for_initial_cube_validity_2 - timer_for_initial_cube_validity_1).count();
    cout <<"finish checking validity of the initial cubes!\n";
    
    for (int64_t i = ncls - nres; i < ncls; i++) {
	    delta = delta + 1;
        if ((delta / nres) > 0.1){
            float  progress = (float(i) / ncls);
            delta = 0;
            int barWidth = 70;
            std::cout << "[";
            int pos = barWidth * progress;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int(progress * 100.0) << " %\r";
            cout << endl;
        }

        int64_t chain_length = 0L;
        vector<uint64_t> pvt;
        vector<Integer> chain;

        if (party == ALICE) {
            chain_length = supports[i].size();
            io->send_data(&chain_length, 8);
        }else{
            io->recv_data(&chain_length, 8);
        }

        SPT s = supports[i];
        PVT p = pivots[i];


        if (party == BOB) {
            for (int j = s.size(); j < chain_length; j++) {
                s.push_back(0L);
            }
            for (int j = p.size(); j < chain_length - 1; j++) {
                p.push_back(0L);
            }
        }


        assert(s.size() == p.size() +1);
        for (uint64_t index: s) {
            chain.push_back(Integer(INDEX_SZ, index, ALICE));
        }

        pvt.push_back(0UL);
        for(uint64_t pp: p){
            pvt.push_back(pp);
        }

        bool last_clause = (i == (ncls - 1));
        vector<double> costs = check_chain(chain, pvt, i, reduced_formulaCR, removed_literals[i], quantifiers_ROZKRAM, last_clause, DEGREE);
        cost_access = cost_access + costs[0];
        cost_forallred = cost_forallred + costs[1];
        cost_resolve = cost_resolve + costs[2];
        cost_quantifier_list_access = cost_quantifier_list_access + costs[3];
    }

    check_zero_MAC(zero_block, 1);
    auto timer_4 = chrono::high_resolution_clock::now();

    reduced_formulaCR->check();

    auto timer_5 = chrono::high_resolution_clock::now();
    cost_access = cost_access +  chrono::duration<double>(timer_5 - timer_4).count();

    auto timer_6 = chrono::high_resolution_clock::now();
    
    quantifiers_ROZKRAM->check();
    
    auto timer_7 = chrono::high_resolution_clock::now();
    cost_quantifier_list_access = cost_quantifier_list_access +  chrono::duration<double>(timer_7 - timer_6).count();

    cout << "a "<< cost_access << " " << "init-cube-validity " << cost_for_initial_cube_validity <<" " << "cube-res " << cost_resolve << " "<< "i "<< cost_input << " "<< "Ered " << cost_forallred << " " << "Q-list-access: " << cost_quantifier_list_access << " t "<< cost_access + cost_resolve + cost_input + cost_forallred + cost_quantifier_list_access + cost_for_initial_cube_validity<< endl;


    bool cheat = finalize_zk_bool<BoolIO<NetIO>>();
    if(cheat)error("cheat!\n");

    uint64_t  counter = 0;

    for(int i = 0; i < threads; ++i) {
        counter = ios[i]->counter + counter;
        delete ios[i]->io;
        delete ios[i];
    }
    cout << "communication:" << counter << endl;
    cout << "----end----" << endl;
    return 0;

}