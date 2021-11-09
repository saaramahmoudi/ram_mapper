#include <iostream>
#include <fstream>
#include <bits/stdc++.h>

using namespace std;

void parse_input(){
    ifstream testFile;
    testFile.open("logical_rams.txt");
    
    string input;
    while(!testFile.eof()){
        testFile >> input;
        cout << input << endl;
    }
}



int main(){
    parse_input();
    return 0;
}