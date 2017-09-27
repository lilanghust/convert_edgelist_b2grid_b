/**************************************************************************************************
 * Authors: 
 *   lang li,
 *
 * Routines:
 *   map the vertex.
 *   
 *************************************************************************************************/

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>

#include "convert.h"
using namespace convert;
using namespace std;

#define LINE_FORMAT_VERTEX		"%u\t%u\t%u\n"
    
map<unsigned int, struct degree_info> map_info;   
map<unsigned int, struct degree_info>::iterator iter;   

void statistics( const char* input_graph_name,
        const char* vertex_file_name,
        const char* out_dir,
        const char* input_file_name)
{
    cout << "Start Processing " << input_graph_name << "\nWill generate the grid files in destination folder.\n";

    std::ostringstream file_name;
    //open the input file
    file_name.str("");
    file_name << input_graph_name << "-01";
    int input_edge_file01 = open( file_name.str().c_str(), O_RDONLY|O_CREAT, 00666 );
    if( input_edge_file01 == -1 ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }
    file_name.str("");
    file_name << input_graph_name << "-10";
    int input_edge_file10 = open( file_name.str().c_str(), O_RDONLY|O_CREAT, 00666 );
    if( input_edge_file10 == -1 ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }

    //generate the remap files
    //process file-01
    file_name.str("");
    file_name << out_dir << '/' << input_file_name << "-antiDiag-degreeInfo";
    FILE * output_info_file = fopen(file_name.str().c_str(), "w+");
    statistics_one_file(input_edge_file01);
    close(input_edge_file01);

    statistics_one_file(input_edge_file10);
    close(input_edge_file10);
    
    //output
    for(iter=map_info.begin();iter!=map_info.end();++iter){
        fprintf(output_info_file, "%u, %u, %u\n", iter->first, iter->second.in_deg, iter->second.out_deg);
    }
    fclose(output_info_file);
}

void statistics_one_file(int input_edge_file){
    //init 
    lseek( input_edge_file , 0 , SEEK_SET );	
    tmp_in_edge * buffer = (tmp_in_edge *)memalign(4096, IOSIZE);

    while ( true ){
        long bytes = read(input_edge_file, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;

        for(int i=0;i<count;++i){
            iter = map_info.find((buffer+i)->src_vert);
            if(iter == map_info.end())
                map_info.insert(pair<int, degree_info>((buffer+i)->src_vert, degree_info(0,1)));
            else
                ++iter->second.out_deg;
            iter = map_info.find((buffer+i)->dst_vert);
            if(iter == map_info.end())
                map_info.insert(pair<int, degree_info>((buffer+i)->dst_vert, degree_info(1,0)));
            else
                ++iter->second.in_deg;
        }
    }//while EOF
}
