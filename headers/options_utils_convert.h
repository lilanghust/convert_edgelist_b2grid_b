/**************************************************************************************************
 * Authors: 
 *   Zhiyuan Shao, Jian He, Huiming Lv 
 *
 * Declaration:
 *   Program parameter parsing.
 *
 * Notes:
 *   1.modify bfs::bfs_root to bfs::bfs-root by Huiming Lv   2014/12/21 
 *************************************************************************************************/

#ifndef __OPTIONS_UTILS_CONVERT_H__
#define __OPTIONS_UTILS_CONVERT_H__

#include <boost/program_options.hpp>
#include <iostream>

boost::program_options::options_description desc;
boost::program_options::variables_map vm;

static void setup_options_convert(int argc, const char* argv[])
{
  desc.add_options()
	( "help,h", "Display help message")
	( "type,t", boost::program_options::value<std::string>()->required(), "Type of the snap file(edgelist, edgelist_map or remap, ignore adjlist)")
	( "graph,g", boost::program_options::value<std::string>()->required(), "Name of the graph in snap format")
	( "destination,d",  boost::program_options::value<std::string>()->required(), "Destination folder that will contain the index and edge file")
	( "memory,m",  boost::program_options::value<unsigned long long>(), "How much memory can be used(MB)");
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc,
									     argv,
									     desc),
				  vm);
    boost::program_options::notify(vm);
  }
  catch (boost::program_options::error &e) {
    if(vm.count("help") || argc ==1) {
      std::cerr << desc << "\n";
    }
    std::cerr << "Error:" << e.what() << std::endl;
    std::cerr << "Try: " << argv[0] << " --help" << std::endl;
    exit(-1);
  }
}


#endif
