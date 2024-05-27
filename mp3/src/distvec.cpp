#include<stdio.h>
#include<vector>
#include<iostream>
#include<set>
#include<unordered_map>
#include<fstream>

using namespace std;

FILE *fpOut;

// forwarding-table
unordered_map<int, unordered_map<int, int> > topo;
unordered_map<int,unordered_map<int,pair<int,int> > > forwarding_table;//src, des, nexthop, cost

//nodes
set<int> N;

// message-file structure
typedef struct message{
    int src;
    int des;
    string msg;
    message(int src, int dest, string msg) : src(src), des(des), msg(msg) {}
}message;
vector<message> msgVec;

void printTopo(FILE *fpOut){
    for(auto src:N){
        //fprintf(fpOut,"\n\n\n");
        for(auto des:N){
            int next_hop=forwarding_table[src][des].first;
            int cost=forwarding_table[src][des].second;
            fprintf(fpOut,"%d %d %d\n",des,next_hop,cost);
        }
    }
};

void comple_table(){
    for(auto src:N){
        for(auto des:N){
            if(src==des){
                forwarding_table[src][des]= make_pair(des,0);
                topo[src][des]=0;
            }
            // not in the nodes
            if(topo.find(src)->second.end()==topo.find(src)->second.find(des)){
                topo[src][des]=-999;
                forwarding_table[src][des]= make_pair(des,-999);
            }
            forwarding_table[src][des]= make_pair(des,topo[src][des]);
        }
    }
}

void init_table(string file){
    //read topo_file
    ifstream read_topo;
    read_topo.open(file,ios::in);

    int src,des,cost;

    //read src,des,cost from file
    while(read_topo >> src >> des >> cost){
        topo[src][des]=cost;
        topo[des][src]=cost;
        N.insert(src);
        N.insert(des);
    }
    comple_table();
    read_topo.close();
}

void DV(){
    int next_hop, min_cost, temp_cost;
    int num=N.size();
    // use DV algorithm to converge table
    for(int i=0;i<num;++i){
        for(auto src:N){
            for(auto des:N){
                next_hop=forwarding_table[src][des].first;
                min_cost=forwarding_table[src][des].second;
                for(auto point:N){
                    if(topo[src][point]>=0&&forwarding_table[point][des].second>=0){
                        temp_cost=topo[src][point]+forwarding_table[point][des].second;
                        if(temp_cost<min_cost || min_cost==-999){
                            next_hop=point;
                            min_cost=temp_cost;
                        }
                    }
                }
                // refresh table
                forwarding_table[src][des]= make_pair(next_hop, min_cost);
            }
        }
    }
}

void RS_Message(string file){
    ifstream read_file;
    read_file.open(file,ios::in);
    if(!read_file){
        cout << "open messagefile error!" << endl;
    }
    string line, msg;

    while(getline(read_file,line)){
        int src,des;
        sscanf(line.c_str(), "%d %d ", &src, &des);
        msg = line.substr(line.find(" "));
        msg = msg.substr(line.find(" ") + 1);

        //print
        int nexthop = src;
        ::fprintf(fpOut,"from %d to %d cost ",src,des);
        //fpOut << "from " << src << " to " << des << " cost ";
        int cost = forwarding_table[src][des].second;
        if (cost == -999) {
            ::fprintf(fpOut,"infinite hops unreachable ");
        } else {
            ::fprintf(fpOut,"%d hops ", cost);
            while (nexthop != des) {
                ::fprintf(fpOut,"%d ",nexthop);
                nexthop = forwarding_table[nexthop][des].first;
            }
        }
        ::fprintf(fpOut,"message%s\n",msg.c_str());
    }

    read_file.close();
}

void change_topo(string file,string file2, FILE *fpOut){
    ifstream change_file;
    change_file.open(file,ios::in);
    if (!change_file) {
        cout << "open changesfile error!" << endl;
    }
    int src, des, cost;
    while(change_file >> src >> des >> cost){
        topo[src][des]=cost;
        topo[des][src]=cost;
        for(auto src:N){
            for(auto des:N){
                forwarding_table[src][des]= make_pair(des,topo[src][des]);
            }
        }
        comple_table();
        DV();
        printTopo(fpOut);
        RS_Message(file2);
    }
    change_file.close();
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    fpOut= fopen("output.txt", "w");
    string topology_file,message_file,changeTopo;

    // get file vars
    topology_file=argv[1];
    message_file=argv[2];
    changeTopo=argv[3];

    // call functions
    init_table(topology_file);
    DV();
    printTopo(fpOut);
    RS_Message(message_file);
    change_topo(changeTopo,message_file,fpOut);

    fclose(fpOut);

    return 0;
}

