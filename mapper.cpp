#include <iostream>
#include <fstream>
#include <sstream>
#include <bits/stdc++.h>

#define LUTRAM_MODE_0_WORDS 64 
#define LUTRAM_MODE_1_WORDS 32
#define LUTRAM_MODE_0_WIDTH 10
#define LUTRAM_MODE_1_WIDTH 20

#define BRAM_8K_BITS 8192
#define BRAM_128_BITS 131072

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


//map available logical RAMs to physical in Circuit c
void find_best_mapping(RAM r){
    cout << "******************************************************************" << endl;
    cout << "MAPPING RAM " << r.mode << " " << r.depth << " " << r.width << endl;
    //MAP TO LUTRAM 
    int required_LUTRAM = -1;
    int required_LUTRAM_logic = -1;
    double LUTRAM_required_area = -1;
    if(r.mode != "TrueDualPort"){
        cout << "========LUTRAM========" << endl;
        //LUTRAM MODE 0
        int lut_mode_0 = 0;
        int lut_mode_0_logic = 0;
        
        if((double)r.depth/LUTRAM_MODE_0_WORDS > 16){
            //can not be implemented 
            lut_mode_0 = -1; 
            lut_mode_0_logic = -1;
        }
        else{
            if(r.width < LUTRAM_MODE_0_WIDTH){
                lut_mode_0++;
            }
            else{
                lut_mode_0 += ceil((double)r.width/LUTRAM_MODE_0_WIDTH);
            }
            //extra logic is needed
            if(r.depth > LUTRAM_MODE_0_WORDS){
                lut_mode_0 *= ceil((double)r.depth/LUTRAM_MODE_0_WORDS);
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
            cout << "LUTRAM MODE 0 " << lut_mode_0 << " " << lut_mode_0_logic << " " << endl;
        }
        //LUTRAM MODE 1
        int lut_mode_1 = 0;
        int lut_mode_1_logic = 0;
        if((double)r.depth/LUTRAM_MODE_1_WORDS > 16){
            //can not be implemented 
            lut_mode_1 = -1; 
            lut_mode_1_logic = -1;
        }
        else{
            if(r.width < LUTRAM_MODE_1_WIDTH){
                lut_mode_1++;
            }
            else{
                lut_mode_1 += ceil((double)r.width/LUTRAM_MODE_1_WIDTH);
            }
            //extra logic needed
            if(r.depth > LUTRAM_MODE_1_WORDS){
                lut_mode_1 *= ceil((double)r.depth/LUTRAM_MODE_1_WORDS);
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
            cout << "LUTRAM MODE 1 " << lut_mode_1 << " " << lut_mode_1_logic << " " << endl;
        }

        if(lut_mode_0 < 0){
            required_LUTRAM = lut_mode_1;
            required_LUTRAM_logic = lut_mode_1_logic;
        }
        if(lut_mode_1 < 0){
            required_LUTRAM = lut_mode_0;
            required_LUTRAM_logic = lut_mode_0_logic;
        }
        if(lut_mode_1 >= 0 && lut_mode_0 >= 0){
            if((LUTRAM_AREA*lut_mode_0+ LUT_AREA*lut_mode_0_logic) > (LUTRAM_AREA*lut_mode_1+LUT_AREA*lut_mode_1_logic)){
                required_LUTRAM = lut_mode_1;
                required_LUTRAM_logic = lut_mode_1_logic;
            }
            else{
                required_LUTRAM = lut_mode_0;
                required_LUTRAM_logic = lut_mode_0_logic;
            }
        }
    }   
    //MIN area required by LUTRAM
    if(required_LUTRAM >= 0 && required_LUTRAM_logic >= 0){
        LUTRAM_required_area = LUTRAM_AREA * required_LUTRAM + LUT_AREA * required_LUTRAM_logic;
    }
    cout << "LUTRAM " << required_LUTRAM << " " << required_LUTRAM_logic << " " <<  LUTRAM_required_area << endl; 

    //MAP TO 8K BRAM
    int required_BRAM8K = INT_MAX;
    int required_BRAM8K_logic = 0;
    double BRAM8K_required_area = DBL_MAX;
    cout << "========8K BRAM========" << endl;
    for(int x = 1 ; x <= 32; x *= 2){
        //x32 is not available on TRUEDUALPORT mode!
        if(r.mode == "TrueDualPort" && x == 32){
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

        cout << "8k BRAM mode x" << x << " " << req_ram << " " << req_logic << endl;  

        if(req_logic >= 0 && req_ram >= 0){
            if(req_ram * BRAM_8K_AREA + req_logic * LUT_AREA < BRAM8K_required_area){
                BRAM8K_required_area = req_ram * BRAM_8K_AREA + req_logic * LUT_AREA;
                required_BRAM8K = req_ram;
                required_BRAM8K_logic = req_logic;
            }
        }
    }
    cout << "BRAM 8K " << required_BRAM8K << " " << required_BRAM8K_logic << " " << BRAM8K_required_area << endl; 
    //MAP TO 128 BRAM

}

int main(){
    parse_input();
    find_best_mapping(circuits[0].ram[0]);
    return 0;
}