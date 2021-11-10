#include <iostream>
#include <fstream>
#include <sstream>
#include<string>  
#include <bits/stdc++.h>

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
};

//store benchmark specification
struct Circuit{
    int num_of_logic;
    vector<RAM> ram;
};

//global variables
Circuit* circuits;
int num_of_circuits;
ofstream checker_input("checker_input.txt");
int counter = 0;

//init the circuits based on input file
void init(){
    circuits = new Circuit[num_of_circuits];
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


void write_checker_file(RAM r, int circuit_id, int best_type, int best_mode, int LUT, int s, int p){
    int w;
    int d;
    if(best_type == 0){
        if(best_mode == 0){
            w = 10;
            d = 64;
        }
        else{
            w = 20;
            d = 32;
        }
    }
    if(best_type == 1){
        w = best_mode;
        d = BRAM_8K_BITS / best_mode;
    }

    if(best_type == 2){
        w = best_mode;
        d =  BRAM_128K_BITS / best_mode;
    }

    string out = "";
    out += to_string(circuit_id) + " " + to_string(r.id) + " " + to_string(LUT) + " ";
    out += "LW " + to_string(r.width) + " LD " + to_string(r.depth) + " ";
    out += "ID " + to_string(counter) + " S " + to_string(s) + " P " + to_string(p); 
    out += " Type " + to_string(best_type+1) + " Mode " + r.mode;
    out += " W " + to_string(w) + " D " + to_string(d);
    
    checker_input << out;
    checker_input << endl;
    counter++;
}

//map available logical RAMs to physical in Circuit c
void find_best_mapping(RAM r, int circuit_id){
    //MAP TO LUTRAM 
    int required_LUTRAM = -1;
    int required_LUTRAM_logic = -1;
    double LUTRAM_required_area = -1;
    int best_LUTRAM_mode = -1;
    int best_LUTRAM_s;
    int best_LUTRAM_p;
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
                int num_of_mux = ceil((double)r.depth/LUTRAM_MODE_0_WORDS)/4 + 1;
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
                    extra_rams = pow(2,ceil(log(extra_rams)/log(2)));
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
                int num_of_mux = (ceil((double)r.depth/LUTRAM_MODE_1_WORDS)/4) + 1;
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

    //MAP TO 8K BRAM
    int required_BRAM8K = INT_MAX;
    int required_BRAM8K_logic = 0;
    double BRAM8K_required_area = DBL_MAX;
    int best_8KBRAM_mode = -1;
    int best_8KBRAM_p; 
    int best_8KBRAM_s;
    for(int x = 1 ; x <= BRAM_8K_MAX_WIDTH; x *= 2){
        //x32 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == BRAM_8K_MAX_WIDTH){
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
    
    //MAP TO 128 BRAM
    int required_BRAM128K = INT_MAX;
    int required_BRAM128K_logic = 0;
    double BRAM128K_required_area = DBL_MAX;
    int best_128KBRAM_mode = -1;
    int best_128KBRAM_p;
    int best_128KBRAM_s;
    for(int x = 1 ; x <= BRAM_128K_MAX_WIDTH; x *= 2){
        //x128 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == BRAM_128K_MAX_WIDTH){
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
    
    //choose the best solution
    if(LUTRAM_required_area < 0){
        LUTRAM_required_area = DBL_MAX;
    }
    int best_type;
    int best_mode;
    double best_area;
    int LUT;
    int best_p;
    int best_s;

    if(LUTRAM_required_area < BRAM8K_required_area){
        if(LUTRAM_required_area <= BRAM128K_required_area){
            best_area = LUTRAM_required_area;
            best_mode = best_LUTRAM_mode;
            best_type = 0;
            LUT = required_LUTRAM_logic;
            best_p = best_LUTRAM_p;
            best_s = best_LUTRAM_s;
        }
        else if(BRAM128K_required_area < LUTRAM_required_area){
            best_area = BRAM128K_required_area;
            best_mode = best_128KBRAM_mode;
            best_type = 2;
            LUT = required_BRAM128K_logic;
            best_p = best_128KBRAM_p;
            best_s = best_128KBRAM_s;
        }
    }

    if(LUTRAM_required_area > BRAM8K_required_area){
        if(BRAM8K_required_area <= BRAM128K_required_area){
            best_area = BRAM8K_required_area;
            best_mode = best_8KBRAM_mode;
            best_type = 1;
            LUT = required_BRAM8K_logic;
            best_p = best_8KBRAM_p;
            best_s = best_8KBRAM_s;

        }
        else if(BRAM8K_required_area > BRAM128K_required_area){
            best_area = BRAM128K_required_area;
            best_mode = best_128KBRAM_mode;
            best_type = 2;
            LUT = required_BRAM128K_logic;
            best_p = best_128KBRAM_p;
            best_s = best_128KBRAM_s;
        }
    }

    write_checker_file(r,circuit_id,best_type,best_mode,LUT,best_s,best_p);
    
}

int main(){
    parse_input();
    
    for(int i = 0 ; i < num_of_circuits; i++){
        counter = 0;
        for(int j = 0; j < circuits[i].ram.size(); j++){
            find_best_mapping(circuits[i].ram[j],i);
        }
    }
    // cout << circuits[0].ram[109].width << " " << circuits[0].ram[109].depth << endl;
    // find_best_mapping(circuits[14].ram[0],14);
    return 0;
}