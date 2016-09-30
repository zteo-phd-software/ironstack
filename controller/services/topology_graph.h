#pragma once
#include <string>
#include <mutex>
#include <map>
#include <vector>
#include <thread>
#include <set>
#include <queue>
#include "../../common/timer.h"
#include "../../common/mac_address.h"
#include "../../common/ip_address.h"
#include "../../common/preallocated.h"
#include "../ironstack_types/openflow_port.h"
//#include "switch_state.h"

//class switch_state;

// Ironstack topology graph
//
// maintains information about other Ironstack switches 
// in the network via a graph structure 
//
// author: Noah Apthorpe (apthorpe@cs.cornell.edu)
// revision 1

//class network_node;
//class port_state;

class topology_graph {
 public:

  // constructors
  topology_graph();
  //topology_graph(const topology_graph& original);
  //topology_graph& operator=(const topology_graph& original);

  // clears the entire topology
  void clear();

  // initializes the topology graph and starts the topology graph daemon
  void init(mac_address my_mac_addr, ip_address my_ip_addr, std::string my_name, std::vector<openflow_port> my_ports,
	    std::function<void(autobuf*)> broadcast_data_fn,
	    preallocated<autobuf>* allocator_ptr);

  // stops the topology graph daemon
  void shutdown();

  // update the status of a port on the local switch
  void update_local_port(openflow_port& modified_port);

  // update the status of a neighbor  IronStack switch
  void update_neighbor(uint16_t phy_port, mac_address neighbor_mac, 
		       ip_address neighbor_ip, autobuf* data);

  // update the status of a distant (non-neighbor) Ironstack switch
  void update_topology(mac_address mac_addr, ip_address ip_addr, autobuf* data);

  // get the shortest path (in number of hops) between two
  // switches in the network
  std::vector<mac_address> get_shortest_path(mac_address source, mac_address dest);

  // get a list of all disjoint paths sorted by length 
  // (in number of hops) between two switches in the network
  std::vector<std::vector<mac_address>> get_disjoint_paths(mac_address source, mac_address dest);

  // get physical port connection to neighbor, 
  //return false if no connection exists
  bool get_neighbor_port(mac_address neighbor, uint16_t* port_num);

  // get the local node's information serialized into an autobuf
  uint32_t serialize_local_node(autobuf& dest);

  // get known data about self and neighbor connections
  //uint32_t get_data(autobuf& dest);

  // convert topology graph into readable output
  std::string to_string();
  //graph_to_json
  //me_to_json

  //private:

  // class for port state information
  class port_state {
  public:
    
    // constructor
    port_state();
    
    // assignment operator
    //port_state operator= (port_state& other);
    bool update(openflow_port& modified_port);
    
    // serialization/deserialization functions
    //void serialize(autobuf& dest);
    //uint32_t deserialize(const autobuf& source);
    void pack(uint8_t* dest) const;
    void unpack(const uint8_t* input);
    std::string to_string() const;

    // instance variables
    uint16_t port_num; 
    bool online;
    bool is_stp_port; // not used
    bool is_flood_port;  // not used
    std::string name;

    // struct for serialization
    typedef struct {
      uint8_t port_num[2];
      uint8_t online;
      uint8_t is_stp_port;
      uint8_t is_flood_port;
      uint8_t name[40];
    } port_state_hdr;
  };
  

  // class defining an ironstack network node
  class network_node {
  public:

    // constructor
    network_node();

    // assignment operator used for updates
    //network_node operator= (network_node& other);

    // clears all information about the node
    void clear();
    
    // updates the node with data sent over the network
    //void update(autobuf* data, uint32_t time);

    // serialization/deserialization methods
    uint32_t serialize(autobuf& dest) const;
    bool deserialize(const autobuf& source);
    std::string to_string() const;

    // instance variables
    uint32_t local_refresh;         // in ms
    uint32_t last_refresh;         // in ms
    uint8_t seq_num;
    mac_address mac_addr;          // mac address of node
    ip_address ip_addr;            // ip address of node
    bool blacklisted;              // has this node been blacklisted
    bool whitelisted;              // has this node been whitelisted
    std::string name;              // name of switch
    std::map<uint16_t, port_state> ports;  // info about ports 
    std::map<uint16_t, std::tuple<mac_address, uint32_t>> links; // links out of this node by (physical port, (neighbor mac, local refresh))
    //std::map<mac_address, std::tuple<uint16_t, uint32_t>> links; // links out of this node by (neighbor MAC, (local port, local refresh))    
  
    // struct for serilization
    typedef struct {
      uint8_t last_refresh[4];
      uint8_t seq_num;
      uint8_t mac_addr[6];
      uint8_t ip_addr[4];
      uint8_t name[40];
      uint8_t num_ports[4];
      uint8_t num_links[4];
    } network_node_hdr;
  };

 private:

  class simple_graph {
  public:
    std::map<mac_address, std::set<mac_address>> edges;
    std::map<std::pair<mac_address, mac_address>, bool> flows;
    void add_edge(mac_address source, mac_address dest);
    void false_all_flows();
    void true_flow(mac_address source, mac_address dest);
    void false_flow(mac_address source, mac_address dest);
    bool get_flow(mac_address source, mac_address dest);
    uint32_t max_flow(mac_address source, mac_address sink);
    std::vector<mac_address> bfs(mac_address source, mac_address dest, bool use_flows);
  };
  
  // instance variables - topology graph
  network_node me;  // information about local switch and connections to neighbors
  std::map<mac_address, network_node> other_nodes; // information about non-local switches
  uint32_t last_update_sent_time;
  //neighbor_connections;
  //network_links;
  
  // parameters
  const double HEARTBEAT_INTERVAL = 2; // in SECONDS
  const double STALE_TIMEOUT = 4000; // in MILLISECONDS

  // instance variables - management details
  bool initialized;                    // has the topology graph been initialized
  preallocated<autobuf>* allocator;    // pointer to autobuf memory allocator
  //switch_state* switch_state_service;  // pointer to service with switch state info
  //uint32_t tbarrier_id;                // id of timed barrier for communication with discovery service
  std::function<void(autobuf*)> broadcast_data;    // function that will broadcast the data in its argument autobuf
  std::thread topology_daemon_tid;     // id of topology daemon thread
  //std::mutex tbarrier_lk;              // lock for signalling on the timed barrier
  std::mutex me_lk;                    // lock for updating local switch info
  std::mutex other_nodes_lk;           // lock for other nodes info
  //  std::map<std::mac_address, std::mutex> other_nodes_lks; // locks for updating non-local switch info

  // entry point for the topology graph mantainance daemon
  void topology_daemon_entry();

  // lock entire other nodes map
  //  std::vector<std::unique_lock<std::mutex>> lock_all_other_nodes();
  
  // broadcast local status
  void send_update();

  // convert to simplegraph
  simple_graph to_simple_graph();

};


  
  

