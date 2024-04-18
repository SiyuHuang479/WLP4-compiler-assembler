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
    Node(string token_kind, vector<string> content, bool isExpression_Node, string type, vector<Node*> children) :
        token_kind{token_kind}, content{content}, isExpression_Node{isExpression_Node}, parent{NULL}, type{type}, children{children} {}
};



void print_tree(Node* node, int depth) {
    for (int i = 0; i < depth; i++) cout << "   ";
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


// clear memory
void release(Node* node) {
    for (int i = 0; i < node->children.size(); i++) {
        release(node->children[i]);
    }
    delete node;
}


// this node has to be a dcl
string what_is_type_dcl(Node* node) {
    if (node->token_kind != "dcl") {
        return "error: not dcl";
    }
    if (node->children[0]->content.size() > 1) return "int*";
    return "int";
}


string push_lvalue(int register_num, int* offset, map<string, pair<string, int>>* SymTable, vector<int>* MemoryStack, string variable, string type) {
    ostringstream oss;
    // check duplicate name
    
    if (SymTable->count(variable) != 0) {
        cerr << "; error: duplicate variable name" << endl;
        return "; error";
    }
    
    // insert SymTable
   // (SymTable)[variable] = make_pair(type, offset);
    SymTable->emplace(variable, make_pair(type, *offset));

    // update offse
    *offset = *offset - 4;
    // code generation
    oss << "sw $" << register_num << ", " << "-4" << "($30)" << endl << "sub $30, $30, $4" << endl;
    return oss.str();
}

string push_rvalue(int register_num, int* offset) {
    ostringstream oss;
    // update offset
    *offset = *offset - 4;
    // code generation
    oss << "; push rvalue" << endl;
    oss << "sw $" << register_num << ", " << "-4" << "($30)" << endl << "sub $30, $30, $4" << endl;
    oss << endl;
    return oss.str();
}

string pop_rvalue(int register_num, int* offset) {
    *offset = *offset + 4;
    ostringstream oss;
    oss << "; pop rvalue" << endl;
    oss << "add $30, $30, $4" << endl << "lw $" << register_num << ", -4($30)" << endl;
    oss << endl;
    return oss.str();
}

string code(string variable, map<string, pair<string, int>>* SymTable) {
    int offset = (*SymTable)[variable].second;
    ostringstream oss;
    oss << "; code: " << endl;
    oss << "lw $3, " << offset << "($29)" << endl;
    oss << endl;
    return oss.str();
}

// string pop(int register_num,int& offset, map<string, pair<string, int>>& SymTable, )



void codeGen(Node* curnode, ostream& out, int* cur_offset, map<string, pair<string, int>>* SymTable, vector<int>* MemoryStack, int* label_counter, int register_num = -1) {

    // start → BOF procedures EOF
    if (curnode->token_kind == "start") {
        codeGen(curnode->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // procedures → main
    if (curnode->token_kind == "procedures" && curnode->content[0] == "main") {
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // procedures → procedure procedures
    if (curnode->token_kind == "procedures" && curnode->content.size() == 2) {
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        codeGen(curnode->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // main → INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE 
    if (curnode->token_kind == "main") {

        out << "wain: " << endl;
        int offset = 0;
        cur_offset = &offset;
        map<string, pair<string, int>> local_table;
        SymTable = &local_table;

        // call init
        // push 1 and 2
        out << push_rvalue(1, cur_offset);
        out << push_rvalue(2, cur_offset);
        // mips.twoints
        if (what_is_type_dcl(curnode->children[3]) == "int") {
            out << "add $2, $0, $0" << endl;
        }
        // call
        out << push_rvalue(31, cur_offset);
        out << "lis $5" << endl << ".word init" << endl;
        out << "jalr $5" << endl;
        out << pop_rvalue(31, cur_offset);
        out << pop_rvalue(2, cur_offset);
        out << pop_rvalue(1, cur_offset);
        out << endl;


        // handle parameters (dcl)
        codeGen(curnode->children[3], out, cur_offset, SymTable, MemoryStack, label_counter, 1);
        codeGen(curnode->children[5], out, cur_offset, SymTable, MemoryStack, label_counter, 2);

        // handle dcls
        codeGen(curnode->children[8], out, cur_offset, SymTable, MemoryStack, label_counter);

        // handle statements
        codeGen(curnode->children[9], out, cur_offset, SymTable, MemoryStack, label_counter);

        // handle RETURN
        codeGen(curnode->children[11], out, cur_offset, SymTable, MemoryStack, label_counter);

        out << endl;

        return;
    }

    // procedure → INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    if (curnode->token_kind == "procedure") {

        out << "; procedure to INT ID LAREN etc" << endl;
        // new symbol table
        map<string, pair<string, int>> local_table;

        // procedure name (ID)
        string procedure_name = curnode->children[1]->content[0];
        out << "F" << procedure_name << ":" << endl;

        // params
        codeGen(curnode->children[3], out, cur_offset, &local_table, MemoryStack, label_counter);

        // new offset
        int local_offset = 0;
        out << "sub $29, $30, $4" << endl;
        // handle dcls
        codeGen(curnode->children[6], out, &local_offset, &local_table, MemoryStack, label_counter);
        out << push_rvalue(5, &local_offset);
        out << push_rvalue(6, &local_offset);
        out << push_rvalue(7, &local_offset);
        codeGen(curnode->children[7], out, &local_offset, &local_table, MemoryStack, label_counter);
        codeGen(curnode->children[9], out, &local_offset, &local_table, MemoryStack, label_counter);
        out << pop_rvalue(7, &local_offset);
        out << pop_rvalue(6, &local_offset);
        out << pop_rvalue(5, &local_offset);
        out << "add $30, $29, $4" << endl;
        out << "jr $31" << endl;
        out << endl;
        return;
    }

    // statement → DELETE LBRACK RBRACK expr SEMI
    if (curnode->token_kind == "statement" && curnode->children.size() == 5 && curnode->content[0] == "DELETE") { 
        out << "; delete int " << endl;
        codeGen(curnode->children[3], out, cur_offset, SymTable, MemoryStack, label_counter);
        int current_label = *label_counter;
        out << "beq $3, $11, skipDelete" << *label_counter << endl;
        *label_counter = current_label + 1;
        out << "add $1, $3, $0" << endl;
        out << push_rvalue(31, cur_offset);
        out << "lis $5" << endl << ".word delete" << endl;
        out << "jalr $5" << endl;
        out << pop_rvalue(31, cur_offset);
        out << "skipDelete" << current_label << ":" << endl;
        return;
    }


    // factor → NEW INT LBRACK expr RBRACK
    if (curnode->token_kind == "factor" && curnode->children.size() == 5 && curnode->content[0] == "NEW") {
        out << "; new int " << endl;
        codeGen(curnode->children[3], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "add $1, $3, $0" << endl;
        out << push_rvalue(31, cur_offset);
        out << "lis $5" << endl << ".word new" << endl;
        out << "jalr $5" << endl;
        out << pop_rvalue(31, cur_offset);
        out << "bne $3, $0, 1" << endl;
        out << "add $3, $0, $11" << endl << endl;
        return;
    }

    // factor → ID LPAREN RPAREN
    if (curnode->token_kind == "factor" && curnode->children.size() == 3 && curnode->content[0] == "ID") {
        out << "; factor to ID LPAREN RPAREN" << endl;
        out << push_rvalue(29, cur_offset);
        out << push_rvalue(31, cur_offset);
        out << "lis $5" << endl;
        out << ".word " << "F" << curnode->children[0]->content[0] << endl;
        out << "jalr $5" << endl;
        out << pop_rvalue(31, cur_offset);
        out << pop_rvalue(29, cur_offset);
        out << endl;
        return;
    }

    // factor → ID LPAREN arglist RPAREN
    if (curnode->token_kind == "factor" && curnode->children.size() == 4) {
        out << "; factor to ID LPAREN arglist RPAREN" << endl;
        out << push_rvalue(29, cur_offset);
        out << push_rvalue(31, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "lis $5" << endl;
        out << ".word " << "F" << curnode->children[0]->content[0] << endl;
        out << "jalr $5" << endl;
        // find number of expr
        Node* this_node = curnode->children[2];
        int number_of_expr = 1;
        while (this_node->children.size() == 3) {
            number_of_expr++;
            this_node = this_node->children[2];
        }
        // pop all expr
        for (int i = 0; i < number_of_expr; i++) {
            out << pop_rvalue(31, cur_offset);
        }
        out << pop_rvalue(31, cur_offset);
        out << pop_rvalue(29, cur_offset);
        out << endl;
        return;
    }

    // arglist → expr and arglist → expr COMMA arglist
    if (curnode->token_kind == "arglist") {
        out << "; arglist to " << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        // to expr
        if (curnode->children.size() == 1) {
            return;
        }
        // to expr COMMA arglist
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // params → paramlist
    if (curnode->token_kind == "params" && curnode->content[0] == "paramlist") {
        // calculate the number of parameter
        int number_of_parameter = 1;
        Node* this_node = curnode->children[0];
        while (this_node->children.size() == 3) {
            number_of_parameter++;
            this_node = this_node->children[2];
        }
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter, number_of_parameter);
        return;
    }

    // paramlist → dcl and paramlist → dcl COMMA paramlist
    if (curnode->token_kind == "paramlist") {
        // add to local symbol table
        Node* dcl = curnode->children[0];
        SymTable->emplace(dcl->children[1]->content[0], make_pair(what_is_type_dcl(dcl), 4 * register_num));
        // if -> dcl
        if (curnode->children.size() == 1) {
            return;
        }
        // if -> dcl COMMA paramlist
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter, register_num - 1);
        return;
    }

    // statements → statements statement
    if (curnode->token_kind == "statements" && curnode->children.size() == 2) {
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        codeGen(curnode->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // statement → lvalue BECOMES expr SEMI
    if (curnode->token_kind == "statement" && curnode->children.size() == 4) {
        out << "; statement to lvalue BECOMES expr SEMI" << endl;
        // code(expr)
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        Node* lvalue = curnode->children[0];

        int offset;
        while (true) {
            // lvalue → ID
            if (lvalue->children.size() == 1) {
                offset = (*SymTable)[lvalue->children[0]->content[0]].second;
                out << "sw $3, " << offset << "($29)" << endl;
                out << endl;
                return;
            }
            // lvalue → LPAREN lvalue RPAREN 
            else if (lvalue->children.size() == 3) {
                lvalue = lvalue->children[1];
            }
            // lvalue → STAR factor
            else {
                out << push_rvalue(3, cur_offset);
                codeGen(lvalue->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
                out << pop_rvalue(5, cur_offset);
                out << "sw $5, 0($3)" << endl << endl;
                return;
            }

        }
        return; 

    }

    // dcls → dcls dcl BECOMES NUM SEMI
    if (curnode->token_kind == "dcls" && curnode->children.size() > 1 && curnode->children[3]->token_kind == "NUM") {
        out << "; dcls to dcls dcl BECOMES NUM SEMI" << endl;
        // deal with current dcl
        out << "lis $3" << endl;
        out << ".word " << curnode->children[3]->content[0] << endl;
        out << push_lvalue(3, cur_offset, SymTable, MemoryStack, curnode->children[1]->children[1]->content[0], what_is_type_dcl(curnode->children[1]));
        // deal with dcls
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // dcls → dcls dcl BECOMES NULL SEMI
     if (curnode->token_kind == "dcls" && curnode->children.size() > 1 && curnode->children[3]->token_kind == "NULL") {
        out << "; dcls to dcls dcl BECOMES NULL SEMI" << endl;
        // deal with current dcl
        out << "lis $3" << endl;
        out << ".word 1" << endl;
        out << push_lvalue(3, cur_offset, SymTable, MemoryStack, curnode->children[1]->children[1]->content[0], "int*");
        // deal with dcls
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        return;
    }

    // dcl → type ID
    if (curnode->token_kind == "dcl") {
        // push
        out << "; dcl to type ID" << endl;
        out << push_lvalue(register_num, cur_offset, SymTable, MemoryStack, curnode->children[1]->content[0], what_is_type_dcl(curnode));
        out << endl;
        return;
    }

    // expr → term
    if (curnode->token_kind == "expr" && curnode->content.size() == 1 && curnode->content[0] == "term") {
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter, register_num);
        return;
    }

    // term → factor
    if (curnode->token_kind == "term" && curnode->content.size() == 1 && curnode->content[0] == "factor") {
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter, register_num);
        return;
    }

    // factor → NUM
    if (curnode->token_kind == "factor" && curnode->content.size() == 1 && curnode->content[0] == "NUM") {
        string num = curnode->children[0]->content[0];
        out << "lis $3" << endl;
        out << ".word " << num << endl;
        return;
    }

    // factor → NULL
    if (curnode->token_kind == "factor" && curnode->content.size() == 1 && curnode->content[0] == "NULL") {
        out << "lis $3" << endl;
        out << ".word 1" << endl;
        return;
    }

    // factor → AMP lvalue
    if (curnode->token_kind == "factor" && curnode->content.size() == 2 && curnode->content[0] == "AMP") {

        out << "; factor to AMP lvalue" << endl;
        
        Node* lvalue = curnode->children[1];
        while (lvalue->children.size() == 3) {
            lvalue = lvalue->children[1];
        }
        // lvalue → ID
        if (lvalue->children.size() == 1) {
            string name = lvalue->children[0]->content[0];
            int offset = (*SymTable)[name].second;
            out << "lis $3" << endl;
            out << ".word " << offset << endl;
            out << "add $3, $3, $29" << endl;
        } 
        // lvalue → STAR factor
        else {
            codeGen(lvalue->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
        }
        out << endl;
        return;
    }


    // factor → ID
     if (curnode->token_kind == "factor" && curnode->content.size() == 1 && curnode->content[0] == "ID") {
        out << "; " << curnode->children[0]->content[0] << endl;
        out << code(curnode->children[0]->content[0], SymTable);
        return;
     }

    // factor → LPAREN expr RPAREN
    if (curnode->token_kind == "factor" && curnode->content.size() == 3 && curnode->content[0] == "LPAREN") {
        codeGen(curnode->children[1], out, cur_offset, SymTable, MemoryStack, label_counter, register_num);
        return;
    }

    // factor → STAR factor
    if (curnode->token_kind == "factor" && curnode->content.size() == 2 && curnode->content[0] == "STAR") {
        codeGen(curnode->children[1], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "lw $3, 0($3)" << endl;
        return;
    } 

    // expr → expr PLUS term
    if (curnode->token_kind == "expr" && curnode->children[1]->token_kind == "PLUS") {
        out << "; expr to expr PLUS term" << endl;
        // int + int
        if (curnode->children[0]->type == "int" && curnode->children[2]->type == "int") {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << pop_rvalue(5, cur_offset);
            out << "add $3, $5, $3" << endl;
        }
        // int* + int
        else if (curnode->children[0]->type == "int*" && curnode->children[2]->type == "int") {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << "mult $3, $4" << endl;
            out << "mflo $3" << endl;
            out << pop_rvalue(5, cur_offset);
            out << "add $3, $5, $3" << endl;
        } 
        // int + int*
        else {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << "mult $3, $4" << endl;
            out << "mflo $3" << endl;
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << pop_rvalue(5, cur_offset);
            out << "add $3, $5, $3" << endl;
        }
        out << endl;
        return;
    }

    // expr → expr MINUS term
    if (curnode->token_kind == "expr" && curnode->children[1]->token_kind == "MINUS") {
        out << "; expr to expr MINUS term" << endl;
        // int - int
        if (curnode->children[0]->type == "int" && curnode->children[2]->type == "int") {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << pop_rvalue(5, cur_offset);
            out << "sub $3, $5, $3" << endl;
        }
        // int* - int
        else if (curnode->children[0]->type == "int*" && curnode->children[2]->type == "int") {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << "mult $3, $4" << endl;
            out << "mflo $3" << endl;
            out << pop_rvalue(5, cur_offset);
            out << "sub $3, $5, $3" << endl;
        } 
        // int* - int*
        else {
            codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << push_rvalue(3, cur_offset);
            codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
            out << pop_rvalue(5, cur_offset);
            out << "sub $3, $5, $3" << endl;
            out << "div $3, $4" << endl;
            out << "mflo $3" << endl;
        }
        out << endl;
        return;
    }

    // expr → expr STAR term
    if (curnode->token_kind == "term" && curnode->children[1]->token_kind == "STAR") {
        out << "; expr to expr MINUS term" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        out << "mult $5, $3" << endl;
        out << "mflo $3" << endl;
        out << endl;
        return;
    }

    // expr → expr SLASH term
    if (curnode->token_kind == "term" && curnode->children[1]->token_kind == "SLASH") {
        out << "; expr to expr MINUS term" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        out << "div $5, $3" << endl;
        out << "mflo $3" << endl;
        out << endl;
        return;
    }

    // expr → expr PCT term
    if (curnode->token_kind == "term" && curnode->children[1]->token_kind == "PCT") {
        out << "; expr to expr PCT term" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        out << "div $5, $3" << endl;
        out << "mfhi $3" << endl;
        out << endl;
        return;
    }

    // test → expr LT expr <
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "LT") {
        out << "; test to expr LT expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $3, $5, $3" << endl;
        } else {
            out << "sltu $3, $5, $3" << endl;
        }
        out << endl;
        return;
    }

    // test → expr GT expr >
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "GT") {
        out << "; test to expr GT expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $3, $3, $5" << endl;
        } else {
            out << "sltu $3, $3, $5" << endl;
        }
        out << endl;
        return;
    }

    // test → expr NE expr
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "NE") {
        out << "; test to expr NE expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        // save 6, 7 just in case
        out << push_rvalue(6, cur_offset);
        out << push_rvalue(7, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $6, $3, $5" << endl;
            out << "slt $7, $5, $3" << endl;
        } else {
            out << "sltu $6, $3, $5" << endl;
            out << "sltu $7, $5, $3" << endl;
        }
        out << "add $3, $6, $7" << endl;
        // return back 6, 7
        out << pop_rvalue(7, cur_offset);
        out << pop_rvalue(7, cur_offset);
        out << endl;
        return;
    }

    // test → expr EQ expr
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "EQ") {
        out << "; test to expr EQ expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        // save 6, 7 just in case
        out << push_rvalue(6, cur_offset);
        out << push_rvalue(7, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $6, $3, $5" << endl;
            out << "slt $7, $5, $3" << endl;
        } else {
            out << "sltu $6, $3, $5" << endl;
            out << "sltu $7, $5, $3" << endl;    
        }       
        out << "add $3, $6, $7" << endl;
        out << "sub $3, $11, $3" << endl;
        // return back 6, 7
        out << pop_rvalue(7, cur_offset);
        out << pop_rvalue(7, cur_offset);
        out << endl;
        return;
    }

    // test → expr LE expr
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "LE") {
        out << "; test to expr LE expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $3, $3, $5" << endl;
        } else {
            out << "sltu $3, $3, $5" << endl;  
        }  
        out << "sub $3, $11, $3" << endl;
        out << endl;
        return;
    }

    // test → expr GE expr
    if (curnode->token_kind == "test" && curnode->children[1]->token_kind == "GE") {
        out << "; test to expr GE expr <" << endl;
        codeGen(curnode->children[0], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << push_rvalue(3, cur_offset);
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << pop_rvalue(5, cur_offset);
        if (curnode->children[0]->type == "int") {
            out << "slt $3, $5, $3" << endl;
        } else {
            out << "sltu $3, $5, $3" << endl;
        }
        out << "sub $3, $11, $3" << endl;
        out << endl;
        return;
    }

    // statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
    if (curnode->token_kind == "statement" && curnode->children[0]->token_kind == "IF") {
        ; out << "; IF" << endl;
        int current_label = *label_counter;
        *label_counter = *label_counter + 1;
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "beq $3, $0, else" << current_label << endl;
        codeGen(curnode->children[5], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "beq $0, $0, endif" << current_label << endl;
        out << "else" << current_label << ": " << endl;
        codeGen(curnode->children[9], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "endif" << current_label << ": " << endl;
        out << endl;
        return;
    }

    // statement → WHILE LPAREN test RPAREN LBRACE statements RBRACE
    if (curnode->token_kind == "statement" && curnode->children[0]->token_kind == "WHILE") {
        out << "; while" << endl;
        int current_label = *label_counter;
        *label_counter = *label_counter + 1;
        out << "loop" << current_label << ": " << endl;
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "beq $3, $0, endWhile" << current_label << endl;
        codeGen(curnode->children[5], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "beq $0, $0, loop" << current_label << endl;
        out << "endWhile" << current_label << ": " << endl;
        out << endl;
        return;
    }

    // statement → PRINTLN LPAREN expr RPAREN SEMI
    if (curnode->token_kind == "statement" && curnode->children[0]->token_kind == "PRINTLN") {
        out << "; print" << endl;
        // preserve $1
        out << push_rvalue(1, cur_offset) << endl;
        codeGen(curnode->children[2], out, cur_offset, SymTable, MemoryStack, label_counter);
        out << "add $1, $3, $0" << endl;
        out << push_rvalue(31, cur_offset);
        out << "lis $5" << endl;
        out << ".word print" << endl;
        out << "jalr $5" << endl;
        out << pop_rvalue(31, cur_offset);
        out << pop_rvalue(1, cur_offset);
        out << endl;
        return;
    }

    /*
    else {
        cerr << "ERROR " << curnode->token_kind << endl;
        for (int i = 0; i < curnode->content.size(); i++) {
            cerr << curnode->content[i] << "  ";
        }
        cerr << endl;
    }
    */


}










int main() {

    istream& in = cin;
    string s;
    bool tree_build = false;
    Node* root;  // ?
    Node* cur_parent;
    ostream& out = cout;
    ostringstream oss;
    int offset = 0;
    int label_counter = 0;

    // variable name: (type, offset from $29)
    map<string, pair<string, int>> SymTable;
    vector<int> MemoryStack;

    // BUILDING TREE by reading input
    while (getline(in, s)) {

        // setup a line
        istringstream iss {s};
        vector<string> rhs;
        string lhs, rhs_str;
        iss >> lhs;
        bool need_annotate = false;
        string new_node_type = "";
        while (iss >> rhs_str) {
            // if this node has type information
            if (rhs_str == ":") {
                iss >> rhs_str;
                need_annotate = true;
                new_node_type = rhs_str;
                break;
            }
            rhs.emplace_back(rhs_str);
        }


        // if this is root
        if (!tree_build) {
            vector<Node*> vec;
            Node* new_root = new Node{lhs, rhs, need_annotate, new_node_type, vec};

            root = new_root;
            cur_parent = root;
            tree_build = true;
            continue;
        }

         // if this is not a root, find the correct parent node
        while (true) {
            if (find(cur_parent->content.begin(), cur_parent->content.end(), lhs) != cur_parent->content.end() && cur_parent->children.size() < cur_parent->content.size()) {
                break;
            }
            // error handling
            if (cur_parent->parent == NULL) {
                cerr << cur_parent->token_kind << "?";
                cerr << "ERR" << endl;
                release(root);
                /*
                for (auto it = symtable.begin(); it != symtable.end(); it++) {
                    delete it->second;
                }
                */
                return 1;
            }
            cur_parent = cur_parent->parent;
        }

        // setup new node
        vector<Node*> vec;
        Node* new_node = new Node{lhs, rhs, need_annotate, new_node_type, vec};
        new_node->parent = cur_parent;
        cur_parent->children.emplace_back(new_node);
        cur_parent = new_node;

    }
    
    
    // setup prologue
    out << ".import init" << endl;
    out << ".import new" << endl;
    out << ".import delete" << endl;
    out << ".import print" << endl;
    out << "lis $4" << endl;
    out << ".word 4" << endl;
    out << "lis $10" << endl;
    out << ".word print" << endl;
    out << "lis $11" << endl;
    out << ".word 1" << endl;
    out << "sub $29 , $30 , $4" << endl;
    out << "; end prologue" << endl << endl;
    out << "beq $0, $0, wain" << endl;
    

    
    // start code section
    codeGen(root, out, &offset, &SymTable, &MemoryStack, &label_counter);
    
    
    // setup epilogue
    out << "jr $31" << endl;
    
    
    
    
    
    // end of program
    // print_tree(root, 0);
    release(root);

}
