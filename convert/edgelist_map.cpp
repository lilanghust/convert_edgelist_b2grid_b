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

#define LINE_FORMAT		"%d\t%d\n"
#define LINE_FORMAT_VERTEX		"%u\t%u\t%u\n"
#define LINE_FORMAT_PARTITION "%u\n"

static FILE * in_vertex;
static struct vertex_map *vtx_map;

void edgelist_map( const char* input_graph_name,
        const char* edge_file_name, 
        const char* vertex_file_name,
        const char* out_dir,
        const char* input_file_name)
{
    cout << "Start Processing " << input_graph_name << "\nWill generate the grid files in destination folder.\n";

    int in = open( input_graph_name, O_RDONLY|O_CREAT, 00666 );
    if( in == -1 ){
        cout << "Cannot open the input graph file!\n";
        exit(1);
    }

    in_vertex = fopen( vertex_file_name, "r" );
    if( in_vertex == NULL ){
        cout << "Cannot open the vertex file!\n";
        exit(1);
    }

    //mmap the partition file, add old_id according the line_num
    vtx_map = (struct vertex_map *)map_anon_memory( sizeof(struct vertex_map) * (max_vertex_id+1), true, true  );
    init_vertex_map( vtx_map );

    //sort the vtx_map by the first column(partition_id)
    std::sort(vtx_map, vtx_map + max_vertex_id + 1, comp_partition_id);
    //get each partition's vertex size
    unsigned int count = 0;
    unsigned int key = vtx_map[0].partition_id;
    for(unsigned int i=0;i<=max_vertex_id;++i)
        if(vtx_map[i].partition_id == key)
            ++count;
        else{
            printf("partition %u : vertex size %u\n", key, count);
            count = 1;
            key = vtx_map[i].partition_id;
        }
    printf("partition %u : vertex size %u\n", key, count);
    int partition = vtx_map[max_vertex_id].partition_id + 1;

    //assign new_id by the order
    for(unsigned int i=0;i<=max_vertex_id;++i)
        vtx_map[i].new_id = min_vertex_id + i;

    //sort the vtx_map by the second column(old_id)
    std::sort(vtx_map, vtx_map + max_vertex_id + 1, comp_old_id);
    
    //generate the grid files
    std::ostringstream file_name;
    //do not need dual buffer, so mutiply 2
    long long partition_size = each_buf_size * 2 / partition / partition;
    long long ** size = new long long*[partition];
    struct tmp_in_edge *** grid_buf = new struct tmp_in_edge **[partition];
    int ** grid_file = new int*[partition];
    for(int i=0;i<partition;++i){
        size[i] = new long long[partition];
        grid_buf[i] = new struct tmp_in_edge *[partition];
        grid_file[i] = new int[partition];
        for(int j=0;j<partition;++j){
            size[i][j] = 0;
            grid_buf[i][j] = buf1 + (i * partition + j) * partition_size;
            file_name.str("");
            file_name << out_dir << '/' << input_file_name << '-' << i << j;
            grid_file[i][j] = open(file_name.str().c_str(), O_WRONLY|O_CREAT, 00666);
        }
    }


    file_name.str("");
    file_name << out_dir << '/' << input_file_name << "-vertex-map";
    FILE *vertex_map_file = fopen( file_name.str().c_str(), "w+" );
    if( NULL == vertex_map_file){
        cout << "Cannot create vertex_map_file:" << file_name.str().c_str() << "\nAborted..\n";
        exit( -1 );
    }

    cout << "generating grid...\n";
    //init 
    lseek( in , 0 , SEEK_SET );	
    tmp_in_edge * buffer = (tmp_in_edge *)memalign(4096, IOSIZE);

    int row, col;
    while ( true ){
        long bytes = read(in, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;

        for(int i=0;i<count;++i){
            row = vtx_map[(buffer+i)->src_vert-min_vertex_id].partition_id;
            col = vtx_map[(buffer+i)->dst_vert-min_vertex_id].partition_id;
            (grid_buf[row][col] + size[row][col])->src_vert  
                = vtx_map[(buffer+i)->src_vert-min_vertex_id].new_id;
            (grid_buf[row][col] + size[row][col])->dst_vert 
                = vtx_map[(buffer+i)->dst_vert-min_vertex_id].new_id;
            ++size[row][col];
            if (size[row][col] == partition_size)
            {
                //write back
                flush_buffer_to_file(grid_file[row][col], (char*)(grid_buf[row][col]), size[row][col]*sizeof(tmp_in_edge));
                size[row][col] = 0;
            }
        }
    }//while EOF
    for(row=0;row<partition;++row){
        for(col=0;col<partition;++col){
            cout << "line[" << row << "][" << col << "]:" << size[row][col] << endl;
            if(size[row][col])
                flush_buffer_to_file(grid_file[row][col], (char*)(grid_buf[row][col]), size[row][col]*sizeof(tmp_in_edge));
        }
    }

    //write back the vertex map file
    for(unsigned int i=0;i<=max_vertex_id;++i)
        fprintf(vertex_map_file, LINE_FORMAT_VERTEX, vtx_map[i].partition_id, vtx_map[i].old_id, vtx_map[i].new_id);
    fclose(vertex_map_file);

    //finished processing
    close( in );
    
    //close grid files
    for(int i=0;i<partition;++i){
        for(int j=0;j<partition;++j)
            close(grid_file[i][j]);
    }
}

bool comp_partition_id(const struct vertex_map &v1, const struct vertex_map &v2){
    return v1.partition_id < v2.partition_id || (v1.partition_id == v2.partition_id && v1.old_id < v2.old_id);
}

bool comp_old_id(const struct vertex_map &v1, const struct vertex_map &v2){
    return v1.old_id < v2.old_id;
}

void init_vertex_map( struct vertex_map * vtx_map )
{
    unsigned long long line_num=0;
    char line_buffer[MAX_LINE_LEN];
    while(true)
    {
        char* res;

        if(( res = fgets( line_buffer, MAX_LINE_LEN, in_vertex )) == NULL )
            return;
        sscanf( line_buffer, LINE_FORMAT_PARTITION, &vtx_map[line_num].partition_id);
        vtx_map[line_num].old_id = line_num;
        ++line_num;
    }
}
