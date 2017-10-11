/**************************************************************************************************
 * Authors: 
 *   Zhiyuan Shao, Jian He
 *
 * Declaration:
 *   Structures and prototypes for graph format conversion.
 *************************************************************************************************/

#ifndef __CONVERT_H__
#define __CONVERT_H__

#define CUSTOM_EOF  	100
#define MAX_LINE_LEN   	1024 
#define EDGE_BUFFER_LEN 2048*2048  
#define VERT_BUFFER_LEN 2048*2048
#define IOSIZE          1048576*24

namespace convert
{

    struct out_edge_with_weight
    {
        unsigned int dest_vert;
        float edge_weight;
    }__attribute__((aligned(8)));

    struct out_edge_without_weight
    {
        unsigned int dest_vert;
        //float edge_weight;
    }__attribute__((aligned(8)));

    struct edge
    {
        unsigned int dest_vert;
        float edge_weight;
    }__attribute__((aligned(8)));

    struct type2_edge
    {
        unsigned int dest_vert;
    }__attribute__((aligned(4)));

    struct vert_index
    {
        unsigned long long offset;
    }__attribute__((aligned(8)));

    struct old_vert_index
    {
        unsigned int vert_id;
        unsigned long long offset;
    }__attribute__((aligned(8)));

    struct old_edge
    {
        unsigned int src_vert;
        unsigned int dest_vert;
        float edge_weight;
    }__attribute__((aligned(8)));

    struct tmp_in_edge
    {
        unsigned int src_vert;
        unsigned int dst_vert;
        tmp_in_edge(unsigned int _src_vert, unsigned int _dst_vert):src_vert(_src_vert),dst_vert(_dst_vert){}
        bool operator<(const tmp_in_edge &p) const{
            return src_vert < p.src_vert || (src_vert == p.src_vert && dst_vert < p.dst_vert);
        }
        bool operator==(const tmp_in_edge &p) const{
            return src_vert == p.src_vert && dst_vert == p.dst_vert;
        }
    }__attribute__((aligned(8)));

    struct in_edge
    {
        unsigned int in_vert;
    }__attribute__((aligned(4)));

    struct vertex_map
    {
        unsigned int partition_id;
        unsigned int old_id;
        unsigned int new_id;
    }__attribute__((aligned(4)));

    struct degree_info
    {
        unsigned int in_deg;
        unsigned int out_deg;
        degree_info(unsigned int _in_deg, unsigned int _out_deg):in_deg(_in_deg), out_deg(_out_deg){}
    }__attribute__((aligned(4)));
}

char *get_adjline();
unsigned long long flush_buffer_to_file( int fd, char* buffer, unsigned long long size );
void read_desc(std::string path);
void write_desc(std::string path);
void process_adjlist(const char*, const char *, const char *);
void process_edgelist(const char*, const char *, const char *, const char *);
bool comp_partition_id(const struct convert::vertex_map &, const struct convert::vertex_map &);
bool comp_old_id(const struct convert::vertex_map &, const struct convert::vertex_map &);
void edgelist_map(const char*, const char *, const char *, const char *, const char *);
void remap_one_file(int, int, unsigned long long);
void remap(const char*, const char *, const char *, const char *);
void statistics_one_file(int);
void statistics(const char*, const char *, const char *, const char *);
char *process_in_edge(unsigned long long, const char *, const char *);
void insert_sort_for_buf(unsigned int, unsigned int);
unsigned long long wake_up_sort_src(unsigned int, unsigned long long, bool);
void do_merge();
unsigned long long do_src_merge(char *, char *);
int read_one_edge( void );
void init_vertex_map( struct convert::vertex_map* );
unsigned long long init_global_vertex_map(unsigned long long, unsigned int *);
float produce_random_weight();
void *map_anon_memory( unsigned long long size, bool mlocked,bool zero = false );

extern FILE * in;
extern FILE * edge_file;
extern unsigned int src_vert, dst_vert;

extern FILE * out_txt;
extern FILE * old_edge_file;

extern unsigned int min_vertex_id, max_vertex_id;
extern unsigned long long  num_edges;
extern unsigned long max_out_edges;

extern struct convert::edge edge_buffer[EDGE_BUFFER_LEN];
extern struct convert::vert_index vert_buffer[VERT_BUFFER_LEN];
extern struct convert::type2_edge type2_edge_buffer[EDGE_BUFFER_LEN];
extern struct convert::in_edge in_edge_buffer[EDGE_BUFFER_LEN];
extern struct convert::vert_index in_vert_buffer[VERT_BUFFER_LEN];

//global vars for in_edge
extern struct convert::tmp_in_edge * buf1;
extern struct convert::tmp_in_edge * buf2;
extern unsigned long long each_buf_len;
extern unsigned long long whole_buf_size;
extern unsigned long long each_buf_size;
extern unsigned long long current_buf_size; //used in process_edgelist(adjlist)
extern int file_id;
extern unsigned long long * file_len;
extern unsigned int num_tmp_files;
extern const char * prev_name_tmp_file;
extern unsigned long long mem_size;
extern const char * in_name_file;

#endif

