#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <algorithm>
#include <utility>
#include <limits>
#include <climits>

using namespace std;

struct Edge {
    int destination;
    int cost;
    Edge(int dest, int c) : destination(dest), cost(c) {}
};

struct CompareNodeInfo {
    bool operator()(const pair<int, int>& p1, const pair<int, int>& p2) const {
        if (p1.first == p2.first)  // If distances are equal
            return p1.second > p2.second;  // Use node ID as tiebreaker (lower ID has higher priority)
        return p1.first > p2.first;  // Else, use distance
    }
};

FILE *fpOut;
map<int, vector<Edge> > graph;

struct EdgeComparator {
    int compareValue;
    EdgeComparator(int value) : compareValue(value) {}
    bool operator()(const Edge& edge) const {
        return edge.destination == compareValue;
    }
};

int numNodes;
vector<vector<int> > dist;
vector<vector<int> > nexthop;

void dijkstra(int src) {
    using NodeInfo = pair<int, int>;  // Pair of distance and node ID
    priority_queue<NodeInfo, vector<NodeInfo>, CompareNodeInfo> pq;

    dist[src - 1][src - 1] = 0;
    nexthop[src - 1][src - 1] = src;
    pq.push(make_pair(0, src));

    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        for (const Edge& edge : graph[u]) {
            int v = edge.destination;
            int weight = edge.cost;
            int newDist = dist[src - 1][u - 1] + weight;

            // Relaxation step with tiebreaker logic for equal distances
            if (newDist < dist[src - 1][v - 1]) {
                dist[src - 1][v - 1] = newDist;
                nexthop[src - 1][v - 1] = (u == src) ? v : nexthop[src - 1][u - 1];
                pq.push(make_pair(dist[src - 1][v - 1], v));
            }
        }
    }
}


void updateTopo() {
    numNodes = graph.size();
    dist.resize(numNodes, vector<int>(numNodes, INT_MAX));
    nexthop.resize(numNodes, vector<int>(numNodes, -1));
    //Apparently resize is not enough!!!
    for (auto& inner_vector : dist) {
        fill(inner_vector.begin(), inner_vector.end(), INT_MAX);
    }

    for (auto& inner_vector : nexthop) {
        fill(inner_vector.begin(), inner_vector.end(), -1);
    }

    for (int src = 1; src <= numNodes; src++) {
        dijkstra(src);
    }
}


void printTable() {
    for (int src = 1; src <= numNodes; src++) {
        for (int dest = 1; dest <= numNodes; dest++) {
            if (dist[src - 1][dest - 1] != INT_MAX) {
                fprintf(fpOut, "%d %d %d\n", dest, nexthop[src - 1][dest - 1], dist[src - 1][dest - 1]);
            }
        }
    }
    fprintf(fpOut, "\n");
}

void printMessage(const char *messagefile) {
    ifstream fpMsg(messagefile);
    string line;
    while (getline(fpMsg, line)) {
        istringstream iss(line);
        int src, dest;
        string message;
        iss >> src >> dest;
        iss.ignore();
        getline(iss, message);

        if (nexthop[src - 1][dest - 1] == -1) {
            fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n", src, dest, message.c_str());
        } else {
            fprintf(fpOut, "from %d to %d cost %d hops ", src, dest, dist[src - 1][dest - 1]);
            vector<int> hops;
            int cur_hop = src;
            while (cur_hop != dest) {
                hops.push_back(cur_hop);
                cur_hop = nexthop[cur_hop - 1][dest - 1];
            }
            for (const int& hop : hops) {
                fprintf(fpOut, "%d ", hop);
            }
            fprintf(fpOut, "message %s\n", message.c_str());
        }
    }
}

int applyChanges(const char *changefile, const char *messagefile) {
    ifstream fpChange(changefile);
    string line;
    while (getline(fpChange, line)) {
        istringstream iss(line);
        int src, dest, path_dist;
        iss >> src >> dest >> path_dist;

        auto& srcEdges = graph[src];
        auto& destEdges = graph[dest];

        if (path_dist == -999) {
            srcEdges.erase(remove_if(srcEdges.begin(), srcEdges.end(), EdgeComparator(dest)), srcEdges.end());
            destEdges.erase(remove_if(destEdges.begin(), destEdges.end(), EdgeComparator(src)), destEdges.end());
        } else {
            graph[src].erase(remove_if(graph[src].begin(), graph[src].end(), EdgeComparator(dest)), graph[src].end());
            graph[dest].erase(remove_if(graph[dest].begin(), graph[dest].end(), EdgeComparator(src)), graph[dest].end());
            srcEdges.push_back(Edge(dest, path_dist));
            destEdges.push_back(Edge(src, path_dist));
        }

        updateTopo();
        printTable();
        printMessage(messagefile);
    }
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    fpOut = fopen("output.txt", "w");

    ifstream fpTopo(argv[1]);
    string line;
    while (getline(fpTopo, line)) {
        istringstream iss(line);
        int src, dest, path_dist;
        iss >> src >> dest >> path_dist;
        graph[src].push_back(Edge(dest, path_dist));
        graph[dest].push_back(Edge(src, path_dist));
    }
    updateTopo();

    printTable();
    printMessage(argv[2]);
    applyChanges(argv[3], argv[2]);

    fclose(fpOut);
    return 0;
}
