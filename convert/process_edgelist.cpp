/**************************************************************************************************
 * Authors: 
 *   Jian He,
 *
 * Routines:
 *   Process graph whose format is edge list.
 *   
 *************************************************************************************************/

#include <iostream>
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

FILE * edge_file;
int file_id;
//hejian-debug
unsigned int src_vert, dst_vert;

//when buffer is filled, write to the output index/edge file
// and then, continue reading and populating.
struct type2_edge type2_edge_buffer[EDGE_BUFFER_LEN];

/*
 * Regarding the vertex indexing:
 * We will assume the FIRST VERTEX ID SHOULD be 0!
 * Although in real cases, this will not necessarily be true,
 * (in many real graphs, the minimal vertex id may not be 0)
 * the assumption we made will ease the organization of the vertex
 * indexing! 
 * Since with this assumption, the out-edge offset of vertices
 * with vertex_ID can be easily accessed by using the suffix:
 * index_map[vertex_ID]
 */

void process_edgelist( const char* input_file_name,
        const char* edge_file_name,
        const char* out_dir,
        const char* origin_edge_file)
{
    unsigned long long num_edges_adjlist = 0;


    printf( "Start Processing %s.\nWill generate %s in destination folder.\n", 
            input_file_name, edge_file_name );

    int in = open( input_file_name, O_RDONLY|O_CREAT, 00666 );
    if( in == -1 ){
        printf( "Cannot open the input graph file!\n" );
        exit(1);
    }

    edge_file = fopen( edge_file_name, "w+" );
    if( NULL == edge_file ){
        printf( "Cannot create edge list file:%s\nAborted..\n",
                edge_file_name );
        exit( -1 );
    }

    memset( (char*)type2_edge_buffer, 0, EDGE_BUFFER_LEN*sizeof(struct type2_edge) );

    //init 
    lseek( in , 0 , SEEK_SET );	
    tmp_in_edge * buffer = (tmp_in_edge *)memalign(4096, IOSIZE);

    while ( true ){
        long bytes = read(in, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;
        //trace the vertex ids
        for(int i=0;i<count;++i){
            if( (buffer+i)->src_vert < min_vertex_id ) min_vertex_id = (buffer+i)->src_vert;
            if( (buffer+i)->dst_vert < min_vertex_id ) min_vertex_id = (buffer+i)->dst_vert;
            if( (buffer+i)->src_vert > max_vertex_id ) max_vertex_id = (buffer+i)->src_vert;
            if( (buffer+i)->dst_vert > max_vertex_id ) max_vertex_id = (buffer+i)->dst_vert;
        }
    }

    //init
    lseek( in , 0 , SEEK_SET );	
    
    printf( "convert to undirected edge list...\n" );       
    while ( true ){
        long bytes = read(in, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;

        for(int i=0;i<count;++i){
            if ((buffer+i)->src_vert == (buffer+i)->dst_vert)//remove self-cycle
                continue;
            (*(buf1 + current_buf_size)).src_vert = (buffer+i)->src_vert + 1 - min_vertex_id;
            (*(buf1 + current_buf_size)).dst_vert = (buffer+i)->dst_vert + 1 - min_vertex_id;
            ++current_buf_size;

            (*(buf1 + current_buf_size)).src_vert = (buffer+i)->dst_vert + 1 - min_vertex_id;
            (*(buf1 + current_buf_size)).dst_vert = (buffer+i)->src_vert + 1 - min_vertex_id;
            ++current_buf_size;

            if (current_buf_size == whole_buf_size)
            {
                //call function to sort and write back
                std::cout << "copy " << current_buf_size << " edges to sort..." << std::endl;
                wake_up_sort_src(file_id, current_buf_size, false);
                current_buf_size = 0;
                file_id++;
            }
        }
    }//while EOF
    //if current_buf_size == 0, we don't take into into consideration
    if (current_buf_size)
    {
        std::cout << "copy " << current_buf_size << " edges to sort..." << std::endl;
        num_edges_adjlist = wake_up_sort_src(file_id, current_buf_size, true);
        current_buf_size = 0;
    }

    //reinit the file pointer
    char * tmp_out_dir;
    tmp_out_dir = new char[strlen(out_dir)+1];
    strcpy(tmp_out_dir, out_dir);
    std::string tmp_file (tmp_out_dir);
    tmp_file += origin_edge_file;
    tmp_file += "-undirected-edgelist";
    in = open( tmp_file.c_str(), O_RDONLY|O_CREAT, 00666 ); 
    if( in == -1 ){ 
        printf( "Cannot open the new undirected edge list!\n" ); 
        exit(1); 
    } 

    printf( "generating adjacency list...\n" );       
    //write the first line
    fprintf(edge_file, "%u %llu\n", max_vertex_id-min_vertex_id+1, num_edges_adjlist/2);
    unsigned int recent_src_vert=UINT_MAX;
    unsigned long long edge_suffix = 0;
    unsigned long long prev_out = 1;
    unsigned long long index;
    while ( true ){
        long bytes = read(in, buffer, IOSIZE);
        long count = bytes / sizeof(tmp_in_edge);
        assert(bytes != -1);
        if(bytes==0) break;

        for(int i=0;i<count;++i, ++num_edges){
            if( num_edges == 0 && i == 0){
                num_edges = 1;
                recent_src_vert = (buffer+i)->src_vert;
            }
            if( (buffer+i)->src_vert == recent_src_vert ){
                edge_suffix = num_edges - prev_out;
                type2_edge_buffer[edge_suffix].dest_vert = (buffer+i)->dst_vert;
            }
            //assmume the max out-degree is less than 2^21
            else if ((buffer+i)->src_vert != recent_src_vert){
                if ( max_out_edges < (num_edges - prev_out) )
                    max_out_edges = num_edges - prev_out;
                //write a line
                for(index=0;index<edge_suffix;++index)
                    fprintf(edge_file, "%u ", type2_edge_buffer[index].dest_vert);
                fprintf(edge_file, "%u\n", type2_edge_buffer[index].dest_vert);
                //if the vetices is not continuous, it should add mutiple "\n"
                while(recent_src_vert+1 != (buffer+i)->src_vert){
                    ++recent_src_vert;
                    fprintf(edge_file, "\n");
                }
                //update the recent src vert id
                recent_src_vert = (buffer+i)->src_vert;
                prev_out = num_edges;
                edge_suffix = 0;
                type2_edge_buffer[0].dest_vert = (buffer+i)->dst_vert;
                //printf("suffix %dth is %d\n", num_edges, edge_suffix);
            }
        }
    }//while EOF
    //write the last line
    for(index=0;index<edge_suffix;++index)
        fprintf(edge_file, "%u ", type2_edge_buffer[index].dest_vert);
    fprintf(edge_file, "%u\n", type2_edge_buffer[index].dest_vert);

    //finished processing
    close( in );
    //remvoe undirected edgelist
    std::cout << "delete undirected edgelist " << tmp_file << std::endl;
    char tmp[1024];
    sprintf(tmp, "rm -rf %s", tmp_file.c_str());
    int ret = system(tmp);
    if(ret<0)
        assert(false);

    fclose( edge_file );
}

/*
 * this function will flush the content of buffer to file, fd
 * the length of the buffer should be "size".
 * Returns: -1 means failure
 * on success, will return the number of bytes that are actually 
 * written.
 */
unsigned long long flush_buffer_to_file( int fd, char* buffer, unsigned long long size )
{
    unsigned long long n, offset, remaining, res;
    n = offset = 0;
    remaining = size;
    while(n<size){
        res = write( fd, buffer+offset, remaining);
        n += res;
        offset += res;
        remaining = size-n;
    }
    return n;
}
