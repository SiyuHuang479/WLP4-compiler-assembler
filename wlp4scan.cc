#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
using namespace std;
const std::string ALPHABET    = ".ALPHABET";
const std::string STATES      = ".STATES";
const std::string TRANSITIONS = ".TRANSITIONS";
const std::string INPUT       = ".INPUT";
const std::string EMPTY       = ".EMPTY";

bool isChar(std::string s) {
  return s.length() == 1;
}
bool isRange(std::string s) {
  return s.length() == 3 && s[1] == '-';
}

bool notID(char c) {
    if ('0' <= c && c <= '9') {
        return false;
    }
    if ('a' <= c && c <= 'z') return false;
    if ('A' <= c && c <= 'Z') return false;
    return true;
}

// Locations in the program that you should modify to store the
// DFA information have been marked with four-slash comments:
//// (Four-slash comment)
int main() {
  ifstream dfa_file {"transitions.dfa"};
  std::istream& in = dfa_file;
  std::string s;
  vector<char> alphabet;
  vector<string> states;
  vector<string> accepting_states;
  map<pair<string, char>, string> transitions;
  string initial_state;


  std::getline(in, s); // Alphabet section (skip header)
  // Read characters or ranges separated by whitespace
  while(in >> s) {
    if (s == STATES) { 
      break; 
    } else {
      if (isChar(s)) {
        //// Variable 's[0]' is an alphabet symbol
        alphabet.emplace_back(s[0]);
      } else if (isRange(s)) {
        for(char c = s[0]; c <= s[2]; ++c) {
          //// Variable 'c' is an alphabet symbol
          alphabet.emplace_back(c);
        }
      } 
    }
  }

  std::getline(in, s); // States section (skip header)
  // Read states separated by whitespace
  while(in >> s) {
    if (s == TRANSITIONS) { 
      break; 
    } else {
      static bool initial = true;
      bool accepting = false;
      if (s.back() == '!' && !isChar(s)) {
        accepting = true;
        s.pop_back();
      }
      //// Variable 's' contains the name of a state
      if (initial) {
        //// The state is initial
        initial_state = s;
        initial = false;
      }
      if (accepting) {
        //// The state is accepting
        accepting_states.emplace_back(s);
      }
      states.emplace_back(s);
    }
  }

  std::getline(in, s); // Transitions section (skip header)
  // Read transitions line-by-line
  while(std::getline(in, s)) {
    if (s == INPUT) { 
      // Note: Since we're reading line by line, once we encounter the
      // input header, we will already be on the line after the header
      break; 
    } else {
      std::string fromState, symbols, toState;
      std::istringstream line(s);
      std::vector<std::string> lineVec;
      while(line >> s) {
        lineVec.push_back(s);
      }
      fromState = lineVec.front();
      toState = lineVec.back();
      for(int i = 1; i < lineVec.size()-1; ++i) {
        std::string s = lineVec[i];
        if (isChar(s)) {
          symbols += s;
        } else if (isRange(s)) {
          for(char c = s[0]; c <= s[2]; ++c) {
            symbols += c;
          }
        }
      }
      for ( char c : symbols ) {
        //// There is a transition from 'fromState' to 'toState' on 'c'
        transitions[make_pair(fromState, c)] = toState;
      }
    }
  }

  while (getline(cin, s)) {
    string cur_state = initial_state;
    int cur_index = 0;
    bool accept = false;
    bool isnumber = false;
    string prev_state;
    while (true) {
        
        if (cur_index >= s.length()) {
            accept = false;
            for (int i = 0; i < accepting_states.size(); i++) {
                if (cur_state == accepting_states[i]) {
                    accept = true;
                    break;
                }
            }
            if (accept) {
                if (cur_state != "q0") cout << cur_state << " " << s << endl;
            }
            else {
                cerr << "ERROR: end";
                return 1;
            }
            break;
        }

        char c = s[cur_index];
        bool legal_char = false;
        for (int i = 0; i < alphabet.size(); i++) {
          if (alphabet[i] == c) {
            legal_char = true;
            break;
          }
        }
        if (c == ' ') legal_char = true;
        if (!legal_char) {
          cerr << "ERROR: illegal char" << endl;
          return 1;
        }

        // if empty
        if (s[cur_index] == ' ') {
            accept = false;
            for (int i = 0; i < accepting_states.size(); i++) {
                if (cur_state == accepting_states[i]) {
                    accept = true;
                    break;
                }
            }
            if (accept) {
                if (cur_state != "q0") cout << cur_state << " " << s.substr(0, cur_index + 1) << endl;
            }
            else {
                cerr << "ERROR: empty";
                return 1;
            }
            s = s.substr(cur_index + 1);
            cur_index = 0;
            cur_state = initial_state;
            continue;
        }

        // identify whether the following token is a number
        if (!('0' <= c && c <= '9')) isnumber = false;
        if (cur_state == initial_state && c == 0 && s[cur_index + 1] == 0) {
            cerr << "ERROR: consecutive 0's" << endl;
            return 1;
        } 
        if ('0' < c && c <= '9' && cur_state == initial_state) isnumber = true;
        // if it is a number, check if it is out of range
        if (isnumber) {
            string temp = s.substr(0, cur_index + 1);
            int64_t n;
            istringstream iss {temp};
            iss >> n;
            if (n > 2147483647) {
                cerr << "ERROR: number out of bound" << endl;
                return 1;
            }
        }

        // check whether the following token is a special keyword
        if (cur_state == initial_state) {
            if (s.length() >= 6 && s.substr(0, 6) == "return") {
                if (s.length() == 6 || notID(s[6])) {
                    if (s.length() > 6) s = s.substr(6);
                    else s = "";
                    cur_index = 0;
                    cout << "RETURN return" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 2 && s.substr(0, 2) == "if") {
                if (s.length() == 2 || notID(s[2])) {
                    if (s.length() > 2) s = s.substr(2);
                    else s = "";
                    cur_index = 0;
                    cout << "IF if" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 4 && s.substr(0, 4) == "else") {
                if (s.length() == 4 || notID(s[4])) {
                    if (s.length() > 4) s = s.substr(4);
                    else s = "";
                    cur_index = 0;
                    cout << "ELSE else" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 5 && s.substr(0, 5) == "while") {
                if (s.length() == 5 || notID(s[5])) {
                    if (s.length() > 5) s = s.substr(5);
                    else s = "";
                    cur_index = 0;
                    cout << "WHILE while" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 7 && s.substr(0, 7) == "println") {
                if (s.length() == 7 || notID(s[7])) {
                    if (s.length() > 7) s = s.substr(7);
                    else s = "";
                    cur_index = 0;
                    cout << "PRINTLN println" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 4 && s.substr(0, 4) == "wain") {
                if (s.length() == 4 || notID(s[4])) {
                    if (s.length() > 4) s = s.substr(4);
                    else s = "";
                    cur_index = 0;
                    cout << "WAIN wain" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 3 && s.substr(0, 3) == "int") {
                if (s.length() == 3 || notID(s[3])) {
                    if (s.length() > 3) s = s.substr(3);
                    else s = "";
                    cur_index = 0;
                    cout << "INT int" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 4 && s.substr(0, 4) == "NULL") {
                if (s.length() == 4 || notID(s[4])) {
                    if (s.length() > 4) s = s.substr(4);
                    else s = "";
                    cur_index = 0;
                    cout << "NULL NULL" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 3 && s.substr(0, 3) == "new") {
                if (s.length() == 3 || notID(s[3])) {
                    if (s.length() > 3) s = s.substr(3);
                    else s = "";
                    cur_index = 0;
                    cout << "NEW new" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 6 && s.substr(0, 6) == "delete") {
                if (s.length() == 6 || notID(s[6])) {
                    if (s.length() > 6) s = s.substr(6);
                    else s = "";
                    cur_index = 0;
                    cout << "DELETE delete" << endl;
                    cur_state = initial_state;
                    continue;
                }
            }
            if (s.length() >= 2 && s.substr(0, 2) == "//") {
                break;
            }
        }

        if (transitions.count(make_pair(cur_state, c)) < 1) {
            accept = false;
            for (int i = 0; i < accepting_states.size(); i++) {
                if (cur_state == accepting_states[i]) {
                    accept = true;
                    break;
                }
            }       
            if (accept) {
                cout << cur_state << " " << s.substr(0, cur_index) << endl;
                cur_state = initial_state;
                s = s.substr(cur_index);
                cur_index = 0;
                continue;
            } else {
                cerr << "ERROR: halfway";
                return 1;
            }
        } else {
            cur_state = transitions[make_pair(cur_state, s[cur_index])];
            cur_index++;
        }

        
    }
  }





    

  
}
