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

#include "convert.h"
using namespace convert;
using namespace std;

#define LINE_FORMAT		"%u\t%u\n"
#define LINE_FORMAT_VERTEX		"%u\t%u\t%u\n"
#define LINE_FORMAT_PARTITION "%u\n"

unsigned long long edge_size;
static FILE * in_vertex;
int input_edge_file01;
int input_edge_file10;
std::ostringstream file_name;
static unsigned int *vtx_map;

void remap( const char* input_graph_name,
        const char* vertex_file_name,
        const char* out_dir,
        const char* input_file_name)
{
    cout << "Start Processing " << input_graph_name << "\nWill generate the grid files in destination folder.\n";

    //open the input file
    file_name.str("");
    file_name << input_graph_name << "-01";
    input_edge_file01 = open( file_name.str().c_str(), O_RDONLY|O_CREAT, 00666 );
    if( input_edge_file01 == -1 ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }
    file_name.str("");
    file_name << input_graph_name << "-10";
    input_edge_file10 = open( file_name.str().c_str(), O_RDONLY|O_CREAT, 00666 );
    if( input_edge_file10 == -1 ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }

    //mmap the vertex map file
    //do not need dual buffer, so mutiply 2
    edge_size = each_buf_len * 2 - sizeof(unsigned int) * (max_vertex_id +1);
    if(edge_size <= 0){
        cout << "Memory is not enough!\n";
        exit(1);
    }
    vtx_map = (unsigned int *)((char *)buf1 + edge_size);
    file_name.str("");
    file_name << input_graph_name << "-00-vertex-map";
    in_vertex = fopen( file_name.str().c_str(), "r" );
    if( in_vertex == NULL ){
        cout << "Cannot open the vertex file!\n";
        exit(1);
    }
    init_global_vertex_map();
    file_name.str("");
    file_name << input_graph_name << "-11-vertex-map";
    in_vertex = fopen( file_name.str().c_str(), "r" );
    if( in_vertex == NULL ){
        cout << "Cannot open the vertex file!\n";
        exit(1);
    }
    init_global_vertex_map();

    //generate the remap files
    //process file-01
    unsigned long long partition_size = edge_size / sizeof(struct tmp_in_edge);
    file_name.str("");
    file_name << out_dir << '/' << input_file_name << "-01-01";
    int output_edge_file = open(file_name.str().c_str(), O_WRONLY|O_CREAT, 00666);
    remap_one_file(input_edge_file01, output_edge_file, partition_size);
    close(input_edge_file01);
    close(output_edge_file);

    //process file-10
    file_name.str("");
    file_name << out_dir << '/' << input_file_name << "-10-10";
    output_edge_file = open(file_name.str().c_str(), O_WRONLY|O_CREAT, 00666);
    remap_one_file(input_edge_file10, output_edge_file, partition_size);
    close(input_edge_file10);
    close(output_edge_file);
}

void remap_one_file(int input_edge_file, int output_edge_file, unsigned long long partition_size){
    //init 
    lseek( input_edge_file , 0 , SEEK_SET );	
    tmp_in_edge * buffer = (tmp_in_edge *)memalign(4096, IOSIZE);

    unsigned long long size = 0;
    while ( true ){
        long bytes = read(input_edge_file, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;

        for(int i=0;i<count;++i){
            (buf1 + size)->src_vert  = vtx_map[(buffer+i)->src_vert];
            (buf1 + size)->dst_vert = vtx_map[(buffer+i)->dst_vert];
            ++size;
            if (size == partition_size)
            {
                //write back
                flush_buffer_to_file(output_edge_file, (char *)buf1, sizeof(tmp_in_edge) * size);
                size = 0;
            }
        }
    }//while EOF
    if(size)
        flush_buffer_to_file(output_edge_file, (char *)buf1, sizeof(tmp_in_edge) * size);
}

void init_global_vertex_map()
{
    static unsigned long long line_num=0;
    unsigned int temp1, temp2;
    char line_buffer[MAX_LINE_LEN];
    while(true)
    {
        char* res;

        if(( res = fgets( line_buffer, MAX_LINE_LEN, in_vertex )) == NULL )
            return;
        sscanf( line_buffer, LINE_FORMAT_VERTEX, &temp1, &temp2, vtx_map + line_num);
        ++line_num;
    }
}
