/**************************************************************************************************
 * Authors: 
 *   Jian He,
 *
 * Routines:
 *   process in-edge
 *   
 *************************************************************************************************/

#include <iostream>
#include <sstream>
#include <cassert>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "convert.h"
using namespace convert;

typedef unsigned long long u64_t;
typedef unsigned int u32_t;
struct in_edge in_edge_buffer[EDGE_BUFFER_LEN];
struct vert_index in_vert_buffer[VERT_BUFFER_LEN];

FILE *in_edge_fd;
struct tmp_in_edge * buf1, *buf2;
u64_t each_buf_len;
u64_t whole_buf_size; //How many edges can be stored in this buf
u64_t each_buf_size; //How many edges can be stored in this buf
u32_t num_parts; //init to 0, add by bufs
u64_t *file_len;
struct tmp_in_edge * edge_buf_for_sort;
char * buf_for_sort;
char * tmp_out_dir;
char * origin_edge_file;
u32_t num_tmp_files;
const char * prev_name_tmp_file;
const char * in_name_file;

u64_t current_buf_size;
u64_t total_buf_size;
u64_t total_buf_len;
u32_t current_file_id;

enum
{
    READ_FILE = 0,
    WRITE_FILE
};

void *map_anon_memory( u64_t size,
        bool mlocked,
        bool zero)
{
    void *space = mmap(NULL, size > 0 ? size:4096,
            PROT_READ|PROT_WRITE,
            MAP_ANONYMOUS|MAP_SHARED, -1, 0);
    printf( "Engine::map_anon_memory had allocated 0x%llx bytes at %llx\n", size, (u64_t)space);
    if(space == MAP_FAILED) {
        std::cerr << "mmap_anon_mem -- allocation " << "Error!\n";
        exit(-1);
    }
    if(mlocked) {
        if(mlock(space, size) < 0) {
            std::cerr << "mmap_anon_mem -- mlock " << "Error!\n";
        }
    }
    if(zero) {
        memset(space, 0, size);
    }
    return space;
}

void do_io_work(const char *file_name_in, u32_t operation, char* buf, u64_t offset_in, u64_t size_in)
{
    int fd;
    switch(operation)
    {
        case READ_FILE:
            {
                int read_finished = 0, remain = size_in, res;
                fd = open(file_name_in, O_RDWR, S_IRUSR | S_IRGRP | S_IROTH); 
                if (fd < 0)
                {
                    printf( "Cannot open attribute file for writing!\n");
                    exit(-1);
                }
                if (lseek(fd, offset_in, SEEK_SET) < 0)
                {
                    printf( "Cannot seek the attribute file!\n");
                    exit(-1);
                }
                while (read_finished < (int)size_in)
                {
                    if( (res = read(fd, buf, remain)) < 0 )
                    {
                        printf( "Cannot seek the attribute file!\n");
                        exit(-1);
                    }
                    read_finished += res;
                    remain -= res;
                }
                close(fd);
                break;
            }
        case WRITE_FILE:
            {
                int written = 0, remain = size_in, res;
                fd = open(file_name_in, O_RDWR, S_IRUSR | S_IRGRP | S_IROTH); 
                if (fd < 0)
                {
                    printf( "Cannot open attribute file for writing!\n");
                    exit(-1);
                }
                if (lseek(fd, offset_in, SEEK_SET) < 0)
                {
                    printf( "Cannot seek the attribute file!\n");
                    exit(-1);
                }
                while (written < (int)size_in)
                {
                    if( (res = write(fd, buf, remain)) < 0 )
                    {
                        printf( "Cannot seek the attribute file!\n");
                        exit(-1);
                    }
                    written += res;
                    remain -= res;
                }
                close(fd);
                break;
            }
    }
}

char* process_in_edge(u64_t mem_size,
        const char * edge_file_name,
        const char * out_dir)
{
    tmp_out_dir = new char[strlen(out_dir)+1];
    strcpy(tmp_out_dir, out_dir);

    origin_edge_file = new char[strlen(edge_file_name)+1];
    strcpy(origin_edge_file, edge_file_name);

    num_parts = 0;
    each_buf_len = mem_size/2;
    whole_buf_size = (u64_t)mem_size/(sizeof(struct tmp_in_edge));
    each_buf_size = (u64_t)mem_size/(sizeof(struct tmp_in_edge)*2);
    current_buf_size = 0;
    current_file_id = 0;

    buf_for_sort = (char *)map_anon_memory(mem_size, true, true );
    edge_buf_for_sort = (struct tmp_in_edge *)buf_for_sort;
    buf1 = (struct tmp_in_edge *)buf_for_sort;
    buf2 = (struct tmp_in_edge *)(buf_for_sort + each_buf_len);

    return buf_for_sort;
}


u64_t wake_up_sort_src(u32_t file_id, u64_t buf_size, bool final_call)
{
    /*start sort for this buffer*/
    std::sort(buf1, buf1 + buf_size);

    /*the src file is small, so it just needs writing not merging*/
    std::stringstream str_file_id;
    str_file_id << file_id;
    std::string tmp_file_name(tmp_out_dir) ;
    tmp_file_name += origin_edge_file;

    if (final_call && file_id == 0)
        tmp_file_name += "-undirected-edgelist";
    else
        tmp_file_name += "-tmp-file-" + str_file_id.str();

    int tmp_in_file = open( tmp_file_name.c_str(), O_WRONLY|O_CREAT, 00666 );
    if ( tmp_in_file == -1 ) {
        printf( "Cannot tmp_file: %s\nAborted..\n", tmp_file_name.c_str());
        exit( -1 );
    }
    printf("final_call is %d,file_id is %d\n",final_call,file_id);
    if (final_call && file_id == 0){
        unsigned long long del_num_edges = 0;
        tmp_in_edge *p_last = buf1; 
        tmp_in_edge *p = buf1 + 1; 
        for(u64_t i = 1;i < buf_size; ++i, ++p){
            //delete the replications
            if( *p == *p_last){
                ++del_num_edges;
                continue;
            }
            ++p_last;
            *p_last = *p;
        }
        flush_buffer_to_file(tmp_in_file, (char *)buf1, sizeof(tmp_in_edge) * (buf_size - del_num_edges));
        close(tmp_in_file);
        printf("file_id=%d\n",file_id);
        std::cout << "buf_size: " << buf_size << "\tdel: " << del_num_edges << std::endl;
        return buf_size - del_num_edges; 
    }
    else{
        flush_buffer_to_file(tmp_in_file, (char *)buf1, sizeof(tmp_in_edge) * buf_size);
        close(tmp_in_file);
    }

    if (final_call && file_id != 0)
    {
        num_tmp_files = file_id + 1;
        file_len = new u64_t[file_id + 1];
        for (u32_t i = 0; i <= file_id; i++)
        {
            if (i == file_id)
                file_len[i] = buf_size*sizeof(tmp_in_edge);
            else
                file_len[i] = whole_buf_size*sizeof(tmp_in_edge);//std::sort is used to sort the whole buffer
        }

        for (u32_t j = 0; j <= file_id; j++)
            std::cout << "the size of tmp file[" <<j << "] is:" <<  file_len[j] << std::endl;
        std::string prev_tmp(tmp_out_dir) ;
        prev_tmp += origin_edge_file;
        prev_tmp += "-tmp-file-";
        prev_name_tmp_file = prev_tmp.c_str();

        return do_src_merge(tmp_out_dir, origin_edge_file);

        //munlock( buf_for_sort, mem_size);
        //munmap( buf_for_sort, mem_size);
    }
    return 0;
}

