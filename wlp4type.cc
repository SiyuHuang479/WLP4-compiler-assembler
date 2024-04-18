#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <memory>
using namespace std;


struct Node {
    string token_kind;  // LHS
    vector<string> content;     // RHS
    bool isExpression_Node;
    Node* parent;
    string type;            
    vector<Node*> children; 
    map<string, Node*> correspond_child;
    Node(string token_kind, vector<string> content, bool isExpression_Node, string type, vector<Node*> children) :
        token_kind{token_kind}, content{content}, isExpression_Node{isExpression_Node}, parent{NULL}, children{children} {}
};


struct procedure_content {
    vector<string> argument_list;   // signature
    map<string, string> local_symtable;
    procedure_content(void) {
        
    }
};




string what_is_type(Node* node) {
    if (node->token_kind != "type") return "ERROR: wrong pass in what is type";
    if (node->content.size() == 1) return "int";
    return "int*";
}


// clear memory
void release(Node* node) {
    for (int i = 0; i < node->children.size(); i++) {
        release(node->children[i]);
    }
    delete node;
}



// annotate_type
void annotate_type(Node* cur_node, map<string, procedure_content*>& global_symtable, map<string, string> local_table) {

    // cur_node does not need type
    if (!cur_node->isExpression_Node) return;

    // base case, TERMINAL
    if (cur_node->children.size() == 0) {

        // INT
        if (cur_node->token_kind == "NUM") {
            cur_node->type = "int";
            return;
        }
        // NULL
        if (cur_node->token_kind == "NULL") {
            cur_node->type = "int*";
            return;
        }

        // ID, in the local Symtable
        if (cur_node->token_kind == "ID" && cur_node->parent->token_kind == "lvalue") {
            if (local_table.count(cur_node->content[0]) == 0) {
                cerr << "ERROR: not declared yet" << endl;
                throw std::terminate;
            }
            cur_node->type = local_table[cur_node->content[0]];
            return;
        }
        if (cur_node->token_kind == "ID" && cur_node->parent->token_kind == "factor") {
            // variable 
            if (cur_node->parent->children.size() == 1) {
                if (local_table.count(cur_node->content[0]) == 0) {
                    cerr << "ERROR: not declared yet" << endl;
                    throw std::terminate;
                }
                cur_node->type = local_table[cur_node->content[0]];
                return;
            }
            // procedure call
            if (global_symtable.count(cur_node->content[0]) == 0) {
                cerr << "ERROR: no such procedure" << endl;
                throw std::terminate;
                return;
            }
            cur_node->isExpression_Node = false;
            return;
        }

        // ID (specifically, type is before ID)
        if (cur_node->token_kind == "ID" && cur_node->parent->token_kind == "dcl") {

            cur_node->type = what_is_type(cur_node->parent->children[0]);  
            return;          
        }
        cerr << "ERROR: non-declare variable" << endl;
        return;
    }

    // one child, NON-TERMINAL
    if (cur_node->children.size() == 1) {
        // need type, exact same type as child
        cur_node->type = cur_node->children[0]->type;
        return;
    }


    // more than one child
    // expr
    if (cur_node->token_kind == "expr") {
        if (cur_node->children[0]->type == "int" && cur_node->children[2]->type == "int") {
            cur_node->type = "int";
            return;
        }
        if (cur_node->children[0]->type == "int" && cur_node->children[2]->type == "int*" || cur_node->children[0]->type == "int*" && cur_node->children[2]->type == "int") {
            cur_node->type = "int*";
            return;
        }
        if (cur_node->children[0]->type == "int*" && cur_node->children[2]->type == "int*" && cur_node->content[1] == "MINUS") {
            cur_node->type = "int";
            return;
        }
        cerr << "ERROR: weird error" << endl;
        throw std::terminate;
    }

    // term
    if (cur_node->token_kind == "term") {
        if (cur_node->children[0]->type == "int" && cur_node->children[2]->type == "int") {
            cur_node->type = "int";
            return;
        }
        cerr << "ERROR: term wrong type" << endl;
        throw std::terminate;
    }

    // factor
    if (cur_node->token_kind == "factor") {
        // AMP &
        if (cur_node->content.size() == 2 && cur_node->content[0] == "AMP") {
            if (cur_node->children[1]->type == "int") {
                cur_node->type = "int*";
                return;
            }
            cerr << "ERROR: factor wrong type1" << endl;
            throw std::terminate;
        }
        // star *
         if (cur_node->content.size() == 2 && cur_node->content[0] == "STAR") {
            if (cur_node->children[1]->type == "int*") {
                cur_node->type = "int";
                return;
            }
            cerr << "ERROR: factor wrong type2" << endl;
            throw std::terminate;
        }
        // ( factor )
         if (cur_node->content.size() == 3 && cur_node->content[0] == "LPAREN" && cur_node->content[2] == "RPAREN") {
            cur_node->type = cur_node->children[1]->type;
            return;
        }
        // new int [ expr ]
        if (cur_node->content.size() == 5 && cur_node->content[0] == "NEW") {
            if (cur_node->children[3]->type == "int") {
                cur_node->type = "int*";
                return;
            }
            cerr << "ERROR: factor wrong type3" << endl;
            throw std::terminate;
            return;
        }
        // call procedure with no params
        if (cur_node->content.size() == 3 && cur_node->content[0] == "ID") {
            string procedure_name = cur_node->children[0]->content[0];
            if (global_symtable[procedure_name]->argument_list.size() == 0) {
                cur_node->type = "int";
                return;
            }
            cerr << "ERROR: factor wrong type3" << endl;
            throw std::terminate;
        }
        // call procedure with params
        if (cur_node->content.size() == 4 && cur_node->content[0] == "ID") {
            string procedure_name = cur_node->children[0]->content[0];
            vector<string> call_par_type;
            Node* arg = cur_node->children[2];
            call_par_type.emplace_back(arg->children[0]->type);
            while (arg->content.size() > 1) {
                arg = arg->children[2];
                call_par_type.emplace_back(arg->children[0]->type);
            }
            if (global_symtable[procedure_name]->argument_list.size() != call_par_type.size()) {
                cerr << "ERROR: factor wrong type3" << endl;
                throw std::terminate;
            }
            for (int i = 0; i < call_par_type.size(); i++) {
                if (global_symtable[procedure_name]->argument_list[i] != call_par_type[i]) {
                    cerr << "ERROR: factor wrong type3" << endl;
                    throw std::terminate;
                }
            }
            cur_node->type = "int"; 
            return;
        }
        cerr << "ERROR: unexpected" << endl;
        throw std::terminate;
    }


    // lvalue
    if (cur_node->token_kind == "lvalue") {
        // star factor 
        if (cur_node->children.size() == 2 && cur_node->children[1]->type == "int*") {
            cur_node->type = "int";
            return;
        }
        if (cur_node->children.size() == 3) {
            cur_node->type = cur_node->children[1]->type;
            return;
        }
        cerr << "ERROR: lvalue wrong type" << endl;
        throw std::terminate;
    }




}


// useless function
// requirement: pass in tokentype = dcl, params, paramlist
/*
bool find_pass_in_param(Node* cur_node, vector<string>& arglist, map<string, string>& local_table) {
    // dcl
    if (cur_node->token_kind == "dcl") {
        arglist.emplace_back(cur_node->content[0]);
        local_table[cur_node->content[1]] = cur_node->content[0];
        return true;
    }
    // params
    // params -> empty
    if (cur_node->token_kind == "params" && cur_node->content[0] == ".EMPTY") {
        return true;
    }
    // params -> paramlist
    if (cur_node->token_kind == "params" && cur_node->content[0] == "paramlist") {
        // paramlist -> dcl
        if (cur_node->children.size() == 1) return find_pass_in_param(cur_node->children[0], arglist, local_table);
        // paramlist -> dcl COMMA paramlist
        return find_pass_in_param(cur_node->children[0], arglist, local_table) && find_pass_in_param(cur_node->children[2], arglist, local_table);
    }   
    return false;
}
*/



void print_tree(Node* node, int depth) {
    // for (int i = 0; i < depth; i++) cout << "   ";
    cout << node->token_kind << " ";
    for (int i = 0; i < node->content.size(); i++) cout << node->content[i] << " ";
    if (node->isExpression_Node) cout << ": " << node->type;
    cout << endl;
    if (node->children.size() <= 0) {
        return;
    }
    for (int i = 0; i < node->children.size(); i++) {
        print_tree(node->children[i], depth + 1);
    }
}





void traverse(Node* curnode, map<string, procedure_content*>& global_symtable, procedure_content* cur_symtable) {


    // if this is a procedure, add a new local symtable
    if (curnode->token_kind == "main") {
        // if there is already wain
        if (global_symtable.count("main") != 0) {
            cerr << "ERROR: duplicated procedure name" << endl;
            throw std::terminate;
        }
        cur_symtable = new procedure_content{};
        global_symtable["main"] = cur_symtable;
    }
    else if (curnode->token_kind == "procedure") {
        // duplicate procedure name
        if (global_symtable.count((curnode->children[1])->content[0]) != 0) {
            cerr << "ERROR: duplicated procedure name" << endl;
            throw std::terminate;
        }
        cur_symtable = new procedure_content{};
        global_symtable[(curnode->children[1])->content[0]] = cur_symtable;
        // change ID to non-expression node
        curnode->children[1]->isExpression_Node = false;
    }


    // traverse all children
    for (int i = 0; i < curnode->children.size(); i++) {
        try {
        traverse(curnode->children[i], global_symtable, cur_symtable);
        } catch(...) {
            throw;
        }
    }




    // back to itself

    // if it is a declaration
    if (curnode->token_kind == "dcl" && cur_symtable) {
        string name = (curnode->children[1])->content[0];
        string type = what_is_type(curnode->children[0]);
        // duplicate variable names
        if (cur_symtable->local_symtable.count(name) > 0) {
            cerr << "ERROR: duplicate variable names" << endl;
            throw std::terminate;
        }
        cur_symtable->local_symtable[name] = type;
    }


    // if it is BECOMES
    if (curnode->token_kind == "dcls" && curnode->children.size() > 0) {
        // NUM
        if (curnode->content[3] == "NUM") {
            if (what_is_type(curnode->children[1]->children[0]) != "int") {
                cerr << "ERROR: dcls wrong type num" << endl;
                throw std::terminate;
            }
        } 
        // NULL
        else {
            if (what_is_type(curnode->children[1]->children[0]) != "int*") {
                cerr << "ERROR: dcls wrong type null" << endl;
                throw std::terminate;
            }
        }
    }


    // if it is a paramlist, add the first children to signature
    // !!! maybe siganature needs to be reverse at the end
    if (curnode->token_kind == "paramlist" && cur_symtable) {
        cur_symtable->argument_list.insert(cur_symtable->argument_list.begin(), what_is_type(curnode->children[0]->children[0]));    // is this correct?
    }

    // type annotation
    if (cur_symtable) {
        annotate_type(curnode, global_symtable, cur_symtable->local_symtable);
    }


    // if return value is not int
    if (curnode->token_kind == "procedure") {
        if (curnode->children[9]->type != "int") {
            cerr << "ERROR: wrong return type1" << endl;
            throw std::terminate;
        }
    }
    if (curnode->token_kind == "main") {
        if (curnode->children[11]->type != "int") {
            cerr << "ERROR: wrong return type2" << endl;
            throw std::terminate;
        }
    }

    // if main second parameter is not int
    if (curnode->token_kind == "main" && what_is_type(curnode->children[5]->children[0]) != "int") {
        cerr << "ERROR: wrong main parameter type" << endl;
        throw std::terminate;
    }


    // check whether variables are declared
    if (curnode->token_kind == "ID" && curnode->parent->token_kind == "lvalue" ) {
        // check if it is in the symtable
        if (cur_symtable->local_symtable.count(curnode->content[0]) == 0) {
            cerr << "ERROR: variable not declared yet" << endl;
            throw std::terminate;
        }
    }


    // semantics analysis

    // check println type
    if (curnode->token_kind == "statement" && curnode->content[0] == "PRINTLN") {
        if (curnode->children[2]->type != "int") {
            cerr << "ERROR: print wrong type" << endl;
            throw std::terminate;
        }
    }

    // check delete type
    if (curnode->token_kind == "statement" && curnode->content[0] == "DELETE") {
        if (curnode->children[3]->type != "int*") {
            cerr << "ERROR: delete wrong type" << endl;
            throw std::terminate;
        }
    }

    // check assignment type
    if (curnode->token_kind == "statement" && curnode->content[0] == "lvalue") {
        if (curnode->children[0]->type != curnode->children[2]->type) {
            cerr << "ERROR: wrong assignment type" << endl;
            throw std::terminate;
        }
    }

    // check test type
    if (curnode->token_kind == "test") {
        if (curnode->children[0]->type != curnode->children[2]->type) {
            cerr << "ERROR: compare wrong type" << endl;
            throw std::terminate;
        }
    }



}







int main() {

    istream& in = cin;
    string s;
    bool tree_build = false;
    Node* root;  // ?
    Node* cur_parent;
    vector<Node*> toBeAnnotate;
    map<string, procedure_content*> symtable;

    
    // reading input, build a "tree"
    while (getline(in, s)) {


        // setup a line
        istringstream iss {s};
        vector<string> rhs;
        string lhs, rhs_str;
        iss >> lhs;
        bool need_annotate = false;
        while (iss >> rhs_str) {
            rhs.emplace_back(rhs_str);
        }

        // check if this node needs annotation
        if (lhs == "NUM" || lhs == "NULL" || lhs == "ID" || lhs == "expr" || lhs == "term" || lhs == "factor" || lhs == "lvalue") {
            need_annotate = true;
        }

        // if this is root
        if (!tree_build) {
            vector<Node*> vec;
            Node* new_root = new Node{lhs, rhs, need_annotate, "", vec};

            root = new_root;
            if (need_annotate && (lhs == "NUM" || lhs == "NULL" || lhs == "ID")) toBeAnnotate.emplace_back(new_root);
            cur_parent = root;
            tree_build = true;
            continue;
        }

        // if this is not a root, find the correct parent node
        while (true) {
            if (find(cur_parent->content.begin(), cur_parent->content.end(), lhs) != cur_parent->content.end() && cur_parent->children.size() < cur_parent->content.size()) {
                break;
            }
            if (cur_parent->parent == NULL) {
                cerr << cur_parent->token_kind << "?";
                cerr << "ERR" << endl;
                release(root);
                for (auto it = symtable.begin(); it != symtable.end(); it++) {
                    delete it->second;
                }
                return 1;
            }
            cur_parent = cur_parent->parent;
        }

        vector<Node*> vec;
        Node* new_node = new Node{lhs, rhs, need_annotate, "", vec};
        new_node->parent = cur_parent;

        cur_parent->children.emplace_back(new_node);
        cur_parent->correspond_child[lhs] = new_node;
        cur_parent = new_node;
        if (need_annotate && (lhs == "NUM" || lhs == "NULL" || lhs == "ID")) toBeAnnotate.emplace_back(new_node);

    }
    // print_tree(root, 0);

    // print_tree(root);
    try {
        traverse(root, symtable, NULL);
    } catch(...) {
        // print_tree(root, 0);
        release(root);
        for (auto it = symtable.begin(); it != symtable.end(); it++) {
            delete it->second;
        }
        return 1;
    }



    print_tree(root, 0);
    release(root);
    for (auto it = symtable.begin(); it != symtable.end(); it++) {
        delete it->second;
    }





}
