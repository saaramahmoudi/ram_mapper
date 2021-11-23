#include <iostream>
#include <fstream>
#include <sstream>
#include <string>  
#include <bits/stdc++.h>
#include <algorithm>
#include <random>
#include <cstdlib>

#define LUTRAM_MODE_0_WORDS 64 
#define LUTRAM_MODE_1_WORDS 32
#define LUTRAM_MODE_0_WIDTH 10
#define LUTRAM_MODE_1_WIDTH 20

#define LUTRAM_AREA 4
#define LUT_AREA 3.5



using namespace std;

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
    int num_of_BRAM_type_1;
    int num_of_BRAM_type_2;
    int num_of_LUTRAM;
    //logic blocks in file
    int num_of_logic_elements;
    int additional_logic;
    //utilization rate
    double util_LUT;
    double util_BRAM_type_1;
    double util_BRAM_type_2;

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

int BRAM_type_1_BITS;
int BRAM_type_1_MAX_WIDTH;
int BRAM_type_1_RATIO;
int LUTRAM_RATIO; 
int BRAM_type_2_BITS;
int BRAM_type_2_MAX_WIDTH;
int BRAM_type_2_RATIO;

double BRAM_type_1_AREA;
double BRAM_type_2_AREA;

//init the circuits based on input file
void init(){
    circuits = new Circuit[num_of_circuits];
    balance = new bool[num_of_circuits];
    LUT_in_circuit = new int[num_of_circuits];
    for(int i = 0; i < num_of_circuits; i++){
        circuits[i].num_of_LUTRAM = 0;
        circuits[i].num_of_BRAM_type_1 = 0;
        circuits[i].num_of_BRAM_type_2 = 0;
        circuits[i].num_of_logic_elements = 0;        
        circuits[i].util_LUT = 0;
        circuits[i].util_BRAM_type_1 = 0;
        circuits[i].util_BRAM_type_2 = 0;
        circuits[i].additional_logic = 0;
        balance[i] = false;

    }
    
}

//calculate utilization for each circuit
void get_memory_utilization(){  
    for(int i = 0 ;  i < num_of_circuits; i++){
        
        int lut_by_BRAM_type_1 = circuits[i].num_of_BRAM_type_1 * BRAM_type_1_RATIO;
        int lut_by_BRAM_type_2 = circuits[i].num_of_BRAM_type_2 * BRAM_type_2_RATIO;
        int lut_block = max(circuits[i].num_of_logic + circuits[i].num_of_LUTRAM, LUTRAM_RATIO * circuits[i].num_of_LUTRAM);

        if(lut_block >= lut_by_BRAM_type_2 && lut_block >= lut_by_BRAM_type_1){
            LUT_in_circuit[i] = lut_block;
        }
        else if(lut_by_BRAM_type_1 >= lut_by_BRAM_type_2){
            LUT_in_circuit[i] = lut_by_BRAM_type_1;
        }
        else{
            LUT_in_circuit[i] = lut_by_BRAM_type_2;
        }

        circuits[i].util_LUT = (double) (circuits[i].num_of_LUTRAM + circuits[i].num_of_logic)/LUT_in_circuit[i]; 
        circuits[i].util_BRAM_type_1 = (double) (circuits[i].num_of_BRAM_type_1/ (ceil((double) LUT_in_circuit[i] / BRAM_type_1_RATIO)));         
        circuits[i].util_BRAM_type_2 = (double) (circuits[i].num_of_BRAM_type_2/ (ceil((double) LUT_in_circuit[i] / BRAM_type_2_RATIO)));         
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
    if(lp_maps[lp_index].type == 0){
        if(lp_maps[lp_index].mode == 0){
            w = 10;
            d = 64;
        }
        else{
            w = 20;
            d = 32;
        }
    }
    if(lp_maps[lp_index].type == 1){
        w = lp_maps[lp_index].mode;
        d = BRAM_type_1_BITS / lp_maps[lp_index].mode;;
    }

    if(lp_maps[lp_index].type == 2){
        w = lp_maps[lp_index].mode;;
        d =  BRAM_type_2_BITS / lp_maps[lp_index].mode;;
    }

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


Fit best_mapping_on_LUTRAM(RAM r, bool re_map){
    //MAP TO LUTRAM 
    int required_LUTRAM = -1;
    int required_LUTRAM_logic = -1;
    double LUTRAM_required_area = -1;
    int best_LUTRAM_mode = -1;
    int best_LUTRAM_s = 0;
    int best_LUTRAM_p = 0;
    if(r.mode != "TrueDualPort"){
        //LUTRAM MODE 0
        int lut_mode_0 = 0;
        int lut_mode_0_logic = 0;
        int lut_mode_0_p = 0;
        int lut_mode_0_s = 0;
        
        if((double)r.depth/LUTRAM_MODE_0_WORDS > 16){
            //can not be implemented 
            lut_mode_0 = -1; 
            lut_mode_0_logic = -1;
        }
        else{
            if(r.width < LUTRAM_MODE_0_WIDTH){
                lut_mode_0++;
                lut_mode_0_p++;
            }
            else{
                lut_mode_0 += ceil((double)r.width/LUTRAM_MODE_0_WIDTH);
                lut_mode_0_p += ceil((double)r.width/LUTRAM_MODE_0_WIDTH);

            }
            //extra logic is needed
            lut_mode_0_s = 1;
            if(r.depth > LUTRAM_MODE_0_WORDS){
                lut_mode_0 *= ceil((double)r.depth/LUTRAM_MODE_0_WORDS);
                lut_mode_0_s = ceil((double)r.depth/LUTRAM_MODE_0_WORDS);
                //mux logic
                int num_of_mux = ceil((double)r.depth/LUTRAM_MODE_0_WORDS/4) + 1;
                if((int) ceil((double)r.depth/LUTRAM_MODE_0_WORDS) % 4 == 1){
                    num_of_mux--;           
                }

                lut_mode_0_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/LUTRAM_MODE_0_WORDS);
                if(extra_rams == 2){
                    lut_mode_0_logic += 1; //only one LUT is required 
                }
                else{
                    // extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
                    lut_mode_0_logic += extra_rams;
                }

            }
        }
        //LUTRAM MODE 1
        int lut_mode_1 = 0;
        int lut_mode_1_logic = 0;
        int lut_mode_1_p = 0;
        int lut_mode_1_s = 0;
        if((double)r.depth/LUTRAM_MODE_1_WORDS > 16){
            //can not be implemented 
            lut_mode_1 = -1; 
            lut_mode_1_logic = -1;
        }
        else{
            if(r.width < LUTRAM_MODE_1_WIDTH){
                lut_mode_1++;
                lut_mode_1_p++;
            }
            else{
                lut_mode_1 += ceil((double)r.width/LUTRAM_MODE_1_WIDTH);
                lut_mode_1_p += ceil((double)r.width/LUTRAM_MODE_1_WIDTH);
            }
            //extra logic needed
            lut_mode_1_s = 1;
            if(r.depth > LUTRAM_MODE_1_WORDS){
                lut_mode_1 *= ceil((double)r.depth/LUTRAM_MODE_1_WORDS);
                lut_mode_1_s = ceil((double)r.depth/LUTRAM_MODE_1_WORDS);
                //mux logic
                int num_of_mux = (ceil((double)r.depth/LUTRAM_MODE_1_WORDS/4)) + 1;
                if((int) ceil((double)r.depth/LUTRAM_MODE_1_WORDS) % 4 == 1)
                    num_of_mux--;
                
                lut_mode_1_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/LUTRAM_MODE_1_WORDS);
                if(extra_rams == 2){
                    lut_mode_1_logic += 1; //only one LUT is required 
                }
                else{
                    extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
                    lut_mode_1_logic += extra_rams;
                }
            }
        }

        if(lut_mode_0 < 0){
            required_LUTRAM = lut_mode_1;
            required_LUTRAM_logic = lut_mode_1_logic;
            best_LUTRAM_mode = 1;
            best_LUTRAM_s = lut_mode_1_s;
            best_LUTRAM_p = lut_mode_1_p;
        }
        if(lut_mode_1 < 0){
            required_LUTRAM = lut_mode_0;
            required_LUTRAM_logic = lut_mode_0_logic;
            best_LUTRAM_mode = 0;
            best_LUTRAM_s = lut_mode_0_s;
            best_LUTRAM_p = lut_mode_0_p;
        }
        if(lut_mode_1 >= 0 && lut_mode_0 >= 0){
            if((LUTRAM_AREA*lut_mode_0+ LUT_AREA*lut_mode_0_logic) > (LUTRAM_AREA*lut_mode_1+LUT_AREA*lut_mode_1_logic)){
                required_LUTRAM = lut_mode_1;
                required_LUTRAM_logic = lut_mode_1_logic;
                best_LUTRAM_mode = 1;
                best_LUTRAM_s = lut_mode_1_s;
                best_LUTRAM_p = lut_mode_1_p;
            }
            else{
                required_LUTRAM = lut_mode_0;
                required_LUTRAM_logic = lut_mode_0_logic;
                best_LUTRAM_mode = 0;
                best_LUTRAM_s = lut_mode_0_s;
                best_LUTRAM_p = lut_mode_0_p;
            }
        }
    }   
    //MIN area required by LUTRAM
    if(required_LUTRAM >= 0 && required_LUTRAM_logic >= 0){
        LUTRAM_required_area = LUTRAM_AREA * required_LUTRAM + LUT_AREA * required_LUTRAM_logic;
    }

    Fit f_temp;
    f_temp.type  = 0;
    f_temp.req_area = LUTRAM_required_area;
    f_temp.req_logic = required_LUTRAM_logic;
    f_temp.p = best_LUTRAM_p;
    f_temp.s = best_LUTRAM_s;
    f_temp.mode = best_LUTRAM_mode;
    return f_temp;
}


Fit best_mapping_on_BRAM_type_1(RAM r, bool re_map){
    //MAP TO _type_1 BRAM
    int required_BRAM_type_1 = INT_MAX;
    int required_BRAM_type_1_logic = 0;
    double BRAM_type_1_required_area = DBL_MAX;
    int bestBRAM_type_1_mode = -1;
    int bestBRAM_type_1_p; 
    int bestBRAM_type_1_s;
    for(int x = 1 ; x <= BRAM_type_1_MAX_WIDTH; x *= 2){
        //x32 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == BRAM_type_1_MAX_WIDTH){
            continue;
        }
        int req_ram = ceil((double)r.width/x) * ceil((double)r.depth/(BRAM_type_1_BITS/x));

        int req_logic = 0;
        if(r.depth > (BRAM_type_1_BITS/x)){//extra logic needed
            if((double)r.depth/(BRAM_type_1_BITS/x) > 16){
                //can not be implemented
                req_ram = -1;
                req_logic = -1;
            }
            else{
                int num_of_mux =  ceil((double)r.depth/(BRAM_type_1_BITS/x)/4) + 1;
                // (ceil((double)r.depth/(BRAM_type_1_BITS/x))/4) + 1;
                if((int) ceil((double)r.depth/(BRAM_type_1_BITS/x)) % 4 == 1)
                    num_of_mux--;
                
                req_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/(BRAM_type_1_BITS/x));
                if(extra_rams == 2){
                    req_logic += 1; //only one LUT is required 
                }
                else{
                    // extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
                    req_logic += extra_rams;
                } 
            }
        }

        if(req_logic >= 0 && req_ram >= 0){
            if(req_ram * BRAM_type_1_AREA + req_logic * LUT_AREA < BRAM_type_1_required_area){
                BRAM_type_1_required_area = req_ram * BRAM_type_1_AREA + req_logic * LUT_AREA;
                required_BRAM_type_1 = req_ram;
                required_BRAM_type_1_logic = req_logic;
                bestBRAM_type_1_mode = x;
                bestBRAM_type_1_s = ceil((double)r.depth/(BRAM_type_1_BITS/x));
                bestBRAM_type_1_p = ceil((double)r.width/x);
            }
        }
    }


    if(r.mode == "TrueDualPort")
        required_BRAM_type_1_logic *= 2;

    Fit f_temp;
    f_temp.type = 1;
    f_temp.mode = bestBRAM_type_1_mode;
    f_temp.s = bestBRAM_type_1_s;
    f_temp.p = bestBRAM_type_1_p;
    f_temp.req_logic = required_BRAM_type_1_logic;
    f_temp.req_area = BRAM_type_1_required_area;
    return f_temp;
}

Fit best_mapping_on_BRAM_type_2(RAM r, bool re_map){
    //MAP TO 128 BRAM
    int required_BRAM_type_2 = INT_MAX;
    int required_BRAM_type_2_logic = 0;
    double BRAM_type_2_required_area = DBL_MAX;
    int best_BRAM_type_2_mode = -1;
    int best_BRAM_type_2_p;
    int best_BRAM_type_2_s;
    for(int x = 1 ; x <= BRAM_type_2_MAX_WIDTH; x *= 2){
        //x128 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == BRAM_type_2_MAX_WIDTH){
            continue;
        }
        int req_ram = ceil((double)r.width/x) * ceil((double)r.depth/(BRAM_type_2_BITS/x));
        int req_logic = 0;
        if(r.depth > (BRAM_type_2_BITS/x)){//extra logic needed
            if((double)r.depth/(BRAM_type_2_BITS/x) > 16){
                //can not be implemented
                req_ram = -1;
                req_logic = -1;
            }
            else{
                int num_of_mux =  ceil((double)r.depth/(BRAM_type_2_BITS/x)/4) + 1;
                
                if((int) ceil((double)r.depth/(BRAM_type_2_BITS/x)) % 4 == 1)
                    num_of_mux--;
                
                req_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/(BRAM_type_2_BITS/x));
                if(extra_rams == 2){
                    req_logic += 1; //only one LUT is required 
                }
                else{
                    // extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
                    req_logic += extra_rams;
                } 
            }
        }
  
        if(req_logic >= 0 && req_ram >= 0){
            if(req_ram * BRAM_type_2_AREA + req_logic * LUT_AREA < BRAM_type_2_required_area){
                BRAM_type_2_required_area = req_ram * BRAM_type_2_AREA + req_logic * LUT_AREA;
                required_BRAM_type_2 = req_ram;
                required_BRAM_type_2_logic = req_logic;
                best_BRAM_type_2_mode = x;
                best_BRAM_type_2_s = ceil((double)r.depth/(BRAM_type_2_BITS/x));
                best_BRAM_type_2_p = ceil((double)r.width/x);
            }
        }
    }

    Fit f_temp;
    if(r.mode == "TrueDualPort")
        required_BRAM_type_2_logic *= 2;

    f_temp.type = 2;
    f_temp.mode = best_BRAM_type_2_mode;
    f_temp.s = best_BRAM_type_2_s;
    f_temp.p = best_BRAM_type_2_p;
    f_temp.req_logic = required_BRAM_type_2_logic;
    f_temp.req_area = BRAM_type_2_required_area;
    return f_temp;

}


//map available logical RAMs to physical in Circuit c
void find_best_mapping(RAM r, int circuit_id, bool re_map){
    
    Fit LUTRAM_fit = best_mapping_on_LUTRAM(r,re_map);
    Fit BRAM_type_1_fit = best_mapping_on_BRAM_type_1(r,re_map);
    Fit BRAM_type_2_fit = best_mapping_on_BRAM_type_2(r,re_map);

    //choose the best solution
    if(LUTRAM_fit.req_area < 0){
        LUTRAM_fit.req_area = DBL_MAX;
    }
    int best_type;
    int best_mode;
    double best_area;
    int LUT;
    int best_p;
    int best_s;

    if(LUTRAM_fit.req_area < BRAM_type_1_fit.req_area){
        if(LUTRAM_fit.req_area <= BRAM_type_2_fit.req_area){
            best_area = LUTRAM_fit.req_area;
            best_mode = LUTRAM_fit.mode;
            best_type = 0;
            LUT = LUTRAM_fit.req_logic;
            best_p = LUTRAM_fit.p;
            best_s = LUTRAM_fit.s;
            
        }
        else if(BRAM_type_2_fit.req_area < LUTRAM_fit.req_area){
            best_area = BRAM_type_2_fit.req_area;
            best_mode = BRAM_type_2_fit.mode;
            best_type = 2;
            LUT = BRAM_type_2_fit.req_logic;
            best_p = BRAM_type_2_fit.p;
            best_s = BRAM_type_2_fit.s;
        }
    }

    if(LUTRAM_fit.req_area > BRAM_type_1_fit.req_area){
        if(BRAM_type_1_fit.req_area <= BRAM_type_2_fit.req_area){
            best_area = BRAM_type_1_fit.req_area;
            best_mode = BRAM_type_1_fit.mode;
            best_type = 1;
            LUT = BRAM_type_1_fit.req_logic;
            best_p = BRAM_type_1_fit.p;
            best_s = BRAM_type_1_fit.s;

        }
        else if(BRAM_type_1_fit.req_area > BRAM_type_2_fit.req_area){
            best_area = BRAM_type_2_fit.req_area;
            best_mode = BRAM_type_2_fit.mode;
            best_type = 2;
            LUT = BRAM_type_2_fit.req_logic;
            best_p = BRAM_type_2_fit.p;
            best_s = BRAM_type_2_fit.s;
        }
    }
    
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
        switch(best_type){
            case 0: circuits[circuit_id].num_of_LUTRAM += best_s*best_p; break;
            case 1: circuits[circuit_id].num_of_BRAM_type_1 += best_s*best_p; break;
            case 2: circuits[circuit_id].num_of_BRAM_type_2 += best_s*best_p; break;
            default : break;
        }
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
        
        //if single port RAM and ROM are in LUTRAM, they can not be packed!
        if(lp_maps[i].type == 0)
            continue;
        
        int logical_ram_width = circuits[c_index].ram[r_index].width;
        int logical_ram_depth = circuits[c_index].ram[r_index].depth;

        int physical_ram_width;
        int physical_ram_depth;
        
        switch(lp_maps[i].type){          
            case 1:
                physical_ram_width = lp_maps[i].mode;
                physical_ram_depth = BRAM_type_1_BITS/lp_maps[i].mode;
                break;
            case 2:
                physical_ram_width = lp_maps[i].mode;
                physical_ram_depth = BRAM_type_2_BITS/lp_maps[i].mode;
                break;
            default:
                physical_ram_width = 0;
                physical_ram_depth = 0;
                break;
        }

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

         // can not pack in LUTRAMs
        if(lp_maps[i].type == 0)
            continue;

        //max width is not available in TrueDualMode
        if(lp_maps[i].type == 1 && lp_maps[i].mode == BRAM_type_1_MAX_WIDTH)
            continue;

        if(lp_maps[i].type == 2 && lp_maps[i].mode == BRAM_type_2_MAX_WIDTH)
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
                case 1: circuits[lp_maps[i].c_index].num_of_BRAM_type_1 -=lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                case 2: circuits[lp_maps[i].c_index].num_of_BRAM_type_2 -= lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
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

//Descending order
bool compare_two_Logical_RAM(const LP_map &lp1, const LP_map &lp2){
    int l1_width = circuits[lp1.c_index].ram[lp1.r_index].width;
    int l1_depth = circuits[lp1.c_index].ram[lp1.r_index].depth;

    int l2_width = circuits[lp2.c_index].ram[lp2.r_index].width;
    int l2_depth = circuits[lp2.c_index].ram[lp2.r_index].depth;

    return l1_width * l1_depth > l2_width * l2_depth;
}


//returns logical RAMs mapped to LUTRAMs within a circuit 
vector<LP_map> get_LUTRAMS(int circuit_id){
    vector<LP_map> LUTRAMS;
    for(int i = 0; i < lp_maps.size(); i++){
        if(lp_maps[i].type != 0 || lp_maps[i].c_index < circuit_id)
            continue;
        if(lp_maps[i].c_index > circuit_id)
            break;
        else{
            LUTRAMS.push_back(lp_maps[i]);
        }
    }
    return LUTRAMS;
}


//update the available resources
void update_LUT_number(int c_index){
    int lut_by_BRAM_type_1 = circuits[c_index].num_of_BRAM_type_1 * BRAM_type_1_RATIO;
    int lut_by_BRAM_type_2 = circuits[c_index].num_of_BRAM_type_2 * BRAM_type_2_RATIO;
    int lut_block = max(circuits[c_index].num_of_logic + circuits[c_index].num_of_LUTRAM, LUTRAM_RATIO * circuits[c_index].num_of_LUTRAM);

    if(lut_block >= lut_by_BRAM_type_2 && lut_block >= lut_by_BRAM_type_1){
        LUT_in_circuit[c_index] = lut_block;
    }
    else if(lut_by_BRAM_type_1 >= lut_by_BRAM_type_2){
        LUT_in_circuit[c_index] = lut_by_BRAM_type_1;
    }
    else{
        LUT_in_circuit[c_index] = lut_by_BRAM_type_2;
    }

    circuits[c_index].util_LUT = (double) (circuits[c_index].num_of_LUTRAM + circuits[c_index].num_of_logic)/LUT_in_circuit[c_index]; 
    circuits[c_index].util_BRAM_type_1 = (double) (circuits[c_index].num_of_BRAM_type_1/ (ceil((double) LUT_in_circuit[c_index] / BRAM_type_1_RATIO)));         
    circuits[c_index].util_BRAM_type_2 = (double) (circuits[c_index].num_of_BRAM_type_2/ (ceil((double) LUT_in_circuit[c_index] / BRAM_type_2_RATIO)));   
}

//move from LUTRAM to 2BRAM_type_1/BRAM_type_1
void balance_LUTRAM_to_BRAM(){
    //loop over all circuits
    for(int i = 0; i < num_of_circuits; i++){
        if(circuits[i].util_LUT <= circuits[i].util_BRAM_type_1 || circuits[i].util_LUT <= circuits[i].util_BRAM_type_2)
            continue;

        balance[i] = true;

        //get the logical RAMs mapped to LUTRAM and sort them
        vector<LP_map> LUTRAMS = get_LUTRAMS(i);
        sort(LUTRAMS.begin(),LUTRAMS.end(),compare_two_Logical_RAM);

        //calculate the number of available resources to move from LUTRAM to these resources
        update_LUT_number(i);
        int avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
        int avail_BRAM_type_2 = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
    
        //loop over the candidates 
        for(int j = 0; j < LUTRAMS.size(); j++){
            //if nothing left to fill break the process
            if((avail_BRAM_type_1 == 0 && avail_BRAM_type_2 == 0))
                break;

            int moving_r_index = LUTRAMS[j].r_index;
            Fit BRAM_type_2_fit = best_mapping_on_BRAM_type_2(circuits[i].ram[moving_r_index],true);
            Fit BRAM_type_1_fit = best_mapping_on_BRAM_type_1(circuits[i].ram[moving_r_index],true);
            
            if(BRAM_type_2_fit.s * BRAM_type_2_fit.p <= avail_BRAM_type_2){
                //increase the number of BRAM_type_2
                circuits[i].num_of_BRAM_type_2 += BRAM_type_2_fit.s * BRAM_type_2_fit.p;
                //reducing the number of LUTRAM
                circuits[i].num_of_LUTRAM -= LUTRAMS[j].s * LUTRAMS[j].p;
                
                //update since number of resources changed
                update_LUT_number(i);
                avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                avail_BRAM_type_2 = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;

                if(avail_BRAM_type_2 >= 0 && circuits[i].util_BRAM_type_2 < circuits[i].util_LUT){
                    //moving process - update the mapping structure
                    int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                    lp_maps[lp_index].type = BRAM_type_2_fit.type;
                    lp_maps[lp_index].mode = BRAM_type_2_fit.mode;
                    lp_maps[lp_index].s = BRAM_type_2_fit.s;
                    lp_maps[lp_index].p = BRAM_type_2_fit.p;
                    lp_maps[lp_index].additional_logic = BRAM_type_2_fit.req_logic;
                    continue;
                }
                else{
                    //if moving is not possible, redo the changes
                    circuits[i].num_of_BRAM_type_2 -= BRAM_type_2_fit.s * BRAM_type_2_fit.p;
                    circuits[i].num_of_LUTRAM += LUTRAMS[j].s * LUTRAMS[j].p;
                    update_LUT_number(i);
                    avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                    avail_BRAM_type_2 = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                }
            }
            if(BRAM_type_1_fit.s * BRAM_type_1_fit.p <= avail_BRAM_type_1){
                //increase the number of BRAM_type_1
                circuits[i].num_of_BRAM_type_1 += BRAM_type_1_fit.s * BRAM_type_1_fit.p;
                //reducing the number of LUTRAM
                circuits[i].num_of_LUTRAM -= LUTRAMS[j].s * LUTRAMS[j].p;
                
                //update since number of resources changed
                update_LUT_number(i);
                avail_BRAM_type_1   = ceil((double)(LUT_in_circuit[i]/BRAM_type_1_RATIO)) - circuits[i].num_of_BRAM_type_1;
                avail_BRAM_type_2 = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;

                if(avail_BRAM_type_1 >= 0 && circuits[i].util_BRAM_type_1 < circuits[i].util_LUT){
                    //moving process - update the mapping structure
                    int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                    lp_maps[lp_index].type = BRAM_type_1_fit.type;
                    lp_maps[lp_index].mode = BRAM_type_1_fit.mode;
                    lp_maps[lp_index].s = BRAM_type_1_fit.s;
                    lp_maps[lp_index].p = BRAM_type_1_fit.p;
                    lp_maps[lp_index].additional_logic = BRAM_type_1_fit.req_logic;
                    continue;
                }
                else{
                    //if moving is not possible, redo the changes
                    circuits[i].num_of_BRAM_type_1 -= BRAM_type_1_fit.s * BRAM_type_1_fit.p;
                    circuits[i].num_of_LUTRAM += LUTRAMS[j].s * LUTRAMS[j].p;
                    update_LUT_number(i);
                    avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                    avail_BRAM_type_2 = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                }  
            }
        }
    }
}

//Ascending order
bool compare_two_Logical_RAM_ascending(const LP_map &lp1, const LP_map &lp2){
    int l1_width = circuits[lp1.c_index].ram[lp1.r_index].width;
    int l1_depth = circuits[lp1.c_index].ram[lp1.r_index].depth;

    int l2_width = circuits[lp2.c_index].ram[lp2.r_index].width;
    int l2_depth = circuits[lp2.c_index].ram[lp2.r_index].depth;

    return l1_width * l1_depth < l2_width * l2_depth;
}

//returns logical RAMs mapped to BRAM within a circuit
//type 1 : BRAM_type_1 
//type 2 : BRAM_type_2 
vector<LP_map> get_BRAMS(int circuit_id, int type){
    vector<LP_map> BRAMS;
    for(int i = 0; i < lp_maps.size(); i++){
        if(lp_maps[i].type != type || lp_maps[i].c_index < circuit_id)
            continue;
        if(lp_maps[i].c_index > circuit_id)
            break;
        else{
            BRAMS.push_back(lp_maps[i]);
        }
    }
    return BRAMS;
}

//move from BRAM_type_2 to LUTRAM/BRAM_type_1
void balance_BRAM_type_2(){
    //loop over all circuits
    for(int i = 0; i < num_of_circuits; i++){
        //if the circuit is already balanced, continue
        if(balance[i])
            continue;

        //no need to move from _type_2 to other RAMs
        if(circuits[i].util_BRAM_type_2 <= circuits[i].util_BRAM_type_1 || circuits[i].util_BRAM_type_2 <= circuits[i].util_LUT)
            continue;
        
        balance[i] = true;

        //get the logical RAMs mapped to BRAM_type_2 and sort them
        vector<LP_map> BRAMS_type_2 = get_BRAMS(i,2);
        sort(BRAMS_type_2.begin(),BRAMS_type_2.end(),compare_two_Logical_RAM_ascending);
        
        //calculate the number of available resources to move from BRAM128 to these resources
        int avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
        int avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/ BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
        
        for(int j = 0; j < BRAMS_type_2.size(); j++){
            //no need to keep moving since it becomes balanced
            if((circuits[i].util_BRAM_type_1 >= circuits[i].util_BRAM_type_2 && circuits[i].util_LUT >= circuits[i].util_BRAM_type_2))
                break;
            
            int moving_r_index = BRAMS_type_2[j].r_index;

            if(circuits[i].ram[moving_r_index].packed)
                continue;
            
            
            Fit BRAM_type_1_fit = best_mapping_on_BRAM_type_1(circuits[i].ram[moving_r_index],true);
            Fit LUTRAM_fit = best_mapping_on_LUTRAM(circuits[i].ram[moving_r_index],true);

            //choose where to move
            int move = 0;
            
            if(circuits[i].ram[BRAMS_type_2[j].r_index].mode == "TrueDualPort" || LUTRAM_fit.req_area == -1){
               move =  0; //can not be mapped on LUTRAM
            }
            else if(circuits[i].util_LUT < circuits[i].util_BRAM_type_2){
                move = 1; //move to LUTRAM
            }


            if(move == 1){ // move to LUTRAM
                if(LUTRAM_fit.s * LUTRAM_fit.p <= avail_logic && LUTRAM_fit.s * LUTRAM_fit.p != 0){
                    //increase the number of LUTRAM
                    circuits[i].num_of_LUTRAM += LUTRAM_fit.s * LUTRAM_fit.p;
                    //decrease the number of BRAM_type_2
                    circuits[i].num_of_BRAM_type_1 -= BRAMS_type_2[j].s * BRAMS_type_2[j].p;

                    //update needed since the resources changed
                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                    avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                    

                    if(avail_logic >= 0){
                        //moving process, updating the mapping structure
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = LUTRAM_fit.type;
                        lp_maps[lp_index].mode = LUTRAM_fit.mode;
                        lp_maps[lp_index].s = LUTRAM_fit.s;
                        lp_maps[lp_index].p = LUTRAM_fit.p;
                        lp_maps[lp_index].additional_logic = LUTRAM_fit.req_logic;
                        continue;
                    }
                    
                    else{
                        //if not possible, redo the changes
                        circuits[i].num_of_LUTRAM -= LUTRAM_fit.s * LUTRAM_fit.p;
                        circuits[i].num_of_BRAM_type_1 += BRAMS_type_2[j].s * BRAMS_type_2[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                        avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;

                    }  
                }
            }


            if(move == 0){//move to BRAM_type_1  
                if(BRAM_type_1_fit.s * BRAM_type_1_fit.p <= avail_BRAM_type_1 && BRAM_type_1_fit.s * BRAM_type_1_fit.p != 0){
                    //increase the number of BRAM_type_1
                    circuits[i].num_of_BRAM_type_1 += BRAM_type_1_fit.s * BRAM_type_1_fit.p;
                    //decrease the number of BRAM_type_2
                    circuits[i].num_of_BRAM_type_2 -= BRAMS_type_2[j].s * BRAMS_type_2[j].p;

                    //update since number of resources changed
                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                    avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                    

                    if(avail_BRAM_type_1 >= 0){
                        //moving process, changing the mapping structure
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = BRAM_type_1_fit.type;
                        lp_maps[lp_index].mode = BRAM_type_1_fit.mode;
                        lp_maps[lp_index].s = BRAM_type_1_fit.s;
                        lp_maps[lp_index].p = BRAM_type_1_fit.p;
                        lp_maps[lp_index].additional_logic = BRAM_type_1_fit.req_logic;
                        continue;
                    }
                    else{
                        //if not possible, redo changes
                        circuits[i].num_of_BRAM_type_1 -= BRAM_type_1_fit.s * BRAM_type_1_fit.p;
                        circuits[i].num_of_BRAM_type_2 += BRAMS_type_2[j].s * BRAMS_type_2[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                        avail_BRAM_type_1   = ceil((double)LUT_in_circuit[i]/BRAM_type_1_RATIO) - circuits[i].num_of_BRAM_type_1;
                    }
                }
            }
        }
    }
}

//move from BRAM_type_1 to LUTRAM/BRAM_type_2
void balanceBRAM_type_1(){
    //loop over all circuits
    for(int i = 0; i < num_of_circuits; i++){
        //if already balanced, no changes
        if(balance[i]){
            continue;
        }
        
        //no need to move from _type_2 to other RAMs
        if(circuits[i].util_BRAM_type_2 >= circuits[i].util_BRAM_type_1 || circuits[i].util_BRAM_type_1 <= circuits[i].util_LUT)
            continue;

        balance[i] = true;

        //get the logical RAMs mapped to BRAM_type_1 and sort them
        vector<LP_map> BRAMS_type_1 = get_BRAMS(i,1);
        sort(BRAMS_type_1.begin(),BRAMS_type_1.end(),compare_two_Logical_RAM_ascending);
        
        //calculate available resources
        int avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
        int avail_BRAM_type_2   = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
        
        for(int j = 0; j < BRAMS_type_1.size(); j++){    
            //no need to move anymore
            if((circuits[i].util_BRAM_type_2 >= circuits[i].util_BRAM_type_1 && circuits[i].util_LUT >= circuits[i].util_BRAM_type_1))
                break;
            
        
            int moving_r_index = BRAMS_type_1[j].r_index;

            if(circuits[i].ram[moving_r_index].packed)
                continue;

            Fit LUTRAM_fit = best_mapping_on_LUTRAM(circuits[i].ram[moving_r_index],true);
            Fit BRAM_type_2_fit = best_mapping_on_BRAM_type_2(circuits[i].ram[moving_r_index],true);

            int move = 0;//decide where to move
            if(circuits[i].ram[BRAMS_type_1[j].r_index].mode == "TrueDualPort" || LUTRAM_fit.req_area == -1){
               move =  0;//can not be mapped to LUTRAM
            }
            else if(circuits[i].util_LUT < circuits[i].util_BRAM_type_1){
                move = 1;//move to LUTRAM
            }
            

            if(move == 1){//move to LUTRAM
                if(LUTRAM_fit.s * LUTRAM_fit.p <= avail_logic && LUTRAM_fit.s * LUTRAM_fit.p != 0){
                    //increase the number of LUTRAMs
                    circuits[i].num_of_LUTRAM += LUTRAM_fit.s * LUTRAM_fit.p;
                    //decrease the number of BRAM_type_1
                    circuits[i].num_of_BRAM_type_1 -= BRAMS_type_1[j].s * BRAMS_type_1[j].p;

                    //update since the resources changed
                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                    avail_BRAM_type_2   = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                    
                    if(avail_logic >= 0){
                        //if moving is possible, change the mapping structure
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = LUTRAM_fit.type;
                        lp_maps[lp_index].mode = LUTRAM_fit.mode;
                        lp_maps[lp_index].s = LUTRAM_fit.s;
                        lp_maps[lp_index].p = LUTRAM_fit.p;
                        lp_maps[lp_index].additional_logic = LUTRAM_fit.req_logic;
                        continue;
                    }
                    else{
                        //if moving is not possible, redo the changes
                        circuits[i].num_of_LUTRAM -= LUTRAM_fit.s * LUTRAM_fit.p;
                        circuits[i].num_of_BRAM_type_1 += BRAMS_type_1[j].s * BRAMS_type_1[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                        avail_BRAM_type_2   = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                    }  
                }
            }
            if(move == 0){//move to BRAM_type_2
                if(BRAM_type_2_fit.s * BRAM_type_2_fit.p <= avail_BRAM_type_2 && BRAM_type_2_fit.s * BRAM_type_2_fit.p != 0){
                    //increase the number of BRAM_type_2
                    circuits[i].num_of_BRAM_type_2 += BRAM_type_2_fit.s * BRAM_type_2_fit.p;
                    //decrease the number of BRAM_type_1
                    circuits[i].num_of_BRAM_type_1 -= BRAMS_type_1[j].s * BRAMS_type_1[j].p;

                    //update since the resources changed
                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                    avail_BRAM_type_2   = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                    
                    if(avail_BRAM_type_2 >= 0){
                        //if moving is possible, change the mapping structure
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = BRAM_type_2_fit.type;
                        lp_maps[lp_index].mode = BRAM_type_2_fit.mode;
                        lp_maps[lp_index].s = BRAM_type_2_fit.s;
                        lp_maps[lp_index].p = BRAM_type_2_fit.p;
                        lp_maps[lp_index].additional_logic = BRAM_type_2_fit.req_logic;
                        continue;
                    }
                    else{
                        //if moving is not possible, redo the changes
                        circuits[i].num_of_BRAM_type_2 -= BRAM_type_2_fit.s * BRAM_type_2_fit.p;
                        circuits[i].num_of_BRAM_type_1 += BRAMS_type_1[j].s * BRAMS_type_1[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / LUTRAM_RATIO) - circuits[i].num_of_LUTRAM;
                        avail_BRAM_type_2   = ceil((double)LUT_in_circuit[i]/BRAM_type_2_RATIO) - circuits[i].num_of_BRAM_type_2;
                    }  
                }
            }
        }
    }
}


int main(int argc, char** argv){
    
    BRAM_type_1_BITS = atoi(argv[1]);
    BRAM_type_1_MAX_WIDTH = atoi(argv[2]);
    BRAM_type_1_RATIO = atoi(argv[3]);
    
    BRAM_type_2_BITS = atoi(argv[4]);
    BRAM_type_2_MAX_WIDTH= atoi(argv[5]);
    BRAM_type_2_RATIO = atoi(argv[6]);

    LUTRAM_RATIO = atoi(argv[7]) + 1;

    //calculate area based on bits
    BRAM_type_1_AREA = 9000 + 5 * BRAM_type_1_BITS  + 90 * sqrt(BRAM_type_1_BITS) + 600 * 2 * BRAM_type_1_MAX_WIDTH; 
    BRAM_type_1_AREA /= 10000;

    BRAM_type_2_AREA = 9000 + 5 * BRAM_type_2_BITS  + 90 * sqrt(BRAM_type_2_BITS) + 600 * 2 * BRAM_type_2_MAX_WIDTH; 
    BRAM_type_2_AREA /= 10000;

    //parse the input files
    parse_input();
    
    //find the initial mapping
    for(int i = 0 ; i < num_of_circuits; i++){
        counter = 0;
        for(int j = 0; j < circuits[i].ram.size(); j++){
            find_best_mapping(circuits[i].ram[j],i,false);
        }
    }
    
    get_memory_utilization();
    
    //balancing the mapping to reduce area
    for(int i = 0; i < 5; i++){
        for(int j = 0; j < num_of_circuits; j++)
            balance[j] = false;

        balance_LUTRAM_to_BRAM();
        balance_BRAM_type_2();
        balanceBRAM_type_1();
    }
    
    get_memory_utilization();
    
    //find some packing to do between two single ports or two ROM
    cal_leftovers();
    pack_leftovers();

    //one more balancing step
    balance_LUTRAM_to_BRAM();
    balance_BRAM_type_2();
    balanceBRAM_type_1();

    //write the output for legality checker
    for(int i = 0; i < num_of_circuits; i++){
        for(int j = 0; j < circuits[i].ram.size(); j++){
            write_checker_file(circuits[i].ram[j],i);
        }
    }
    
    return 0;
}