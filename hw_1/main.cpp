#include <iostream>
#include <climits>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <vector>
#include <ctime>

//#define DEBUG

using namespace std;

double start_time = 0.0;
double end_time   = 0.0;

int compute_cutset(unordered_set<int> *connections, unordered_set<int> group_a, unordered_set<int> group_b, int num_cells)
{
    unordered_set<int> group_a1;
    unordered_set<int> group_b1;

    int *d_value;
    d_value = new int[num_cells+1];

    while (1)
    {
        group_a1 = group_a;
        group_b1 = group_b;

        // compute d values for nodes
        for (int cell = 1; cell < num_cells+1; cell++)
        {
            d_value[cell] = 0;

            //for (auto it = connections[cell].begin(); it != connections[cell].end(); it++)
            for (auto &node : connections[cell])
            {
                //int node = *it;

                const bool cell_in_a = group_a.find(cell) != group_a.end();
                const bool cell_in_b = group_b.find(cell) != group_b.end();
                const bool node_in_a = group_a.find(node) != group_a.end();
                const bool node_in_b = group_b.find(node) != group_b.end();

                if (cell_in_a == cell_in_b)
                    printf("[error]: cannot have two of the same cells in different groups\n");

                // is cell in group a and node in group b (an external connection)
                // vice versa? if so, increment by 1, else decrement by 1.
                if ((cell_in_a && node_in_b) || (cell_in_b && node_in_a))
                    d_value[cell] += 1;
                else
                    d_value[cell] -= 1;
            }
        }

        int *gains;
        int *max_node_a;
        int *max_node_b;

        int *lock;

        gains      = new int[num_cells+1];
        max_node_a = new int[num_cells+1];
        max_node_b = new int[num_cells+1];

        lock       = new int[num_cells+1];

        int max_rduct_cost = 0;
        int reduction_cost = 0;

        int d      = INT_MIN;
        int dmax_a = INT_MIN;
        int dmax_b = INT_MIN;

        int max_a = -1;
        int max_b = -1;

        // if we find max gain between cells, then
        // let's mark it as locked (do not swap),
        // swap other nodes that are not locked
        // we will be using the greedy pair method
        for (int i = 1; i < (num_cells/2)+1; i++)
        {
            //d              = INT_MIN;
            dmax_a         = INT_MIN;
            dmax_b         = INT_MIN;
            max_rduct_cost = INT_MIN;
            //reduction_cost = INT_MIN;

            bool node_in_a1 = false;
            bool node_in_b1 = false;

            for (auto &node_a1 : group_a1)
            {
                node_in_a1 = group_a1.find(i) != group_a1.end();

                if (lock[node_a1]) continue;

                d = d_value[node_a1];
                if (d > dmax_a)
                {
                    dmax_a = d;
                    max_a  = node_a1;
                }
            }
            for (auto &node_b1 : group_b1)
            {
                node_in_b1 = group_b1.find(i) != group_b1.end();

                if (lock[node_b1]) continue;

                d = d_value[node_b1];
                if (d > dmax_b)
                {
                    dmax_b = d;
                    max_b  = node_b1;
                }
            }
            if ((node_in_a1) && (node_in_b1))
                reduction_cost = dmax_a + dmax_b - 2;
            else
                reduction_cost = dmax_a + dmax_b;

            if (reduction_cost > max_rduct_cost){
                max_rduct_cost = reduction_cost;
                max_node_a[i]  = max_a;
                max_node_b[i]  = max_b;
            }
            gains[i] = max_rduct_cost;

            // swap a and b
            group_a1.erase( max_node_a[i]);
            group_b1.erase( max_node_b[i]);
            group_a1.insert(max_node_b[i]);
            group_b1.insert(max_node_a[i]);

            // lock nodes
            lock[max_node_a[i]] = 1;
            lock[max_node_b[i]] = 1;

            // now we need to update
            // dvalues for group a1/b1
            for (int cell = 1; cell < num_cells+1; cell++)
            {
                d_value[cell] = 0;

                for (auto &node : connections[cell])
                {
                    const bool cell_in_a1 = group_a1.find(cell) != group_a1.end();
                    const bool cell_in_b1 = group_b1.find(cell) != group_b1.end();
                    const bool node_in_a1 = group_a1.find(node) != group_a1.end();
                    const bool node_in_b1 = group_b1.find(node) != group_b1.end();

                    if (cell_in_a1 == cell_in_b1)
                        printf("[error]: cannot have two of the same cells in different groups\n");

                    // is cell in group a and node in group b (an external connection)
                    // vice versa? if so, increment by 1, else decrement by 1.
                    if ((cell_in_a1 && node_in_b1) || (cell_in_b1 && node_in_a1))
                        d_value[cell] += 1;
                    else
                        d_value[cell] -= 1;
                }
            }
        }

#ifdef DEBUG
        for (int i = 1; i < num_cells+1; i++)
        {
            printf("d[%02d] = [%02d]\n", i, d_value[i]);
        }
        printf("--------------------------------\n");
#endif
        for (int i = 1; i < (num_cells/2)+1; i++)
        {
            printf("max reduction cost[%d] (%d, %d) = %d\n", i, max_node_a[i], max_node_b[i], gains[i]);
        }

        int g_max = INT_MIN;
        int g_sum = 0;
        int k_val = 1;
        int k_max = 0;

        // find the maximum gain
        for (int i = 1; i < (num_cells)/2+1; i++)
        {
            g_sum += gains[i];
            if (g_sum > g_max){
                g_max = g_sum;
                k_max = k_val;
            }
            k_val += 1;
        }

//#ifdef DEBUG
        printf("------------------------------------\n");
        printf("g_sum = %d | g_max = %d | k_max = %d\n", g_sum, g_max, k_max);
        printf("------------------------------------\n");
//#endif

        // keep looping until g_max
        // is no longer > 0
        if (g_max <= 0) break;

        // we must exchange a[1]...a[k_max]
        // with b[1]...b[k_max]
        for (int k = 1; k < k_max+1; k++)
        {

#ifdef DEBUG
            printf("exchanging[%d]: (%d, %d)\n", k, max_node_a[k], max_node_b[k]);
#endif
            group_a.erase( max_node_a[k]);
            group_b.erase( max_node_b[k]);
            group_a.insert(max_node_b[k]);
            group_b.insert(max_node_a[k]);
        }

        delete [] max_node_a;
        delete [] max_node_b;
        delete [] lock;
    }

    // sum up all external connections
    // from group_a, this will be the
    // cutset size
    int cut_size = 0;
    for (int cell = 1; cell < num_cells+1; cell++)
    {
        for (auto &node : connections[cell])
        {
            const bool cell_in_a1 = group_a1.find(cell) != group_a1.end();
            const bool node_in_b1 = group_b1.find(node) != group_b1.end();

            // is cell in group a and node in group b (an external connection)
            // if so, increment cutsize by 1
            if (cell_in_a1 && node_in_b1) cut_size += 1;
        }
    }

    printf("1: cutset size = %d\n", cut_size);

    // put sets in vector to remove duplicates
    vector <int> a;
    vector <int> b;

    // group a
    for (int cell = 0; cell < (num_cells/2); cell++)
    {
        for (auto &node_a : group_a1)
        {
            a.push_back(node_a);
        }
    }

    printf("a: ");
    sort(a.begin(), a.end());
    a.erase(unique(a.begin(), a.end()), a.end());

    for (auto &node_a : a)
    {
        printf("%d ", node_a);
    }
    printf("\n");

    // group b
    for (int cell = 0; cell < (num_cells/2); cell++)
    {
        for (auto &node_b : group_b1)
        {
            b.push_back(node_b);
        }
    }

    printf("b: ");
    sort(b.begin(), b.end());
    b.erase(unique(b.begin(), b.end()), b.end());

    for (auto &node_b : b)
    {
        printf("%d ", node_b);
    }
    printf("\n");
    /*
    printf("a: ");
    for (auto &node_a : group_a1)
    {
        printf("%d ", node_a);
    }
    printf("\n");
    printf("b: ");
    for (auto &node_b : group_b1)
    {
        printf("%d ", node_b);
    }
    printf("\n");
    */

    delete [] connections;
    delete [] d_value;

    end_time = clock();
    printf("execution time = %0.3fs\n", (end_time - start_time) / CLOCKS_PER_SEC);

    return cut_size;
}

int main(int argc, char **argv)
{
    int node_1    = 0;
    int node_2    = 0;

    int num_cells = 0;
    int num_nets  = 0;

    int line      = 1;

    unordered_set<int> *connection;
    unordered_set<int> group_a;
    unordered_set<int> group_b;


    //string filename;
    //printf("input filename: ");
    //cin>>filename;
    //ifstream file(filename);

    start_time = clock();

    ifstream file(argv[1]);
    while (file.good())
    {
        // first two lines from file
        // are the cell/net count
        if (line == 1){
            file>>num_cells;
            line++;
            connection = new unordered_set<int> [num_cells+1];
        }
        if (line == 2){
            file>>num_nets;
            line++;
        }
        else{
            file>>node_1;
            file>>node_2;
            connection[node_1].insert(node_2);
            connection[node_2].insert(node_1);
        }
    }

#ifdef DEBUG
    printf("number of cells: %d\n", num_cells);
    printf("number of nets:  %d\n", num_nets);

    for (int i = 1; i < num_cells+1; i++)
    {
        for (auto &j : connection[i])
        {
            printf("node [%02d] -> [%02d]\n", i, j);
        }
    }
#endif

    // split cells into two groups - every other one
    for (int cell = 1; cell <= num_cells; cell++)
     {
         if (cell % 2)
             group_a.insert(cell);
         else
             group_b.insert(cell);
    }

    compute_cutset(connection, group_a, group_b, num_cells);

    return 0;
}
