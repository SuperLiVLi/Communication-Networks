//
// Created by li Vlon on 12/1/23.
//
#include<stdio.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include <vector>
#include <iomanip>

using namespace std;

int node_num;
int length;
int M;
int total_time;
struct node{
    int node_id;
    int R;
    int backoff;
    int collision_count;
    node(int node_id, int R,int backoff, int collision_count):node_id(node_id),R(R),backoff(backoff),collision_count(collision_count){}
};
vector<node> nodes;
vector<int> bound;
ofstream fpOut;

void init_vec(){
    int count=node_num;
    for(int i=0;i<count;i++){
        node newNode(i,bound[0],(i%bound[0]),0);
        nodes.push_back(newNode);
    }
}

int toy_simulation(const string& file_input) {
    // read info
    ifstream file(file_input);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << file_input << std::endl;
        return -1;
    }
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        char command;
        iss >> command;

        switch (command) {
            case 'N':
                iss >> node_num;
                break;
            case 'L':
                iss >> length;
                break;
            case 'M':
                iss >> M;
                break;
            case 'R':
                int value;
                while (iss >> value) {
                    bound.push_back(value);
                }
                break;
            case 'T':
                iss >> total_time;
                break;

        }
    }
    //init and operation
    init_vec();

    int ticks = 0;
    int trans = 0;
    int to_time = total_time;
    double alltime = total_time;
    double trans_slots = 0;

    for (ticks = 0; ticks < to_time; ++ticks) {
        cout << ticks << ": ";
        for (int node_id = 0; node_id < node_num; ++node_id) {
            cout << nodes[node_id].backoff << "  ";
        }
        cout << endl;

        // collision_detection
        int zero_num = 0;

        for (int id_count = 0; id_count < node_num; id_count++) {
            if (nodes[id_count].backoff == 0) {
                ++zero_num;
            }
        }
        // count-down
        if (zero_num < 1) {
            for (int id_count = 0; id_count < node_num; id_count++) {
                --nodes[id_count].backoff;
            }
        }

        // transmission
        else if(zero_num==1){
            if(trans < length){
                ++trans;
                ++trans_slots;
            }
            if(trans == length){
                for (int id_count = 0; id_count < node_num; id_count++) {
                    if (nodes[id_count].backoff == 0) {
                        nodes[id_count].collision_count = 0; //reset
                        nodes[id_count].R = bound[nodes[id_count].collision_count];
                        nodes[id_count].backoff = (nodes[id_count].node_id + ticks + 1) % nodes[id_count].R;
                    }
                }
                trans = 0;
                //cout<<"trans"<<trans_slots<<endl;
            }
        }

        //collision
        else {
            for (int id_count = 0; id_count < node_num; id_count++) {
                if (nodes[id_count].backoff == 0) {
                    ++nodes[id_count].collision_count;
                    // M-th collision
                    if (nodes[id_count].collision_count == M) {
                        nodes[id_count].collision_count = 0; //reset
                        nodes[id_count].R = bound[nodes[id_count].collision_count];
                        nodes[id_count].backoff = (nodes[id_count].node_id + ticks + 1) % nodes[id_count].R;
                    } else {
                        nodes[id_count].R = bound[nodes[id_count].collision_count];
                        nodes[id_count].backoff = (nodes[id_count].node_id + ticks + 1) % nodes[id_count].R;
                    }
                }
            }
        }
    }
    cout<<"trans"<<trans_slots<<endl;
    // calculate utilization
    double utilization = trans_slots / alltime;
    fpOut << std::fixed << std::setprecision(2) << utilization;
    return 0;
}







/*        // collision_detection
        int zero_num=0;
        for(int id_count=0;id_count<node_num;id_count++){
            if(nodes[id_count].backoff==0){
                ++zero_num;
            }
        }

        if(zero_num==0){
            for(int id_count=0;id_count<node_num;id_count++){
                --nodes[id_count].backoff;
            }
        }

        // one transmission
        else if(zero_num==1){
                if(trans!=1) {
                    --trans;
                    ++trans_slots;
                    continue;
                }
                else{

                }
                else{
                    trans_slots+=(to_time+1-ticks);
                    ticks+=(to_time+1-ticks);
                }
                for(int id_count=0;id_count<node_num;id_count++){
                    if(nodes[id_count].backoff==0){
                        nodes[id_count].backoff=(nodes[id_count].node_id+ticks)%nodes[id_count].R;
                    }
                }

                ticks-=1;
        }


            // collision operations
        else{
            for(int id_count=0;id_count<node_num;id_count++){
                if(nodes[id_count].backoff==0){
                    ++nodes[id_count].collision_count;
                    // M-th collision
                    if(nodes[id_count].collision_count==M){
                        nodes[id_count].collision_count=0; //reset
                        nodes[id_count].R=bound[nodes[id_count].collision_count];
                        nodes[id_count].backoff=(nodes[id_count].node_id+ticks+1)%nodes[id_count].R;
                    }
                    else{
                        nodes[id_count].R=bound[nodes[id_count].collision_count];
                        nodes[id_count].backoff=(nodes[id_count].node_id+ticks+1)%nodes[id_count].R;
                    }
                }
            }
        }
    }*/


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }
    fpOut.open("output.txt");

    //Implementation
    string file_input;
    file_input=argv[1];

    toy_simulation(file_input);

    fpOut.close();
    return 0;
}

