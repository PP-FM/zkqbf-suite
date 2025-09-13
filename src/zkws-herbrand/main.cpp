#include <iostream>
#include <map>
#include "clause.h"
#include "clauseRAM.h"
#include "commons.h"

int port, party;
const int threads = 8;
int DEGREE = 4;
int true_var = 0;
block *mac, *data;
uint64_t data_mac_pointer;
SVoleF2k <BoolIO<NetIO>> *svole;
F2kOSTriple <BoolIO<NetIO>> *ostriple;
uint64_t constant;
BoolIO <NetIO> *io;

int main(int argc, char **argv) {
    cout << "---- begin ----" << endl;
    constant = 1UL << (VAL_SZ - 2);
    parse_party_and_port(argv, &party, &port);
    BoolIO <NetIO> *ios[threads];
    for (int i = 0; i < threads; ++i)
        ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE ? nullptr : argv[3], port + i), party == ALICE);  
    char *qbffile = argv[4];     
    char *herbrandfile = argv[5];    
    char *prooffile = argv[6];

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
    cout << endl << "----set up----" << endl << endl;

    GF2X P;
//    random(P, 128);
    SetCoeff(P, 128, 1);
    SetCoeff(P, 7, 1);
    SetCoeff(P, 2, 1);
    SetCoeff(P, 1, 1);
    SetCoeff(P, 0, 1);
    GF2E::init(P);

    // This section is to get the QBF from both the prover and the verifier
    // So it assumes that the qbf is public. A private qbf version may be in a different branch
    vector <uint64_t> e_vars;
    vector <uint64_t> a_vars;
    vector <CLS> qbf_clauses;
    vector <uint64_t> max_dep_for_a_vars;
    vector <uint64_t> dep_for_e_vars;

    readqbf(string(qbffile), e_vars, a_vars, qbf_clauses, max_dep_for_a_vars, dep_for_e_vars);
    
    cout << "----QBF matrix----" << endl;
    cout << "number of forall variables: " << a_vars.size() << endl;
    cout << "number of existential variables: " << e_vars.size() << endl;
    cout << "number of clauses: " << qbf_clauses.size() << endl << endl;

    int ncls = 0, nres = 0;
    assert(qbf_clauses[qbf_clauses.size()-1].size() == 1);
    true_var = qbf_clauses[qbf_clauses.size()-1][0];
    cout << "Variable representing true: " << true_var << endl << endl;
    // This is the end of the aforementioned public qbf reading section.

    // This section is to read the prover-private skolem function.
    // It assumes that there are no latches in the AIGER file.
    int herbrand_deg = 0;

    vector <int64_t> dependencies;
    std::map <uint64_t, uint64_t> var_to_index;
    var_to_index[true_var] = 0;
    vector <CLS> her_vars;
    vector <SPT> herbrand_supports;
    int num_ands = 0, num_ins = 0, num_outs = 0, max_var = 0;

    if (party == ALICE) {
        readherbrand(string(herbrandfile), her_vars, dependencies, herbrand_supports, num_ands, num_ins, num_outs, max_var, var_to_index);
        cout << string(herbrandfile) << endl;
        io->send_data(&num_ins, 4);
        io->send_data(&num_outs, 4);
        io->send_data(&num_ands, 4);
        assert(var_to_index.size() == num_ins+num_ands+1);
    }
    if (party == BOB) {
        io->recv_data(&num_ins, 4);
        io->recv_data(&num_outs, 4);
        io->recv_data(&num_ands, 4);
        dependencies = vector<int64_t>(1+num_ins+num_ands);
        her_vars = vector<CLS>(1+num_ins+num_ands);
        herbrand_supports = vector<SPT>(1+num_ins+num_ands);
        for (int i = 1; i <= num_ins+num_ands; i++) {
            var_to_index[i] = 0;
        }
        assert(var_to_index.size() == num_ins+num_ands+1);
    }
    cout << "----Herbrand Function----" << endl;
    cout << "number of input variables: " << num_ins << endl;
    cout << "number of output variables: " << num_outs << endl;
    cout << "number of and gates: " << num_ands << endl << endl;
    // This is the end of the aforementioned private skolem reading section.

    vector <CLS> clauses;
    vector <SPT> supports;
    vector <SPT> pivots;

    if (party == ALICE) {
        readproof(string(prooffile), DEGREE, clauses, supports, pivots, ncls, nres);
        cout << string(prooffile) << endl;
        io->send_data(&nres, 4);
        io->send_data(&ncls, 4);
        io->send_data(&DEGREE, 4);
    }

    if (party == BOB) {
        io->recv_data(&nres, 4);
        io->recv_data(&ncls, 4);
        io->recv_data(&DEGREE, 4);

        clauses = vector<CLS>(ncls);
        supports = vector<SPT>(ncls);
        pivots = vector < vector < int64_t >> (ncls);
    }
    cout << "----input proof----" << endl;
    cout << "number of resolution steps: " << nres << endl;
    cout << "total number of clauses: " << ncls << endl;
    cout << "DEGREE for ZKUNSAT: " << DEGREE << endl << endl;
    cout << "----end set up----" << endl << endl;
    cout << "----loading input----" << endl;
    //if ( ncls > 524287) return 0; 

    double cost_input = 0;
    double cost_resolve = 0;
    double cost_access = 0 ;

/*
 * encode the formula and resolution proof
 * */
    auto timer_0 = chrono::high_resolution_clock::now();
    vector<clause> raw_formula;

    vector<Integer> pvt_deps;
    vector<clause> herbrand_variables;
    for (int i = 0; i < num_ands+num_ins+1; i++) {
        pvt_deps.push_back(Integer(INDEX_SZ, dependencies[i], ALICE));
        vector<uint64_t> variable;
        for (int64_t lit: her_vars[i]){
            variable.push_back(wrap(lit));
        }
        padding(variable, 3);
        clause tmp(variable, 3);
        herbrand_variables.push_back(tmp);
    }
    clauseRAM<BoolIO<NetIO>>* herbrand_vars_CR = new clauseRAM<BoolIO<NetIO>>(party, INDEX_SZ, 3);
    herbrand_vars_CR->init(herbrand_variables);
    ROZKRAM<BoolIO<NetIO>>* dependencies_ROZKRAM = new ROZKRAM<BoolIO<NetIO>>(party, INDEX_SZ, INDEX_SZ);
    dependencies_ROZKRAM->init(pvt_deps);


    float delta = 0 ;


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
        for (int64_t lit: clauses[i]) {
            literals.push_back((wrap(lit)));
        }
        padding(literals, DEGREE);
        clause c(literals, DEGREE);
        raw_formula.push_back(c);
    }
    clauseRAM<BoolIO<NetIO>>* formula = new clauseRAM<BoolIO<NetIO>>(party, INDEX_SZ, DEGREE);
    formula->init(raw_formula);
    cout <<"finish  input!\n";
    auto timer_1 = chrono::high_resolution_clock::now();
    cost_input = chrono::duration<double>(timer_1 - timer_0).count();

    auto herbrand_timer_begin = chrono::high_resolution_clock::now();
    // This loop checks
    // 1) That the first variable in the skolem_vars_CR is the true variable.
    // 2) That the next num_ins variables in the skolem_vars_CR 
    //    are the input variables and are equal to the forall variables in the QBF.
    // 3) That each variable 0 < v <= num_ins+num_ands is assigned only 
    //    once in the AIGER.
    // This is sufficient to say no forall variable is constrained by the 
    // AIGER.
    // 4) The dependencies of the a_vars are correct.
    for (int i = 0; i < num_ins+num_ands+1; i++) {
        if (i == 0) {
            vector<uint64_t> temp_true_var;
            temp_true_var.push_back(wrap(true_var));
            temp_true_var.push_back(wrap(-true_var));
            padding(temp_true_var, 3);
            clause temp_true_var_clause(temp_true_var, 3);
            temp_true_var_clause.poly.Equal(herbrand_vars_CR->get(Integer(INDEX_SZ, 0, PUBLIC)).poly);
        }
        else if (i < num_ins+1) {
            vector<uint64_t> temp_variable;
            temp_variable.push_back(wrap(e_vars[i-1]));
            temp_variable.push_back(wrap(-e_vars[i-1]));
            padding(temp_variable, 3);
            clause temp_var_clause(temp_variable, 3);
            Integer index = Integer(INDEX_SZ, i, PUBLIC);
            temp_var_clause.poly.Equal(herbrand_vars_CR->get(index).poly);
            // Check the dependencies for the e_vars
            Integer tmp_dep = Integer(INDEX_SZ, dep_for_e_vars[i-1], PUBLIC);
            if (!(tmp_dep.equal(dependencies_ROZKRAM->read(index)).reveal()))
                error("dependency issue in herbrand function (e_vars)");
        }
        if (i < num_ins+num_ands) {
            vector<uint64_t> temp_variable;
            temp_variable.push_back(wrap(i+1));
            temp_variable.push_back(wrap(-i-1));
            padding(temp_variable, 3);
            clause temp_var_clause(temp_variable, 3);
            Integer index = Integer(INDEX_SZ, var_to_index[i+1], ALICE);
            // The check below is probably not necessary because both parties know the size of 
            // herbrand_vars_CR = 1+num_ins+num_ands
            // if (index.geq(Integer(INDEX_SZ, 1+num_ins+num_ands, PUBLIC)).reveal())
            //     error("herbrand function incorrect");
            temp_var_clause.poly.Equal(herbrand_vars_CR->get(index).poly);
        }
    }

    // This loop checks that the input clauses in the resolution proof were derived from the 
    // Herbrand function's AIGER and the initial QBF's matrix.
    // It also ensure that the dependencies of the variables that are not e_vars is valid.
    for (int i = 0; i < ncls - nres; i++) {
        if (i < (3*(num_ands))) {
            if (i % 3 == 0){
                Integer ind = Integer(INDEX_SZ, 1+num_ins+ int(i/3), PUBLIC);
                clause out_herbrand_var = herbrand_vars_CR->get(ind);   
                SPT dep_s = herbrand_supports[1 + num_ins+ int(i/3)];
                if (party == BOB) {
                    dep_s.push_back(0L);
                    dep_s.push_back(0L);
                }
                // Check that the dependencies listed in the herbrand file are valid.
                // i.e. dep(out) >= dep(in1) and dep(in2) 
                Integer dependency_of_out_var = dependencies_ROZKRAM->read(ind);
                Integer Index_of_in1 = Integer(INDEX_SZ, abs(dep_s[0])-1, ALICE);
                Integer Index_of_in2 = Integer(INDEX_SZ, abs(dep_s[1])-1, ALICE);
                if (Index_of_in1.geq(ind).reveal()) error ("herbrand function incorrect");
                if (Index_of_in2.geq(ind).reveal()) error ("herbrand function incorrect");
                Integer dependency_of_in1 = dependencies_ROZKRAM->read(Index_of_in1);
                Integer dependency_of_in2 = dependencies_ROZKRAM->read(Index_of_in2);
                if (!(dependency_of_out_var.geq(dependency_of_in1).reveal())) error ("dependency issue in herbrand function (intermediate var)");
                if (!(dependency_of_out_var.geq(dependency_of_in2).reveal())) error ("dependency issue in herbrand function (intermediate var)");     
                
                // A lot of variables are coming up, so here's the jist
                // if the aiger line has "a b c", then it is equivalent to
                // having the clauses "-a b", "-a c" and "a -b -c"
                // So, "a" should be root_out
                // "b" should be inp1
                // "c" should be inp2

                CLS out_raw = her_vars[1 + num_ins + int(i/3)];
                vector <uint64_t> root_out;
                vector <uint64_t> root_negout;
                if (out_raw.size() != 0) {
                    root_out.push_back(wrap(abs(out_raw[0])));
                    root_negout.push_back(wrap(-abs(out_raw[0])));
                }
                padding(root_out, 3);
                padding(root_negout, 3);
                clause out(root_out, 3);
                clause negout(root_negout, 3);
                out.poly.ConverseCheck(negout.poly);
                vector <uint64_t> root_inp1;
                vector <uint64_t> root_neginp1;
                vector <uint64_t> root_inp2;
                vector <uint64_t> root_neginp2;
                if (dep_s.size() == 2) {
                    if (dep_s[0] < 0) {
                        root_inp1.push_back(wrap(-abs(her_vars[abs(dep_s[0])-1][0])));
                        root_neginp1.push_back(wrap(abs(her_vars[abs(dep_s[0])-1][0])));
                    }
                    else if (dep_s[0] > 0){
                        root_inp1.push_back(wrap(abs(her_vars[abs(dep_s[0])-1][0])));
                        root_neginp1.push_back(wrap(-abs(her_vars[abs(dep_s[0])-1][0])));
                    }
                    if (dep_s[1] < 0) {
                        root_inp2.push_back(wrap(-abs(her_vars[abs(dep_s[1])-1][0])));
                        root_neginp2.push_back(wrap(abs(her_vars[abs(dep_s[1])-1][0])));
                    }
                    else if (dep_s[1] > 0){
                        root_inp2.push_back(wrap(abs(her_vars[abs(dep_s[1])-1][0])));
                        root_neginp2.push_back(wrap(-abs(her_vars[abs(dep_s[1])-1][0])));
                    }
                }
                else if (dep_s.size() != 0) {
                    error("check herbrand AIGER and line");
                }
                padding(root_inp1, 3);
                padding(root_neginp1, 3);
                clause inp1(root_inp1, 3);
                clause neginp1(root_neginp1, 3);
                inp1.poly.ConverseCheck(neginp1.poly);
                padding(root_inp2, 3);
                padding(root_neginp2, 3);
                clause inp2(root_inp2, 3);
                clause neginp2(root_neginp2, 3);
                inp2.poly.ConverseCheck(neginp2.poly);

                // TODO: Check that these polynomials are unit polynomials
                // TODO: Check that the lines i, i+1 and i+2 in the proof file are 
                //       consistent with the skolem function.
                clause first = formula->get(Integer(INDEX_SZ, i, PUBLIC));
                clause second = formula->get(Integer(INDEX_SZ, i+1, PUBLIC));
                clause third = formula->get(Integer(INDEX_SZ, i+2, PUBLIC));
                first.poly.ProductEqual(negout.poly, inp1.poly);
                second.poly.ProductEqual(negout.poly, inp2.poly);

                // This section checks that the clause (out v neginp2 v neginp1) is a subclause
                // of the clause i+2 in the prooffile.
                // This is sufficient because having a super-clause C' of C instead of C would preserve
                // The satisfiability of the formula (whatever assignment satisfied C, satisfies C'). 
                vector <uint64_t> witness_for_neginp1_roots;
                vector <uint64_t> witness_for_neginp2_roots;
                vector <uint64_t> witness_for_out_roots;
                vector <uint64_t> witness_for_third_roots;
                if (party == ALICE) {
                    if (root_inp1[0] == root_inp2[0]) {
                        witness_for_out_roots.push_back(root_neginp1[0]);
                        witness_for_neginp1_roots.push_back(root_out[0]);
                        witness_for_neginp2_roots.push_back(root_out[0]);
                        witness_for_third_roots.push_back(root_neginp1[0]);
                    }
                    else {
                        witness_for_out_roots.push_back(root_neginp1[0]);
                        witness_for_out_roots.push_back(root_neginp2[0]);
                        witness_for_neginp1_roots.push_back(root_out[0]);
                        witness_for_neginp1_roots.push_back(root_neginp2[0]);
                        witness_for_neginp2_roots.push_back(root_out[0]);
                        witness_for_neginp2_roots.push_back(root_neginp1[0]);
                    }
                }
                padding(witness_for_neginp1_roots, 3);
                padding(witness_for_neginp2_roots, 3);
                padding(witness_for_out_roots, 3);
                padding(witness_for_third_roots, 3);
                clause witness_for_neginp1(witness_for_neginp1_roots, 3);
                clause witness_for_neginp2(witness_for_neginp2_roots, 3);
                clause witness_for_out(witness_for_out_roots, 3);
                clause witness_for_third(witness_for_third_roots, 3);
                third.poly.ProductEqual(witness_for_neginp1.poly, neginp1.poly); // Shows neginp1 is in third
                third.poly.ProductEqual(witness_for_neginp2.poly, neginp2.poly); // Shows neginp2 is in third
                third.poly.ProductEqual(witness_for_out.poly, out.poly); // Shows out is in third
                // Check that third is in out.poly*neginp1.poly*neginp2.poly
                vector <uint64_t> would_be_third_roots;
                would_be_third_roots.push_back(root_out[0]);
                would_be_third_roots.push_back(root_neginp1[0]);
                would_be_third_roots.push_back(root_neginp2[0]);
                padding(would_be_third_roots, 4);
                clause would_be_third(would_be_third_roots, 4);
                would_be_third.poly.ProductEqual(witness_for_third.poly, third.poly);

                // Check the committed polynomials are consistent with the herbrand file
                clause inp1_herbrand_var = herbrand_vars_CR->get(Index_of_in1);
                clause inp2_herbrand_var = herbrand_vars_CR->get(Index_of_in2);
                inp1_herbrand_var.poly.ProductEqual(inp1.poly, neginp1.poly);
                inp2_herbrand_var.poly.ProductEqual(inp2.poly, neginp2.poly);
                out_herbrand_var.poly.ProductEqual(out.poly, negout.poly);
            }
        }
        else {
            //VERIFY THE REST OF THE INPUT CNF FOR ZKUNSAT WITH VERIFIER'S COPY OF QBF
            int j = i - (3*num_ands);
            vector<uint64_t> tmp_clause_lits;
            for (auto lit: qbf_clauses[j]) {
                tmp_clause_lits.push_back(wrap(lit));
            }
            padding(tmp_clause_lits, DEGREE);
            clause tmp_clause(tmp_clause_lits, DEGREE);
            clause tmp_clause_from_formula = formula->get(Integer(INDEX_SZ, i, PUBLIC));
            tmp_clause.poly.Equal(tmp_clause_from_formula.poly);
        }
    }

    // This loop checks that the dependencies of the a_vars is valid
    for (int i = 0; i < a_vars.size(); i++) {
        Integer tmp_dep = Integer(INDEX_SZ, max_dep_for_a_vars[i], PUBLIC);
        Integer index = Integer(INDEX_SZ, var_to_index[a_vars[i]], ALICE);
        if (!(tmp_dep.geq(dependencies_ROZKRAM->read(index)).reveal()))
            error("dependency issue in skolem function (a_vars)");
    }
    herbrand_vars_CR->check();
    dependencies_ROZKRAM->check();
    auto herbrand_timer_end = chrono::high_resolution_clock::now();
    auto cost_herbrand = chrono::duration<double>(herbrand_timer_end - herbrand_timer_begin).count();

    delta = 0;
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

        //cout<<s.size()<< " " << i << endl;
        assert(s.size() == p.size() +1);
        for (uint64_t index: s) {
            chain.push_back(Integer(INDEX_SZ, index, ALICE));
        }

        pvt.push_back(0UL);
        for(uint64_t pp: p){
            pvt.push_back(pp);
        }

        bool last_clause = (i == (ncls - 1));
        auto cost = check_chain(chain, pvt, i, formula, last_clause);
        cost_resolve = cost_resolve + cost.second;
        cost_access = cost_access + cost.first;
    }

    check_zero_MAC(zero_block, 1);
    auto timer_4 = chrono::high_resolution_clock::now();

    formula->check();
    dependencies_ROZKRAM->check();
    herbrand_vars_CR->check();

    auto timer_5 = chrono::high_resolution_clock::now();
    cost_access = cost_access +  chrono::duration<double>(timer_5 - timer_4).count();
    cout << "a "<< cost_access << " " << "r " << cost_resolve << " "<< "i "<< cost_input << " t "<< chrono::duration<double>(timer_5-timer_0).count() << endl;
    cout << "herbrand function checking time " << cost_herbrand << endl;

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
