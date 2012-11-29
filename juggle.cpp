//  juggle.cpp: Ryan Koven 11/28/12
//
#include <fstream>
#include <vector>
#include <sstream> //includes <string>

using namespace std;

typedef struct {
    string name;
    int    H;
    int    E;
    int    P;
} circuit;

typedef struct {
    string   name;
    int     H;
    int     E;
    int     P;
    vector<int> circuit_scores;
    vector<int> circuit_prefs; // list of the indices of the preferred circuits for the juggler.
} juggler;

string extract_name(const string& input){
    string name;
    int i = 2; //The name begins after the identifier ('C' or 'J') and a space.
    while (input[i] != ' ')
        name += input[i++];
    
    return name;
}

int extract_index(string input){
    int index;
    string index_string = "";
    int i = 1; // start after the identifier in the name.
    while (i < input.size())
        index_string += input[i++];
    if (!(istringstream(index_string) >> index)) index = -1;
    return index;
}

vector<int> extract_prefs(const string& input){
    vector<int> prefs;
    int i = 0;
    string next_pref = "";
    
    // Move the counter to the beginning of the preference list
    // 'C' will be found on a juggler line iff the counter finds
    // the start of the preferences list.
    while(input[i] != 'C') i++;
    
    while (i < input.length()){
        if (input[i] != ',')
            next_pref += input[i++];
        else{
            prefs.push_back(extract_index(next_pref));
            next_pref = "";
            i++;
        }
    }
    prefs.push_back(extract_index(next_pref));
    return prefs;
}

int find_attr(char attr, const string& input){
    int value;
    string result;
    int i = 0;
    
    while (input[i++] != attr) //move the counter to the ':' after the attribute you want to find.
        ;
    if (input[i + 2] != ' '){ // the attr value is either a 1 or 2 digit number.
        result += input[i + 1];
        result += input[i + 2];
    }
    else
        result = input[i + 1];
    
    if (!(istringstream(result) >> value)) value = -1;
    return value;
}

template <typename T>
T* parse_attributes(const string& input, T* new_guy){
    (*new_guy).name = extract_name(input);
    (*new_guy).H = find_attr('H', input);
    (*new_guy).E = find_attr('E', input);
    (*new_guy).P = find_attr('P', input);
    return new_guy;
}

void swap_jugglers(int moving_circuit_index, int moving_index, int bumped_circuit_index, int bumped_index, vector<vector<juggler*> >& list){
    juggler* temp;
    temp = list[bumped_circuit_index][bumped_index];
    list[bumped_circuit_index][bumped_index] = list[moving_circuit_index][moving_index];
    list[moving_circuit_index][moving_index] = temp;
    
}

string num_to_string(const int number){
    ostringstream converter;
    converter << number;
    return converter.str();
}

void print_assignments(vector<circuit*>& circuits, vector<juggler*>& jugglers, vector<vector<juggler*> >& list, char* file){
    ofstream output;
    output.open(file);
    string next_line = "";
    for (int i = 0; i < circuits.size(); i++){
        next_line += (*circuits[i]).name + " ";
        for (int j = 0; j < list[i].size(); j++){
            next_line += (*list[i][j]).name + " ";
            for (int k = 0; k < (*list[i][j]).circuit_prefs.size(); k++){
                next_line += "C" + num_to_string((*list[i][j]).circuit_prefs[k]);
                next_line += ":" + num_to_string((*list[i][j]).circuit_scores[k]);
                next_line += (k < (*list[i][j]).circuit_prefs.size() - 1) ? " " : "";
            }
            next_line += (j < list[i].size() - 1) ? "," : "";
        }
        next_line += "\n";
        output << next_line;
        next_line = "";
    }
    output.close();
}

void parse_file(vector<juggler*>& jugglers, vector<circuit*>& circuits, char* file){
    string line;
    int counter = 0;
    ifstream input(file);
    if (input.is_open()){
        while(input.good()){
            getline(input, line);
            switch(line[0]){
                case 'J':
                    jugglers.push_back(parse_attributes(line, new juggler));
                    (*jugglers[counter++]).circuit_prefs = extract_prefs(line);
                    break;
                case 'C':
                    circuits.push_back(parse_attributes(line, new circuit));
                    break;
            }
        }
    }
}

void calculate_circuit_scores(vector<juggler*>& jugglers, vector<circuit*>& circuits){
    for (int j = 0; j < jugglers.size(); j++){
        for (int k = 0; k < circuits.size(); k++){
            (*jugglers[j]).circuit_scores.push_back((*jugglers[j]).H*(*circuits[k]).H +
                                                    (*jugglers[j]).E*(*circuits[k]).E +
                                                    (*jugglers[j]).P*(*circuits[k]).P);
        }
    }
}

// An initial distribution of an assignment list is produced. Each circuit is
// allotted the proper number of jugglers without regard to interest or "score-fit"
void initialize_assignment_list(vector<vector<juggler*> >& assignment_list, vector<juggler*>& jugglers, vector<circuit*>& circuits){
    for (int j = 0; j < circuits.size(); j++)
        assignment_list.push_back(vector<juggler*>());
    int circuit_counter = 0;
    int juggler_counter = 0;
    int juggler_list = 0;
    while(circuit_counter < circuits.size()){
        while(juggler_counter < jugglers.size()/circuits.size()){
            assignment_list[circuit_counter].push_back(jugglers[juggler_list++]);
            juggler_counter++;
        }
        circuit_counter++;
        juggler_counter = 0;
    }
}

//The initial distribution of jugglers is refined by swapping jugglers based on interest
//and match score. If juggler A's preferences are not accommodated by the current distribution,
//we check the match scores of the jugglers currently assigned to juggler A's preferred circuits,
//and if juggler A beats at least one of the jugglers assigned to its preferred circuits in order
//of preference, we swap.
//
void refine(vector<vector<juggler*> >& assignment_list, vector<juggler*>& jugglers, vector<circuit*>& circuits){
    bool happy_here;
    bool still_while = true;
    while(still_while){
        still_while = false;
        int next_pref_index;
        for (int i = 0; i < assignment_list.size(); i++){
            for (int j = 0; j < assignment_list[i].size(); j++){
                juggler* next_juggler = assignment_list[i][j];
                for (int k = 0; k < (*next_juggler).circuit_prefs.size(); k++){
                    happy_here = false;
                    next_pref_index = (*next_juggler).circuit_prefs[k];
                    if (next_pref_index != i){
                        for (int p = 0; p < assignment_list[next_pref_index].size(); p++){
                            if ((*assignment_list[next_pref_index][p]).circuit_scores[next_pref_index] < (*next_juggler).   circuit_scores[next_pref_index]){
                                swap_jugglers(i, j, next_pref_index, p, assignment_list);
                                still_while = true;
                                happy_here = true;
                                
                            }
                        }
                    }
                    else
                        break;
                    if (happy_here)
                        break;
                }
            }
        }
    }
}

int main(int argc, const char* argv[])
{
    vector<juggler*> jugglers; // the list of jugglers
    vector<circuit*> circuits; // the list of circuits.
    vector<vector<juggler*> > assignment_list; //assignment_list[i] is the list of jugglers assigned to circuit[i].
    
    parse_file(jugglers, circuits, "jugglefest.txt");
    calculate_circuit_scores(jugglers, circuits);
    initialize_assignment_list(assignment_list, jugglers, circuits);
    refine(assignment_list, jugglers, circuits);
    print_assignments(circuits, jugglers, assignment_list, "output.txt");
    
    return 0;
}

