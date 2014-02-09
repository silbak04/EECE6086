//
// Copyright (C) 2014 by Samir Silbak
//
// Design Automation: KL Algorithm
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//

#include <iostream>
#include <climits>
#include <unordered_set>
#include <set>
#include <ctime>

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>

//#define DEBUG
//#define MEM_USAGE
//#define EXEC_TIME

using namespace std;

double start_time = 0.0;
double end_time   = 0.0;

#ifdef MEM_USAGE
// http://stackoverflow.com/questions/669438/how-to-get-memory-usage-at-run-time-in-c
void process_mem_usage(double& resident_set)
{
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat", ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime, vsize;

   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest
   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   resident_set = rss * page_size_kb;
}
#endif

int compute_cutset(unordered_set<int> *connections, set<int> group_a, set<int> group_b, int num_cells)
{
    set<int> group_a1;
    set<int> group_b1;

    int *d_value;
    d_value = new int[num_cells+1];

    int gen = 0;

    while (1)
    {
        group_a1 = group_a;
        group_b1 = group_b;
        gen++;

#ifdef DEBUG
        printf("------------------------------------\n");
        printf("          generation = %d\n", gen      );
        printf("------------------------------------\n");
#endif

        // compute d values for nodes
        for (int cell = 1; cell < num_cells+1; cell++)
        {
            d_value[cell] = 0;

            for (auto &node : connections[cell])
            {
                const bool cell_in_a = group_a.find(cell) != group_a.end();
                const bool node_in_a = group_a.find(node) != group_a.end();

                // is cell in 'group a' and node is not in 'group a' (an external connection)
                // and vice versa? if so, increment by 1, else decrement by 1.
                if ((cell_in_a && !node_in_a) || (!cell_in_a && node_in_a))
                    d_value[cell] += 1;
                else
                    d_value[cell] -= 1;
            }
        }

        int *gains;
        int *max_node_a;
        int *max_node_b;

        int *lock;

        int max_rduct_cost = 0;
        int reduction_cost = 0;

        gains      = new int[num_cells+1];
        max_node_a = new int[num_cells+1];
        max_node_b = new int[num_cells+1];

        lock       = new int[num_cells+1];

        int d      = INT_MIN;
        int dmax_a = INT_MIN;
        int dmax_b = INT_MIN;

        int max_a = -1;
        int max_b = -1;

        // if we find max gain between cells, then
        // let's mark it as locked (do not swap),
        // swap other nodes that are unlocked
        // we will be using the greedy pair method
        for (int i = 1; i < (num_cells/2)+1; i++)
        {
            dmax_a         = INT_MIN;
            dmax_b         = INT_MIN;
            max_rduct_cost = INT_MIN;

            for (auto &node_a1 : group_a1)
            {
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
                if (lock[node_b1]) continue;

                d = d_value[node_b1];
                if (d > dmax_b)
                {
                    dmax_b = d;
                    max_b  = node_b1;
                }
            }
            if (connections[max_a].find(max_b) != connections[max_a].end())
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
            // for those we just swapped
            for (auto &node : connections[max_node_a[i]])
            {
                const bool cell_in_a1 = group_a1.find(max_node_a[i]) != group_a1.end();
                const bool node_in_a1 = group_a1.find(node)          != group_a1.end();

                // is cell in 'group a' and node is not in 'group a' (an external connection)
                // and vice versa? if so, increment by 2, else decrement by 2.
                if ((cell_in_a1 && !node_in_a1) || (!cell_in_a1 && node_in_a1))
                    d_value[node] += 2;
                else
                    d_value[node] -= 2;
            }
            for (auto &node : connections[max_node_b[i]])
            {
                const bool cell_in_b1 = group_b1.find(max_node_b[i]) != group_b1.end();
                const bool node_in_b1 = group_b1.find(node)          != group_b1.end();

                // is cell in 'group a' and node is not in 'group a' (an external connection)
                // and vice versa? if so, increment by 2, else decrement by 2.
                if ((cell_in_b1 && !node_in_b1) || (!cell_in_b1 && node_in_b1))
                    d_value[node] += 2;
                else
                    d_value[node] -= 2;
            }
        }

#ifdef DEBUG
        for (int i = 1; i < num_cells+1; i++)
        {
            printf("d[%02d] = [%02d]\n", i, d_value[i]);
        }
        printf("--------------------------------\n");

        for (int i = 1; i < (num_cells/2)+1; i++)
        {
            printf("max reduction cost[%d] (%d, %d) = %d\n", i, max_node_a[i], max_node_b[i], gains[i]);
        }
#endif

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

#ifdef DEBUG
        printf("------------------------------------\n");
        printf("g_sum = %d | g_max = %d | k_max = %d\n", g_sum, g_max, k_max);
        printf("------------------------------------\n");
#endif

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

    delete [] connections;
    delete [] d_value;

    end_time = clock();
#ifdef EXEC_TIME
    printf("execution time = %0.3fs\n", (end_time - start_time) / CLOCKS_PER_SEC);
#endif

    return cut_size;
}

int main(int argc, char **argv)
{
    int node_1    = 0;
    int node_2    = 0;

    int num_cells = 0;
    int num_nets  = 0;

    set<int> group_a;
    set<int> group_b;

    start_time = clock();

    // first two lines from file
    // are the cell/net count
    cin >> num_cells;
    cin >> num_nets;

    unordered_set<int> *connection;
    connection = new unordered_set<int> [num_cells+1];

    while(cin >> node_1 >> node_2)
    {
        connection[node_1].insert(node_2);
        connection[node_2].insert(node_1);
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

#ifdef MEM_USAGE
    double rss;
    process_mem_usage(rss);
    printf("memory usage: %0.2fMB\n", rss / 1024.0);
#endif

    return 0;
}
