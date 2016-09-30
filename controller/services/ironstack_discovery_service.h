#pragma once
#include <string>
//#include <mutex>
//#include <map>
#include <map>
#include <mutex>
#include <vector>
#include <tuple>
#include <thread>
#include "topology_graph.h"
#include "switch_state.h"
#include "../hal/hal.h"
#include "../hal/l3_handler.h"
#include "../hal/service_catalog.h"
#include "../ironstack_types/openflow_port.h"
#include "../../common/timed_barrier.h"
#include "../../common/mac_address.h"
#include "../../common/ip_address.h"
#include "../../common/common_utils.h"
#include "../../common/preallocated.h"
#include "../../common/timer.h"

class switch_state;

// Ironstack discovery service
//
// discovers network topology by sending and 
// handling non-fowardable packets 
// with destination MAC and destination IP both nil
//
// author: Noah Apthorpe (apthorpe@cs.cornell.edu)
// revision 1

//class switch_state;
//class topology_graph;

class discovery_service : public service, public udp_handler {
 public:
  
  // constructor/destructor 
  discovery_service(service_catalog* serv_catalog_ptr, preallocated<autobuf>* allocator_ptr);
  virtual ~discovery_service() {};

  // initializer/shutdown functions called by main
  virtual void init();
  virtual void shutdown();

  // get debugging info about the service (required by superclass)
  virtual std::string get_service_info() const;
  virtual std::string get_running_info() const;

  // packet handler for discovery service packets
  virtual bool udp_callback(const udp_packet& pkt, const autobuf& original);

  // handler for local switch port state changes
  void discovery_port_handler(uint16_t phy_port);

  // gets all the flood ports of the local switch
  std::vector<uint16_t> get_flood_ports();

  // gets shortest path (by number of hops) between two switches
  std::vector<mac_address> get_shortest_path(mac_address source, mac_address dest);

  // gets disjoint paths between two switches
  std::vector<std::vector<mac_address>> get_disjoint_paths(mac_address source, mac_address dest);

  // serializes the local topology information into an autobuf
  uint32_t serialize_local_topology_info(autobuf& dest);

  // gets the physical port to a particular neighbor
  bool get_neighbor_port(mac_address neighbor, uint16_t* port_number);

 private:
  
  // const parameters
  const uint16_t NEIGHBOR_UPDATE_PORT = 100; // port for neighbor update packets
  //const uint16_t NEIGHBOR_NOP_PORT = 101;    // port for neighbor still-alive packets
  const uint16_t TOPOLOGY_UPDATE_PORT = 102; // port for topology update packets
  // const uint16_t TOPOLOGY_NOP_PORT = 103;    // port for topology still-alive packets
  // const double REFRESH_INTERVAL = 2;         // in SECONDS
  const double CLEAN_BROADCASTS_SEEN_INTERVAL = 10; // in SECONDS
  const double STALE_BROADCASTS_SEEN_INTERVAL = 10000;  // in MILLISECONDS

  // instance variables
  bool initialized;                   // has the service been started
  topology_graph topology;            // graph with all locally known topology info
  hal* controller;                    // pointer to hardware-abstraction controller
  preallocated<autobuf>* allocator;   // pointer to autobuf memory allocator
  //service_catalog* serv_catalog;      // pointer to the service catalog
  switch_state* switch_state_service; // pointer to service with switch state info
  uint32_t last_timestamp;            // timestamp of  last discovery packet sent
  uint8_t next_seq_num;               // sequence number of next discovery packet 
  uint32_t packet_callback_id;        // id of registered discovery packet handler callback
  uint32_t port_callback_id;          // id of registered port state change handler callback
  std::thread discovery_tid;          // id of thread running discovery service daemon
  //uint32_t tbarrier_id;               // id of timed barrier for communication with topology graph
  std::map<mac_address, std::tuple<uint32_t,uint8_t>> broadcasts_seen;
  std::mutex broadcasts_seen_lk;

  // discovery service daemon entrypoint
  void discovery_daemon_entry();   

  // send a discovery packet, either hello or helloACK
  //void send_discovery_packet(uint16_t phy_port, uint16_t src_port);
  //void send_discovery_packet(uint16_t udp_port, autobuf* data);
  
  // broadcast all info about local switch and connections to its neighbors
  void broadcast_data(autobuf* data);

  void clean_broadcasts_seen();

  // forward-broadcast a discovery packet and change its udp port
  void fwd_broadcast_change_udp_port(const udp_packet& pkt, uint16_t port);
  
  // forward-broadcast a discovery packet without changing its udp port
  void fwd_broadcast_same_udp_port(const udp_packet& pkt);

  // class defining a discovery service packet
  class discovery_packet {
  public: 
   
    // packet header structure
    typedef struct {
      uint8_t src_mac[6];   // MAC of local switch
      uint8_t src_ip[4];    // IP of local controller
      uint8_t timestamp[4]; // ms since epoch
      uint8_t seq_num;      // to disambiguate between identical timestamps
      uint8_t name[40];     // name of local switch - null terminated
      uint8_t data[0];      // data array
    } discovery_packet_hdr;

    // user addressable packet fields
    mac_address src_mac;
    ip_address src_ip;
    uint32_t timestamp;
    uint8_t seq_num;
    std::string name; // not currently used
    autobuf data; // should this be a pointer or reference to an autobuf??
 
    // constructor and function to clear packet fields
    discovery_packet();
    void clear();

    // serialize and deserialize methods 
    uint32_t serialize(autobuf& dest) const;
    bool deserialize(const autobuf& pkt_content);

    // convert to string for debugging
    std::string to_string() const;
  };

  bool first_time_seen(const discovery_packet& disc_packet);

};

    

  
