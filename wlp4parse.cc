#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <memory>
#include "wlp4data.h"
using namespace std;



// true if shifting, otherwise reduce
bool shiftOrReduce(int state_num, string next_input, map<int, pair<int, vector<string>>>& reductions) {
    // no reduce rule, shift
    if (reductions.count(state_num) == 0) return true;
    
    // lookahead present
    vector<string> lookaheads = reductions[state_num].second;
    if (find(lookaheads.begin(), lookaheads.end(), next_input) != lookaheads.end()) {
        return false;
    }
    // lookahead not present
    return true;
}

struct Node {
    string content;
    string token_kind;
    vector<Node> children;
};


void print_tree(Node& node) {
    if (node.children.size() <= 0) {
        cout << node.content << endl;
        return;
    }
    cout << node.content << endl;
    for (int i = node.children.size() - 1; i >= 0; i--) {
        print_tree(node.children[i]);
    }
}




int main() {
    // ifstream cfg_file {"wlp4data.h"};
    // std::istream& in = cfg_file;
    istringstream cfg_file {WLP4_COMBINED};
    istream& in = cfg_file;
    string s;
    vector<string> complete_rules;
    vector<pair<string, vector<string>>> production_rules;
    vector<Node> input_sequence;
    vector<Node> reduction_sequence;
    // transistions [ <curState, input> ] = nextSate
    map<pair<int, string>, int> transistions;   
    // reductions [ statenum ] = < rulenum, vector<lookaheads> ]
    map<int, pair<int, vector<string>>> reductions;
    vector<int> states;
    int num_of_shift = 0;



    // first read CFG component
    // skip header
    getline(in, s);  
    // read each production rule
    while (getline(in, s)) {
        // next component
        if (s == ".TRANSITIONS") break;

        string lhs;
        string word;
        vector<string> rhs;
        istringstream iss {s};
        complete_rules.emplace_back(s);

        // attain lhs
        iss >> lhs;

        // attain rhs
        while (iss >> word) rhs.emplace_back(word);

        production_rules.emplace_back(make_pair(lhs, rhs));
    }



    
    // read TRANSISTIONS component
    while (getline(in, s)) {

        if (s == ".REDUCTIONS") break;

        int num1, num2;
        string input;
        istringstream iss {s};
        iss >> num1;
        iss >> input;
        iss >> num2;
        transistions[make_pair(num1, input)] = num2;
    }




    // read REDUCTIONS component
    while (getline(in, s)) {

        if (s == ".END") break;

        int state_num, rule_num;
        string lookahead;
        istringstream iss {s};
        iss >> state_num;
        iss >> rule_num;
        iss >> lookahead;
        
        if (reductions.count(state_num) == 0) {
            vector<string> lookaheads;
            lookaheads.emplace_back(lookahead);
            reductions[state_num] = make_pair(rule_num, lookaheads);
        } else {
            if (reductions[state_num].first != rule_num) {
                cerr << "two different rule_num" << endl;
                return 1;
            }
            reductions[state_num].second.emplace_back(lookahead);
        }
    }

    Node this_node {"BOF BOF", "BOF"};
    input_sequence.emplace_back(this_node);
    // reading input
    while (getline(cin, s)) {
        istringstream iss {s};
        string token_kind, lexeme;
        iss >> token_kind;
        Node node {s, token_kind};
        input_sequence.emplace_back(node);
    }
    Node that_node = Node {"EOF EOF", "EOF"};
    input_sequence.emplace_back(that_node);

    reverse(input_sequence.begin(), input_sequence.end());



    // end reading
    // initial state is 0
    states.emplace_back(0);
    while (input_sequence.size() > 0) {

        // shift
        if (shiftOrReduce(states[states.size() - 1], input_sequence[input_sequence.size() - 1].token_kind, reductions)) {

            Node node = input_sequence[input_sequence.size() - 1];
            input_sequence.pop_back();
            reduction_sequence.emplace_back(node);

            // no such transitions
            if (transistions.count(make_pair(states[states.size() - 1], node.token_kind)) == 0) {
                cerr << "ERROR at " << num_of_shift + 1 << endl;
                return 1;
            }
            // successful transitions
            else {
                states.emplace_back(transistions[make_pair(states[states.size() - 1], node.token_kind)]);
                if (node.token_kind != "EOF" && node.token_kind != "BOF") {
                    num_of_shift++;
                }
                // input_sequence.pop_back();
                // reduction_sequence.emplace_back(word);
            }
        }

        // reduce
        else {

            int curState = states[states.size() - 1];
            int rule_num = reductions[curState].first;

            string tobeAdd = production_rules[rule_num].first;
            string word = tobeAdd;
            vector<string> tobeReplace = production_rules[rule_num].second;
            int num = tobeReplace.size();
            if (tobeReplace[0] == ".EMPTY") num = 0;


            Node node {complete_rules[rule_num], tobeAdd};

            for (int i = 0; i < num; i++) {
                states.pop_back();
                node.children.emplace_back(reduction_sequence[reduction_sequence.size() - 1]);
                reduction_sequence.pop_back();
            }
            reduction_sequence.emplace_back(node);

            // add new states
            if (transistions.count(make_pair(states[states.size() - 1], word)) == 0) {
                cerr << "ERROR at " << num_of_shift + 1 << endl;
                return 1;
            }
            // successful transitions
            else {
                states.emplace_back(transistions[make_pair(states[states.size() - 1], word)]);
                // input_sequence.pop_back();
                // reduction_sequence.emplace_back(word);
            }


        }


        

    }

    // print
    cout << "start BOF procedures EOF" << endl;
        for (int i = 0; i < reduction_sequence.size(); i++) {
            Node root = reduction_sequence[i];
            print_tree(root);
        }
    

        if (0) {
        for (int i = 0; i < reduction_sequence.size(); i++) cout << reduction_sequence[i].token_kind << " ";
        cout << ". ";
        for (int i = input_sequence.size() - 1; i >= 0; i--) cout << input_sequence[i].token_kind << " ";
        cout << endl;
        }


    /*
    if (reduction_sequence[0] == "BOF" && reduction_sequence[reduction_sequence.size() - 1] == "EOF") {
        cout << production_rules[0].first << " ." << endl;
    } else {
        cerr << "ERROR at " << num_of_shift + 1 << endl;
        return 1;
    }
    */







}

