#include <iostream>
#include <fstream>
#include <sstream>
#include <string>  
#include <bits/stdc++.h>
#include <algorithm>
#include <random>

#define LUTRAM_MODE_0_WORDS 64 
#define LUTRAM_MODE_1_WORDS 32
#define LUTRAM_MODE_0_WIDTH 10
#define LUTRAM_MODE_1_WIDTH 20

#define BRAM_8K_BITS 8192
#define BRAM_8K_MAX_WIDTH 32
#define BRAM_128K_BITS 131072
#define BRAM_128K_MAX_WIDTH 128

#define LUTRAM_AREA 4
#define LUT_AREA 3.5
#define BRAM_8K_AREA 9.6505
#define BRAM_128K_AREA 73.5343

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
    int num_of_8KBRAM;
    int num_of_128KBRAM;
    int num_of_LUTRAM;
    //logic blocks in file
    int num_of_logic_elements;
    //utilization rate
    double util_LUT;
    double util_8KBRAM;
    double util_128KBRAM;

};

struct Fit{
    int type;
    int req_logic;
    double req_area;
    int s;
    int p; 
    int mode;
};

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

//init the circuits based on input file
void init(){
    circuits = new Circuit[num_of_circuits];
    balance = new bool[num_of_circuits];
    LUT_in_circuit = new int[num_of_circuits];
    for(int i = 0; i < num_of_circuits; i++){
        circuits[i].num_of_LUTRAM = 0;
        circuits[i].num_of_8KBRAM = 0;
        circuits[i].num_of_128KBRAM = 0;
        circuits[i].num_of_logic_elements = 0;        
        circuits[i].util_LUT = 0;
        circuits[i].util_8KBRAM = 0;
        circuits[i].util_128KBRAM = 0;
        balance[i] = false;

    }
    
}



//debug function
void print_mapping(RAM r, int c_index){
    int lp_index = r.lp_map_id;
    cout << "============================================================" << endl;
    cout << r.mode << endl;
    cout << r.width << " " << r.depth << endl;
    cout << "Mapped to .." << endl;
    cout << lp_maps[lp_index].type+1 <<" " << lp_maps[lp_index].mode << endl;
    cout << lp_maps[lp_index].s <<" " << lp_maps[lp_index].p << endl;

}


//print utilization
void get_memory_utilization(){  
    for(int i = 0 ;  i < num_of_circuits; i++){
        
        int lut_by_8KBRAM = circuits[i].num_of_8KBRAM * 10;
        int lut_by_128KBRAM = circuits[i].num_of_128KBRAM * 300;
        int lut_block = max(circuits[i].num_of_logic + circuits[i].num_of_LUTRAM, 2 * circuits[i].num_of_LUTRAM);

        if(lut_block >= lut_by_128KBRAM && lut_block >= lut_by_8KBRAM){
            LUT_in_circuit[i] = lut_block;
        }
        else if(lut_by_8KBRAM >= lut_by_128KBRAM){
            LUT_in_circuit[i] = lut_by_8KBRAM;
        }
        else{
            LUT_in_circuit[i] = lut_by_128KBRAM;
        }

        //print utilization for circuit i
        

        circuits[i].util_LUT = (double) (circuits[i].num_of_LUTRAM + circuits[i].num_of_logic)/LUT_in_circuit[i]; 
        circuits[i].util_8KBRAM = (double) (circuits[i].num_of_8KBRAM/ (ceil((double) LUT_in_circuit[i] / 10)));         
        circuits[i].util_128KBRAM = (double) (circuits[i].num_of_128KBRAM/ (ceil((double) LUT_in_circuit[i] / 300)));         

        if(circuits[i].util_LUT < 0.9 || circuits[i].util_8KBRAM < 0.9 || circuits[i].util_128KBRAM < 0.9){
            cout << "CIRCUIT NO : " << i << endl;
            cout << "TOTAL NUMBER OF LUT BLOCKS " << LUT_in_circuit[i] << " ,USED LUT : " << circuits[i].num_of_LUTRAM + circuits[i].num_of_logic << endl;
            cout << "TOTAL NUMBER OF 8KBRAM " << ceil((double)LUT_in_circuit[i]/10) << " ,USED 8KBRAM : " << circuits[i].num_of_8KBRAM << endl;
            cout << "TOTAL NUMBER OF 128KBRAM " << ceil((double)LUT_in_circuit[i]/300) << " ,USED 128KBRAM : " << circuits[i].num_of_128KBRAM << endl;
            cout << "LUT UTILIZATION " << circuits[i].util_LUT << endl;
            cout << "8KBRAM UTILIZATION " << circuits[i].util_8KBRAM << endl;    
            cout << "128KBRAM UTILIZATION " << circuits[i].util_128KBRAM << endl;
        }
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
        d = BRAM_8K_BITS / lp_maps[lp_index].mode;;
    }

    if(lp_maps[lp_index].type == 2){
        w = lp_maps[lp_index].mode;;
        d =  BRAM_128K_BITS / lp_maps[lp_index].mode;;
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
    if((r.mode == "SimpleDualPort" && !re_map) || (r.mode != "TrueDualPort" && re_map)){
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


Fit best_mapping_on_8KBRAM(RAM r, bool re_map){
    //MAP TO 8K BRAM
    int required_BRAM8K = INT_MAX;
    int required_BRAM8K_logic = 0;
    double BRAM8K_required_area = DBL_MAX;
    int best_8KBRAM_mode = -1;
    int best_8KBRAM_p; 
    int best_8KBRAM_s;
    for(int x = 1 ; x <= BRAM_8K_MAX_WIDTH; x *= 2){
        //x32 is not available on TRUEDUALPORT mode!
        if(((r.mode != "SimpleDualPort" && !re_map) || (r.mode == "TrueDualPort" && re_map)) && x == BRAM_8K_MAX_WIDTH){
            continue;
        }
        int req_ram = ceil((double)r.width/x) * ceil((double)r.depth/(BRAM_8K_BITS/x));

        int req_logic = 0;
        if(r.depth > (BRAM_8K_BITS/x)){//extra logic needed
            if((double)r.depth/(BRAM_8K_BITS/x) > 16){
                //can not be implemented
                req_ram = -1;
                req_logic = -1;
            }
            else{
                int num_of_mux = (ceil((double)r.depth/(BRAM_8K_BITS/x))/4) + 1;
                if((int) ceil((double)r.depth/(BRAM_8K_BITS/x)) % 4 == 1)
                    num_of_mux--;
                
                req_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/(BRAM_8K_BITS/x));
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
            if(req_ram * BRAM_8K_AREA + req_logic * LUT_AREA < BRAM8K_required_area){
                BRAM8K_required_area = req_ram * BRAM_8K_AREA + req_logic * LUT_AREA;
                required_BRAM8K = req_ram;
                required_BRAM8K_logic = req_logic;
                best_8KBRAM_mode = x;
                best_8KBRAM_s = ceil((double)r.depth/(BRAM_8K_BITS/x));
                best_8KBRAM_p = ceil((double)r.width/x);
            }
        }
    }


    if(r.mode == "TrueDualPort")
        required_BRAM8K_logic *= 2;

    Fit f_temp;
    f_temp.type = 1;
    f_temp.mode = best_8KBRAM_mode;
    f_temp.s = best_8KBRAM_s;
    f_temp.p = best_8KBRAM_p;
    f_temp.req_logic = required_BRAM8K_logic;
    f_temp.req_area = BRAM8K_required_area;
    return f_temp;
}

Fit best_mapping_on_128KBRAM(RAM r, bool re_map){
    //MAP TO 128 BRAM
    int required_BRAM128K = INT_MAX;
    int required_BRAM128K_logic = 0;
    double BRAM128K_required_area = DBL_MAX;
    int best_128KBRAM_mode = -1;
    int best_128KBRAM_p;
    int best_128KBRAM_s;
    for(int x = 1 ; x <= BRAM_128K_MAX_WIDTH; x *= 2){
        //x128 is not available on TRUEDUALPORT mode!
        if(((r.mode != "SimpleDualPort" && !re_map) || (r.mode == "TrueDualPort" && re_map)) && x == BRAM_128K_MAX_WIDTH){
            continue;
        }
        int req_ram = ceil((double)r.width/x) * ceil((double)r.depth/(BRAM_128K_BITS/x));
        int req_logic = 0;
        if(r.depth > (BRAM_128K_BITS/x)){//extra logic needed
            if((double)r.depth/(BRAM_128K_BITS/x) > 16){
                //can not be implemented
                req_ram = -1;
                req_logic = -1;
            }
            else{
                int num_of_mux = (ceil((double)r.depth/(BRAM_128K_BITS/x))/4) + 1;
                if((int) ceil((double)r.depth/(BRAM_128K_BITS/x)) % 4 == 1)
                    num_of_mux--;
                
                req_logic += r.width * num_of_mux;
                //decoder logic
                int extra_rams = ceil((double)r.depth/(BRAM_128K_BITS/x));
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
            if(req_ram * BRAM_128K_AREA + req_logic * LUT_AREA < BRAM128K_required_area){
                BRAM128K_required_area = req_ram * BRAM_128K_AREA + req_logic * LUT_AREA;
                required_BRAM128K = req_ram;
                required_BRAM128K_logic = req_logic;
                best_128KBRAM_mode = x;
                best_128KBRAM_s = ceil((double)r.depth/(BRAM_128K_BITS/x));
                best_128KBRAM_p = ceil((double)r.width/x);
            }
        }
    }

    Fit f_temp;
    if(r.mode == "TrueDualPort")
        required_BRAM128K_logic *= 2;

    f_temp.type = 2;
    f_temp.mode = best_128KBRAM_mode;
    f_temp.s = best_128KBRAM_s;
    f_temp.p = best_128KBRAM_p;
    f_temp.req_logic = required_BRAM128K_logic;
    f_temp.req_area = BRAM128K_required_area;
    return f_temp;

}


//map available logical RAMs to physical in Circuit c
void find_best_mapping(RAM r, int circuit_id, bool re_map){
    
    Fit LUTRAM_fit = best_mapping_on_LUTRAM(r,re_map);
    Fit BRAM8K_fit = best_mapping_on_8KBRAM(r,re_map);
    Fit BRAM128K_fit = best_mapping_on_128KBRAM(r,re_map);

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

    if(LUTRAM_fit.req_area < BRAM8K_fit.req_area){
        if(LUTRAM_fit.req_area <= BRAM128K_fit.req_area){
            best_area = LUTRAM_fit.req_area;
            best_mode = LUTRAM_fit.mode;
            best_type = 0;
            LUT = LUTRAM_fit.req_logic;
            best_p = LUTRAM_fit.p;
            best_s = LUTRAM_fit.s;
            
        }
        else if(BRAM128K_fit.req_area < LUTRAM_fit.req_area){
            best_area = BRAM128K_fit.req_area;
            best_mode = BRAM128K_fit.mode;
            best_type = 2;
            LUT = BRAM128K_fit.req_logic;
            best_p = BRAM128K_fit.p;
            best_s = BRAM128K_fit.s;
        }
    }

    if(LUTRAM_fit.req_area > BRAM8K_fit.req_area){
        if(BRAM8K_fit.req_area <= BRAM128K_fit.req_area){
            best_area = BRAM8K_fit.req_area;
            best_mode = BRAM8K_fit.mode;
            best_type = 1;
            LUT = BRAM8K_fit.req_logic;
            best_p = BRAM8K_fit.p;
            best_s = BRAM8K_fit.s;

        }
        else if(BRAM8K_fit.req_area > BRAM128K_fit.req_area){
            best_area = BRAM128K_fit.req_area;
            best_mode = BRAM128K_fit.mode;
            best_type = 2;
            LUT = BRAM128K_fit.req_logic;
            best_p = BRAM128K_fit.p;
            best_s = BRAM128K_fit.s;
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
            case 1: circuits[circuit_id].num_of_8KBRAM += best_s*best_p; break;
            case 2: circuits[circuit_id].num_of_128KBRAM += best_s*best_p; break;
            default : break;
        }
        lp_maps_id++;
    }
    else{
        
        int lp_index = circuits[circuit_id].ram[r.id].lp_map_id;
        // cout << "PREVIOUS MAPPING.. " << lp_maps[lp_index].type+1 << " " << lp_maps[lp_index].mode << endl;
        lp_maps[lp_index].type = best_type;
        lp_maps[lp_index].mode = best_mode;
        lp_maps[lp_index].s = best_s;
        lp_maps[lp_index].p = best_p;
        lp_maps[lp_index].additional_logic = LUT;

        // cout << "NEW MAPPING..." << endl;
        // cout << lp_maps[lp_index].type+1 << " " << lp_maps[lp_index].mode << endl;

        // cout << "choose between.." << endl;
        // cout << BRAM128K_required_area << " " << BRAM8K_required_area << " " << LUTRAM_required_area << endl;

    }
    
}


void cal_leftovers(){
    for(int i = 0; i < lp_maps.size(); i++){   
        int c_index = lp_maps[i].c_index;
        int r_index = lp_maps[i].r_index;

        if(circuits[c_index].ram[r_index].mode != "ROM" && circuits[c_index].ram[r_index].mode != "SinglePort")
            continue;
        
        int logical_ram_width = circuits[c_index].ram[r_index].width;
        int logical_ram_depth = circuits[c_index].ram[r_index].depth;

        int physical_ram_width;
        int physical_ram_depth;
        switch(lp_maps[i].type){          
            case 1:
                physical_ram_width = lp_maps[i].mode;
                physical_ram_depth = BRAM_8K_BITS/lp_maps[i].mode;
                break;
            case 2:
                physical_ram_width = lp_maps[i].mode;
                physical_ram_depth = BRAM_128K_BITS/lp_maps[i].mode;
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
            // if(c_index == 19 && lp_maps[i].r_index == 42){
            //     cout << "next power 2" <<   next_power_2_d << endl;
            // }
            if(l_w <= lp_maps[i].width_left_2 && next_power_2_d <= lp_maps[i].depth_left_2 && !circuits[lp_maps[i].c_index].ram[j].packed){
                // cout << " SECOND LEFTOVER CANDIDATES... " << endl;
                // cout << c_index << " " << j << endl;
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
        if(max_ram_id_2 == -1){
            continue;
        }
        else{
            
            //update the resource usage
            int locate_ram_in_lp = circuits[lp_maps[i].c_index].ram[max_ram_id_2].lp_map_id;
            circuits[lp_maps[i].c_index].num_of_logic_elements -= lp_maps[locate_ram_in_lp].additional_logic;
            switch(lp_maps[locate_ram_in_lp].type){
                case 0: circuits[lp_maps[i].c_index].num_of_LUTRAM -= lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                case 1: circuits[lp_maps[i].c_index].num_of_8KBRAM -=lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                case 2: circuits[lp_maps[i].c_index].num_of_128KBRAM -= lp_maps[locate_ram_in_lp].p* lp_maps[locate_ram_in_lp].s;break;
                default: break;
            }

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

bool compare_two_Logical_RAM(const LP_map &lp1, const LP_map &lp2){
    int l1_width = circuits[lp1.c_index].ram[lp1.r_index].width;
    int l1_depth = circuits[lp1.c_index].ram[lp1.r_index].depth;

    int l2_width = circuits[lp2.c_index].ram[lp2.r_index].width;
    int l2_depth = circuits[lp2.c_index].ram[lp2.r_index].depth;

    return l1_width * l1_depth > l2_width * l2_depth;
}


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


void update_LUT_number(int c_index){
    int lut_by_8KBRAM = circuits[c_index].num_of_8KBRAM * 10;
    int lut_by_128KBRAM = circuits[c_index].num_of_128KBRAM * 300;
    int lut_block = max(circuits[c_index].num_of_logic + circuits[c_index].num_of_LUTRAM, 2 * circuits[c_index].num_of_LUTRAM);

    if(lut_block >= lut_by_128KBRAM && lut_block >= lut_by_8KBRAM){
        LUT_in_circuit[c_index] = lut_block;
    }
    else if(lut_by_8KBRAM >= lut_by_128KBRAM){
        LUT_in_circuit[c_index] = lut_by_8KBRAM;
    }
    else{
        LUT_in_circuit[c_index] = lut_by_128KBRAM;
    }

    circuits[c_index].util_LUT = (double) (circuits[c_index].num_of_LUTRAM + circuits[c_index].num_of_logic)/LUT_in_circuit[c_index]; 
    circuits[c_index].util_8KBRAM = (double) (circuits[c_index].num_of_8KBRAM/ (ceil((double) LUT_in_circuit[c_index] / 10)));         
    circuits[c_index].util_128KBRAM = (double) (circuits[c_index].num_of_128KBRAM/ (ceil((double) LUT_in_circuit[c_index] / 300)));   
}


void balance_LUTRAM_to_BRAM(){
    for(int i = 0; i < num_of_circuits; i++){
        
        if(circuits[i].util_LUT <= circuits[i].util_8KBRAM || circuits[i].util_LUT <= circuits[i].util_128KBRAM)
            continue;

        balance[i] = true;

        vector<LP_map> LUTRAMS = get_LUTRAMS(i);
        sort(LUTRAMS.begin(),LUTRAMS.end(),compare_two_Logical_RAM);
        
        update_LUT_number(i);
        int avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
        int avail_128KBRAM = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
    
        for(int j = 0; j < LUTRAMS.size(); j++){
            if(avail_8KBRAM == 0 && avail_128KBRAM == 0)
                break;

            
            
            int moving_r_index = LUTRAMS[j].r_index;
            Fit BRAM128K_fit = best_mapping_on_128KBRAM(circuits[i].ram[moving_r_index],true);
            
            if(BRAM128K_fit.s * BRAM128K_fit.p <= avail_128KBRAM){
                //increase the number of BRAM128K
                circuits[i].num_of_128KBRAM += BRAM128K_fit.s * BRAM128K_fit.p;
                circuits[i].num_of_LUTRAM -= LUTRAMS[j].s * LUTRAMS[j].p;
                
                update_LUT_number(i);
                avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                avail_128KBRAM = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;


                if(avail_128KBRAM >= 0 && circuits[i].util_128KBRAM < circuits[i].util_LUT){
                    int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                    lp_maps[lp_index].type = BRAM128K_fit.type;
                    lp_maps[lp_index].mode = BRAM128K_fit.mode;
                    lp_maps[lp_index].s = BRAM128K_fit.s;
                    lp_maps[lp_index].p = BRAM128K_fit.p;
                    lp_maps[lp_index].additional_logic = BRAM128K_fit.req_logic;
                    continue;
                }
                else{
                    circuits[i].num_of_128KBRAM -= BRAM128K_fit.s * BRAM128K_fit.p;
                    circuits[i].num_of_LUTRAM += LUTRAMS[j].s * LUTRAMS[j].p;
                    update_LUT_number(i);
                    avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                    avail_128KBRAM = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
                }

            }
            
            Fit BRAM8K_fit = best_mapping_on_8KBRAM(circuits[i].ram[moving_r_index],true);
            if(BRAM8K_fit.s * BRAM8K_fit.p <= avail_8KBRAM){
                
                circuits[i].num_of_8KBRAM += BRAM8K_fit.s * BRAM8K_fit.p;
                circuits[i].num_of_LUTRAM -= LUTRAMS[j].s * LUTRAMS[j].p;
                
                update_LUT_number(i);
                avail_8KBRAM   = ceil((double)(LUT_in_circuit[i]/10)) - circuits[i].num_of_8KBRAM;
                avail_128KBRAM = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;

                

                if(avail_8KBRAM >= 0 && circuits[i].util_8KBRAM < circuits[i].util_LUT){
                    int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                    lp_maps[lp_index].type = BRAM8K_fit.type;
                    lp_maps[lp_index].mode = BRAM8K_fit.mode;
                    lp_maps[lp_index].s = BRAM8K_fit.s;
                    lp_maps[lp_index].p = BRAM8K_fit.p;
                    lp_maps[lp_index].additional_logic = BRAM8K_fit.req_logic;
                    continue;
                }
                else{
                    circuits[i].num_of_8KBRAM -= BRAM8K_fit.s * BRAM8K_fit.p;
                    circuits[i].num_of_LUTRAM += LUTRAMS[j].s * LUTRAMS[j].p;
                    update_LUT_number(i);
                    avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                    avail_128KBRAM = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
                
                }

                
            }
        }
        
    }
}

bool compare_two_Logical_RAM_ascending(const LP_map &lp1, const LP_map &lp2){
    int l1_width = circuits[lp1.c_index].ram[lp1.r_index].width;
    int l1_depth = circuits[lp1.c_index].ram[lp1.r_index].depth;

    int l2_width = circuits[lp2.c_index].ram[lp2.r_index].width;
    int l2_depth = circuits[lp2.c_index].ram[lp2.r_index].depth;

    return l1_width * l1_depth < l2_width * l2_depth;
}


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


void balance_128KBRAM(){
    for(int i = 0; i < num_of_circuits; i++){
        
        if(circuits[i].util_128KBRAM <= circuits[i].util_8KBRAM)
            continue;

        if(balance[i])
            continue;

        balance[i] = true;


        vector<LP_map> BRAMS128K = get_BRAMS(i,2);
        sort(BRAMS128K.begin(),BRAMS128K.end(),compare_two_Logical_RAM_ascending);
        
        update_LUT_number(i);
        int avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
        int avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
        
        for(int j = 0; j < BRAMS128K.size(); j++){
            
            update_LUT_number(i);
            if((circuits[i].util_8KBRAM >= circuits[i].util_128KBRAM && circuits[i].util_LUT >= circuits[i].util_128KBRAM))
                break;
            
            //move from BRAM128K TO LUTRAM
            int moving_r_index = BRAMS128K[j].r_index;

            if(circuits[i].ram[moving_r_index].packed)
                continue;
            
            Fit BRAM8K_fit = best_mapping_on_8KBRAM(circuits[i].ram[moving_r_index],true);
            Fit LUTRAM_fit = best_mapping_on_LUTRAM(circuits[i].ram[moving_r_index],true);

            int move = 0;
            
            if(circuits[i].ram[BRAMS128K[j].r_index].mode == "TrueDualPort" || LUTRAM_fit.req_area == -1){
               move =  0;
            }
            else if(circuits[i].util_LUT < circuits[i].util_128KBRAM){
                move = 1;
            }


            if(move == 1){
                if(LUTRAM_fit.s * LUTRAM_fit.p <= avail_logic && LUTRAM_fit.s * LUTRAM_fit.p != 0){
                
                    circuits[i].num_of_LUTRAM += LUTRAM_fit.s * LUTRAM_fit.p;
                    circuits[i].num_of_8KBRAM -= BRAMS128K[j].s * BRAMS128K[j].p;

                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                    avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                    

                    if(avail_logic >= 0){
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = LUTRAM_fit.type;
                        lp_maps[lp_index].mode = LUTRAM_fit.mode;
                        lp_maps[lp_index].s = LUTRAM_fit.s;
                        lp_maps[lp_index].p = LUTRAM_fit.p;
                        lp_maps[lp_index].additional_logic = LUTRAM_fit.req_logic;
                        continue;
                    }
                    
                    else{
                        circuits[i].num_of_LUTRAM -= LUTRAM_fit.s * LUTRAM_fit.p;
                        circuits[i].num_of_8KBRAM += BRAMS128K[j].s * BRAMS128K[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                        avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;

                    }  
                }
            }


            if(move == 0){      
                if(BRAM8K_fit.s * BRAM8K_fit.p <= avail_8KBRAM && BRAM8K_fit.s * BRAM8K_fit.p != 0){
                    
                    //increase the number of BRAM128K
                    circuits[i].num_of_8KBRAM += BRAM8K_fit.s * BRAM8K_fit.p;
                    circuits[i].num_of_128KBRAM -= BRAMS128K[j].s * BRAMS128K[j].p;

                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                    avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                    

                    if(avail_8KBRAM >= 0){
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = BRAM8K_fit.type;
                        lp_maps[lp_index].mode = BRAM8K_fit.mode;
                        lp_maps[lp_index].s = BRAM8K_fit.s;
                        lp_maps[lp_index].p = BRAM8K_fit.p;
                        lp_maps[lp_index].additional_logic = BRAM8K_fit.req_logic;
                        continue;
                    }
                    else{
                        circuits[i].num_of_8KBRAM -= BRAM8K_fit.s * BRAM8K_fit.p;
                        circuits[i].num_of_128KBRAM += BRAMS128K[j].s * BRAMS128K[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                        avail_8KBRAM   = ceil((double)LUT_in_circuit[i]/10) - circuits[i].num_of_8KBRAM;
                    }
                }
            }
        }
    }
}

void balance_8KBRAM(){
    for(int i = 0; i < num_of_circuits; i++){
        
        if(balance[i]){
            continue;
        }

        balance[i] = true;


        vector<LP_map> BRAMS8K = get_BRAMS(i,1);
        sort(BRAMS8K.begin(),BRAMS8K.end(),compare_two_Logical_RAM_ascending);
        
        update_LUT_number(i);
        int avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
        int avail_128KBRAM   = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
        
        
        for(int j = 0; j < BRAMS8K.size(); j++){
            
            update_LUT_number(i);
            if((circuits[i].util_128KBRAM >= circuits[i].util_8KBRAM && circuits[i].util_LUT >= circuits[i].util_8KBRAM))
                break;
            
            //move from BRAM128K TO LUTRAM
            int moving_r_index = BRAMS8K[j].r_index;

            if(circuits[i].ram[moving_r_index].packed)
                continue;

            Fit LUTRAM_fit = best_mapping_on_LUTRAM(circuits[i].ram[moving_r_index],true);
            Fit BRAM128K_fit = best_mapping_on_128KBRAM(circuits[i].ram[moving_r_index],true);

            int move = 0;
            if(circuits[i].ram[BRAMS8K[j].r_index].mode == "TrueDualPort" || LUTRAM_fit.req_area == -1){
               move =  0;
            }
            else if(circuits[i].util_LUT < circuits[i].util_8KBRAM){
                move = 1;
            }
            

            if(move == 1){
                if(LUTRAM_fit.s * LUTRAM_fit.p <= avail_logic && LUTRAM_fit.s * LUTRAM_fit.p != 0){
                
                    circuits[i].num_of_LUTRAM += LUTRAM_fit.s * LUTRAM_fit.p;
                    circuits[i].num_of_8KBRAM -= BRAMS8K[j].s * BRAMS8K[j].p;

                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                    avail_128KBRAM   = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
                    

                    if(avail_logic >= 0){
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = LUTRAM_fit.type;
                        lp_maps[lp_index].mode = LUTRAM_fit.mode;
                        lp_maps[lp_index].s = LUTRAM_fit.s;
                        lp_maps[lp_index].p = LUTRAM_fit.p;
                        lp_maps[lp_index].additional_logic = LUTRAM_fit.req_logic;
                        continue;
                    }
                    else{
                        circuits[i].num_of_LUTRAM -= LUTRAM_fit.s * LUTRAM_fit.p;
                        circuits[i].num_of_8KBRAM += BRAMS8K[j].s * BRAMS8K[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                        avail_128KBRAM   = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;

                    }  
                }
            }
            
            if(move == 0){
                if(BRAM128K_fit.s * BRAM128K_fit.p <= avail_128KBRAM && BRAM128K_fit.s * BRAM128K_fit.p != 0){
                
                    //increase the number of BRAM128K
                    circuits[i].num_of_128KBRAM += BRAM128K_fit.s * BRAM128K_fit.p;
                    circuits[i].num_of_8KBRAM -= BRAMS8K[j].s * BRAMS8K[j].p;

                    update_LUT_number(i);
                    avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                    avail_128KBRAM   = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
                    

                    if(avail_128KBRAM >= 0){
                        int lp_index = circuits[i].ram[moving_r_index].lp_map_id;
                        lp_maps[lp_index].type = BRAM128K_fit.type;
                        lp_maps[lp_index].mode = BRAM128K_fit.mode;
                        lp_maps[lp_index].s = BRAM128K_fit.s;
                        lp_maps[lp_index].p = BRAM128K_fit.p;
                        lp_maps[lp_index].additional_logic = BRAM128K_fit.req_logic;
                        continue;
                    }
                    else{
                        circuits[i].num_of_128KBRAM -= BRAM128K_fit.s * BRAM128K_fit.p;
                        circuits[i].num_of_8KBRAM += BRAMS8K[j].s * BRAMS8K[j].p;
                        update_LUT_number(i);
                        avail_logic = ceil(LUT_in_circuit[i] / 2) - circuits[i].num_of_LUTRAM;
                        avail_128KBRAM   = ceil((double)LUT_in_circuit[i]/300) - circuits[i].num_of_128KBRAM;
                    }  
                }
            }
        }
    }
}


int main(){
    parse_input();
    
    for(int i = 0 ; i < num_of_circuits; i++){
        counter = 0;
        for(int j = 0; j < circuits[i].ram.size(); j++){
            find_best_mapping(circuits[i].ram[j],i,false);
        }
    }
    
    cal_leftovers();
    pack_leftovers();

    get_memory_utilization();
    for(int i = 0; i < 100; i++){
        for(int j = 0; j < num_of_circuits; j++)
            balance[j] = false;

        balance_LUTRAM_to_BRAM();
        balance_128KBRAM();
        balance_8KBRAM();
    }
    
    

    cout << "===================================================BALANCE===========================================" << endl;
    get_memory_utilization();
    
    for(int i = 0; i < num_of_circuits; i++){
        for(int j = 0; j < circuits[i].ram.size(); j++){
            write_checker_file(circuits[i].ram[j],i);
        }
    }
    
    return 0;
}