#include <iostream>
#include <fstream>
#include <sstream>
#include <string>  
#include <bits/stdc++.h>
#include <algorithm>
#include <random>
#include <cstdlib>

using namespace std;

#define LUT_AREA 3.5

struct RAM{
    int id;
    int circuit_id;
    string mode;
    int width;
    int depth;

    int lp_map_id; //to avoid search
    bool packed;
};

//store benchmark specification
struct Circuit{
    int num_of_logic;
    vector<RAM> ram;
    
    //used RAMS
    int num_of_BRAM;
    int num_of_12BRAM;
    int num_of_LUTRAM;
    //logic blocks in file
    int num_of_logic_elements;
    int additional_logic;
    //utilization rate
    double util_LUT;
    double util_BRAM;
    double util_12BRAM;

};

//A map from a logical RAM to physical one
struct Fit{
    int type;
    int req_logic;
    double req_area;
    int s;
    int p; 
    int mode;
};

//holds the whole mapping 
struct LP_map{
    int r_index;//RAM id
    int c_index;//circuit id
    
    int type;
    int s;
    int p;
    int mode;
    int additional_logic;

    int physical_ram_id;

    //leftovers
    int depth_left_1;
    int width_left_1;
    int depth_left_2;
    int width_left_2;
};


//global variables
Circuit* circuits;
int num_of_circuits;
vector <LP_map> lp_maps;
int lp_maps_id = 0;
ofstream checker_input("checker_input.txt");
bool* balance;
int* LUT_in_circuit;
int counter = 0;

int BRAM_MAX_WIDTH;
int BRAM_RATIO;
double BRAM_AREA;
int BRAM_BITS;

int small_BRAM_ratio[] = {5, 15, 10, 1, 20, 25};
int large_BRAM_ratio[] = {10, 20, 40, 80, 160, 300};


//init the circuits based on input file
void init(){
    circuits = new Circuit[num_of_circuits];
    balance = new bool[num_of_circuits];
    LUT_in_circuit = new int[num_of_circuits];
    for(int i = 0; i < num_of_circuits; i++){
        circuits[i].num_of_LUTRAM = 0;
        circuits[i].num_of_BRAM = 0;
        circuits[i].num_of_12BRAM = 0;
        circuits[i].num_of_logic_elements = 0;        
        circuits[i].util_LUT = 0;
        circuits[i].util_BRAM = 0;
        circuits[i].util_12BRAM = 0;
        circuits[i].additional_logic = 0;
        balance[i] = false;

    }
    
}

void parse_input(){
    ifstream testFile;
    testFile.open("logical_rams.txt");
    string input;

    //get the number of circuits in benchmark
    testFile >> input; 
    testFile >> num_of_circuits;
    
    init();

    //get the header line and ignore
    testFile >> input;//Circuit
    testFile >> input;//RamID
    testFile >> input;//Mode
    testFile >> input;//Depth
    testFile >> input;//Width
    
    int c_index = 0;
    while(!testFile.eof()){
        int inp;
        //circuit id
        testFile >> inp;
        //input file last line is empty
        if(inp < c_index){ 
            break;
        }
        if(inp != c_index){
            c_index = inp;
        }
        RAM ram_temp;
        ram_temp.circuit_id = inp;
        //RAM id
        testFile >> inp;
        ram_temp.id = inp;
        //RAM mode
        string mode;
        testFile >> mode;
        ram_temp.mode = mode;
        //RAM depth
        testFile >> inp;
        ram_temp.depth = inp;
        //RAM WIDTH
        testFile >> inp;
        ram_temp.width = inp;

        ram_temp.lp_map_id = -1;
        ram_temp.packed = false;

        circuits[c_index].ram.push_back(ram_temp);
    }

    testFile.close();
    
    //read the second file
    testFile.open("logic_block_count.txt");
    //get the header line and ignore
    testFile >> input;
    testFile >> input;
    testFile >> input;
    testFile >> input;
    testFile >> input;
    testFile >> input;
    testFile >> input;
    
    while (!testFile.eof()){
        int circuit_id;
        int circuit_logic_count;
        testFile >> circuit_id;
        testFile >> circuit_logic_count;
        circuits[circuit_id].num_of_logic = circuit_logic_count;
    }
    
    testFile.close();
}


void write_checker_file(RAM r, int circuit_id){
    
    int lp_index = r.lp_map_id;

    int w;
    int d;
    // if(lp_maps[lp_index].type == 0){
    //     if(lp_maps[lp_index].mode == 0){
    //         w = 10;
    //         d = 64;
    //     }
    //     else{
    //         w = 20;
    //         d = 32;
    //     }
    // }
    if(lp_maps[lp_index].type == 0){
        w = lp_maps[lp_index].mode;
        d = BRAM_BITS / lp_maps[lp_index].mode;;
    }

    // if(lp_maps[lp_index].type == 2){
    //     w = lp_maps[lp_index].mode;;
    //     d =  BRAM_128K_BITS / lp_maps[lp_index].mode;;
    // }

    string temp_mode = "";
    if(r.packed){
        temp_mode = "TrueDualPort";
    }
    else{
        temp_mode = r.mode;
    }
    
    string out = "";
    out += to_string(circuit_id) + " " + to_string(r.id) + " " + to_string(lp_maps[lp_index].additional_logic) + " ";
    out += "LW " + to_string(r.width) + " LD " + to_string(r.depth) + " ";
    out += "ID " + to_string(lp_maps[lp_index].physical_ram_id) + " S " + to_string(lp_maps[lp_index].s) + " P " + to_string(lp_maps[lp_index].p); 
    out += " Type " + to_string(lp_maps[lp_index].type+1) + " Mode " + temp_mode;
    out += " W " + to_string(w) + " D " + to_string(d);
    
    checker_input << out;
    checker_input << endl;
}


Fit best_mapping_on_BRAM(RAM r, bool re_map){
    //MAP TO 8K BRAM
    int required_BRAM8K = INT_MAX;
    int required_BRAM8K_logic = 0;
    double BRAM8K_required_area = DBL_MAX;
    int best_BRAM_mode = -1;
    int best_BRAM_p; 
    int best_BRAM_s;
    for(int x = 1 ; x <= BRAM_MAX_WIDTH; x *= 2){
        //x32 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == BRAM_MAX_WIDTH){
            continue;
        }
        int req_ram = ceil((double)r.width/x) * ceil((double)r.depth/(BRAM_BITS/x));

        int req_logic = 0;
        if(r.depth > (BRAM_BITS/x)){//extra logic needed
            if((double)r.depth/(BRAM_BITS/x) > 16){
                //can not be implemented
                req_ram = -1;
                req_logic = -1;
            }
            else{
                int num_of_mux = ceil(ceil((double)r.depth/(BRAM_BITS/x))/4.) + 1;
                if((int) ceil((double)r.depth/(BRAM_BITS/x)) % 4 == 1)
                    num_of_mux--;
                
                req_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/(BRAM_BITS/x));
                if(extra_rams == 2){
                    req_logic += 1; //only one LUT is required 
                }
                else{
                    extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
                    req_logic += extra_rams;
                } 
            }
        }

        if(req_logic >= 0 && req_ram >= 0){
            if(req_ram * BRAM_AREA + req_logic * LUT_AREA < BRAM8K_required_area){
                BRAM8K_required_area = req_ram * BRAM_AREA + req_logic * LUT_AREA;
                required_BRAM8K = req_ram;
                required_BRAM8K_logic = req_logic;
                best_BRAM_mode = x;
                best_BRAM_s = ceil((double)r.depth/(BRAM_BITS/x));
                best_BRAM_p = ceil((double)r.width/x);
            }
        }
    }


    if(r.mode == "TrueDualPort")
        required_BRAM8K_logic *= 2;

    Fit f_temp;
    f_temp.type = 1;
    f_temp.mode = best_BRAM_mode;
    f_temp.s = best_BRAM_s;
    f_temp.p = best_BRAM_p;
    f_temp.req_logic = required_BRAM8K_logic;
    f_temp.req_area = BRAM8K_required_area;
    return f_temp;
}

//map available logical RAMs to physical in Circuit c
void find_best_mapping(RAM r, int circuit_id, bool re_map){

    Fit BRAM8K_fit = best_mapping_on_BRAM(r,re_map);
    
    int best_type;
    int best_mode;
    double best_area;
    int LUT;
    int best_p;
    int best_s;


    best_area = BRAM8K_fit.req_area;
    best_mode = BRAM8K_fit.mode;
    best_type = 0;
    LUT = BRAM8K_fit.req_logic;
    best_p = BRAM8K_fit.p;
    best_s = BRAM8K_fit.s;

    if(!re_map){
        LP_map lp_map_temp;
        lp_map_temp.type = best_type;
        lp_map_temp.mode = best_mode;
        lp_map_temp.s = best_s;
        lp_map_temp.p = best_p;
        lp_map_temp.additional_logic = LUT;
        lp_map_temp.r_index = r.id;
        lp_map_temp.c_index = circuit_id;
        lp_map_temp.physical_ram_id = counter;
        counter++;
        
        lp_maps.push_back(lp_map_temp);
        circuits[circuit_id].ram[r.id].lp_map_id = lp_maps_id;
        circuits[circuit_id].num_of_logic_elements += LUT;
        circuits[circuit_id].num_of_BRAM += best_s*best_p;
        lp_maps_id++;
    }
    else{
        int lp_index = circuits[circuit_id].ram[r.id].lp_map_id;
        lp_maps[lp_index].type = best_type;
        lp_maps[lp_index].mode = best_mode;
        lp_maps[lp_index].s = best_s;
        lp_maps[lp_index].p = best_p;
        lp_maps[lp_index].additional_logic = LUT;

    }
    
}


//Check if we can put more than one logical RAM within a physical RAM
void cal_leftovers(){
    for(int i = 0; i < lp_maps.size(); i++){   
        int c_index = lp_maps[i].c_index;
        int r_index = lp_maps[i].r_index;

        //only single port RAM and ROM can be mapped within a same physical RAM
        if(circuits[c_index].ram[r_index].mode != "ROM" && circuits[c_index].ram[r_index].mode != "SinglePort")
            continue;
        
        int logical_ram_width = circuits[c_index].ram[r_index].width;
        int logical_ram_depth = circuits[c_index].ram[r_index].depth;

        int physical_ram_width;
        int physical_ram_depth;
        

        physical_ram_width = lp_maps[i].mode;
        physical_ram_depth = BRAM_BITS/lp_maps[i].mode;
        
        physical_ram_width = lp_maps[i].p * (physical_ram_width);
        physical_ram_depth = lp_maps[i].s * (physical_ram_depth);
    
        lp_maps[i].width_left_1 =  physical_ram_width - logical_ram_width;
        lp_maps[i].depth_left_1 =  physical_ram_depth;

        lp_maps[i].width_left_2 = physical_ram_width;
        lp_maps[i].depth_left_2 = physical_ram_depth - logical_ram_depth;
        
    } 
}

//Move the best candidate to share a physical RAM
void pack_leftovers(){
    for(int i = 0; i < lp_maps.size(); i++){
        
        int c_index = lp_maps[i].c_index;
        int r_index = lp_maps[i].r_index;

        if(circuits[c_index].ram[r_index].mode != "ROM" && circuits[c_index].ram[r_index].mode != "SinglePort")
            continue;

        if(lp_maps[i].width_left_1 * lp_maps[i].depth_left_1 == 0 && lp_maps[i].width_left_2 * lp_maps[i].depth_left_2 == 0){
            //no leftover
            continue;
        }

        //max width is not available in TrueDualMode
        if(lp_maps[i].type == 0 && lp_maps[i].mode == BRAM_MAX_WIDTH)
            continue;

        
        if(circuits[c_index].ram[r_index].packed){//already packed with another rom
            continue;
        }
        
        int max_ram_id_2 = -1;
        int max_depth_2 = INT_MAX;
        int max_width_2 = INT_MAX;
        
        for(int j = 0; j < circuits[lp_maps[i].c_index].ram.size(); j++){
            if(j == lp_maps[i].r_index){
                continue; //current mapping
            }
            if(circuits[lp_maps[i].c_index].ram[j].mode != "ROM" && circuits[lp_maps[i].c_index].ram[j].mode != "SinglePort"){
                continue;//no packing
            }
            
            
            int l_d = circuits[lp_maps[i].c_index].ram[j].depth;
            int l_w = circuits[lp_maps[i].c_index].ram[j].width;
            
            //leftover2
            int next_power_2_d = pow(2,ceil(log(l_d)/log(2)));
            if(l_w <= lp_maps[i].width_left_2 && next_power_2_d <= lp_maps[i].depth_left_2 && !circuits[lp_maps[i].c_index].ram[j].packed){
                if(max_depth_2 == INT_MAX && max_width_2 == INT_MAX){
                    max_depth_2 = l_d;
                    max_width_2 = l_w;
                    max_ram_id_2 = j;
                }
                else if(max_depth_2 * max_width_2 < l_d * l_w ){
                    max_depth_2 = l_d;
                    max_width_2 = l_w;
                    max_ram_id_2 = j;
                }
            }
        }
        if(max_ram_id_2 == -1){ //no candidate has been found
            continue;
        }
        else{
            
            //update the resource usage
            int locate_ram_in_lp = circuits[lp_maps[i].c_index].ram[max_ram_id_2].lp_map_id;
            circuits[lp_maps[i].c_index].num_of_logic_elements -= lp_maps[locate_ram_in_lp].additional_logic;
            switch(lp_maps[locate_ram_in_lp].type){
                case 0: circuits[lp_maps[i].c_index].num_of_LUTRAM -= lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                case 1: circuits[lp_maps[i].c_index].num_of_BRAM -=lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                case 2: circuits[lp_maps[i].c_index].num_of_12BRAM -= lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                default: break;
            }

            //change the mapping structure to the shared physical RAM
            circuits[lp_maps[i].c_index].ram[lp_maps[i].r_index].packed = true;
            circuits[lp_maps[i].c_index].ram[max_ram_id_2].packed = true;
            lp_maps[locate_ram_in_lp].physical_ram_id = lp_maps[i].physical_ram_id;
            lp_maps[locate_ram_in_lp].type = lp_maps[i].type;
            lp_maps[locate_ram_in_lp].mode = lp_maps[i].mode;
            lp_maps[locate_ram_in_lp].p = lp_maps[i].p;
            lp_maps[locate_ram_in_lp].s = lp_maps[i].s;
            lp_maps[locate_ram_in_lp].additional_logic = lp_maps[i].additional_logic;
        }
    }
}


long double get_geo_average(){
    long double g_average = 1;
    for(int i = 0; i < lp_maps.size(); i++){
        circuits[lp_maps[i].c_index].additional_logic += lp_maps[i].additional_logic; 
    }
    for(int i = 0 ; i < num_of_circuits; i++){
        long total_logic_in_circuit = max(circuits[i].num_of_logic + (int) ceil((double)circuits[i].additional_logic/10) ,circuits[i].num_of_BRAM*BRAM_RATIO);
        g_average *= total_logic_in_circuit;
    }
    return pow(g_average,(1.0/num_of_circuits));
}


int main(int argc, char** argv){
    
    BRAM_BITS = atoi(argv[1]);

    bool small = BRAM_BITS > 16384 ? false : true;

    //calculate area based on bits
    BRAM_AREA = 9000 + 5 * BRAM_BITS  + 90 * sqrt(BRAM_BITS) + 600 * 2 * BRAM_MAX_WIDTH; 
    BRAM_AREA /= 10000;

    //parse the input files
    parse_input();
    
    long double best_geo_average = DBL_MAX;
    int best_max_width;
    int best_ratio;
    if(small){
        bool re_map = false;
        for(int ratio = 0 ; ratio < 6; ratio++){
            BRAM_RATIO = small_BRAM_ratio[ratio];
            for(BRAM_MAX_WIDTH = 2; BRAM_MAX_WIDTH <= BRAM_BITS/256; BRAM_MAX_WIDTH *= 2){
                //find the initial mapping
                for(int i = 0 ; i < num_of_circuits; i++){
                    counter = 0;
                    for(int j = 0; j < circuits[i].ram.size(); j++){
                        find_best_mapping(circuits[i].ram[j],i,re_map);
                    }
                }
                re_map = true;
                //find some packing to do between two single ports or two ROM
                cal_leftovers();
                pack_leftovers();
                long double g_average = get_geo_average();
                if(g_average < best_geo_average){
                    best_geo_average = g_average;
                    best_max_width = BRAM_MAX_WIDTH;
                    best_ratio = BRAM_RATIO;
                    for(int i = 0; i < num_of_circuits; i++){
                        for(int j = 0; j < circuits[i].ram.size(); j++){
                                write_checker_file(circuits[i].ram[j],i);
                        }
                    }
                }

            }
        }
    }

    cout << best_geo_average << " " << best_max_width << " " << best_ratio << endl; 
    return 0;
}