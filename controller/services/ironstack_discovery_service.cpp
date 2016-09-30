#include "ironstack_discovery_service.h"

////////////////////////////////////////
// INITIALIZATION / SHUTDOWN METHODS 
////////////////////////////////////////

// constructor
discovery_service::discovery_service(service_catalog* serv_catalog_ptr,
				     preallocated<autobuf>* allocator_ptr) :
  service(serv_catalog_ptr, service_catalog::service_type::TOPOLOGY, 1, 0)
  //allocator(allocator_ptr)
{
  allocator = allocator_ptr;
  initialized = false;
  controller = serv_catalog_ptr->get_controller();
  next_seq_num = 0;
  last_timestamp = 0;
}

// starts the discovery service and initializes member values
void discovery_service::init() {
  if (initialized) return;

  // get the switch state service, create the timed barrier, and initialize the topology graph
  switch_state_service = (switch_state*) service_catalog_ptr->get_service(service_catalog::service_type::SWITCH_STATE);
  //tbarrier_id = timed_barrier::create(REFRESH_INTERVAL);
  //topology.init(switch_state_service, tbarrier_id, allocator);
  topology.init(switch_state_service->get_switch_mac(), switch_state_service->get_switch_ip(),
		switch_state_service->get_name(), switch_state_service->get_switch_ports(),
		std::bind(&discovery_service::broadcast_data, this, std::placeholders::_1), allocator);
  // mark as initialized, register UDP packet and port modification callbacks, start daemon, send first packets
  initialized = true;
  controller->get_l3_handler()->register_udp_callback(this);
  
  //fprintf(stderr, "StartTime: %u\n", timer::ms_since_epoch());


  //packet_callback_id = controller->get_l3_handler()->
  //  register_udp_callback(std::bind(&discovery_service::discovery_packet_handler, this, std::placeholders::_1, std::placeholders::_2));
  port_callback_id = switch_state_service->
    register_port_mod_callback(std::bind(&discovery_service::discovery_port_handler, this, std::placeholders::_1));
  discovery_tid = std::thread(&discovery_service::discovery_daemon_entry, this);
  //broadcast_my_data(topology->get_data());
}

// shuts down the discovery service 
// marks as not initialized, unregisters callbacks, shuts down the topology graph, and waits for the daemon thread to end 
void discovery_service::shutdown() {
  if (!initialized) return;
  initialized = false;
  switch_state_service->unregister_port_mod_callback(port_callback_id);
  //controller->get_l3_handler()->unregister_udp_callback(packet_callback_id);
  controller->get_l3_handler()->unregister_udp_callback(this);
  topology.shutdown();
  discovery_tid.join();
  //fprintf(stderr, "StopTime: %u\n", timer::ms_since_epoch());
}

// stub - to fix later
std::vector<uint16_t> discovery_service::get_flood_ports() {
  std::vector<uint16_t> v;
  return v;
}

std::string discovery_service::get_service_info() const {
  return std::string();
}

std::string discovery_service::get_running_info() const {
  return std::string();
}

std::vector<mac_address> discovery_service::get_shortest_path(mac_address source, mac_address dest) {
  return topology.get_shortest_path(source, dest);
}

std::vector<std::vector<mac_address>> discovery_service::get_disjoint_paths(mac_address source, mac_address dest) {
  return topology.get_disjoint_paths(source, dest);
}

uint32_t discovery_service::serialize_local_topology_info(autobuf& dest) {
  return topology.serialize_local_node(dest);
}

bool discovery_service::get_neighbor_port(mac_address neighbor, uint16_t* port_num) {
  return topology.get_neighbor_port(neighbor, port_num);
}


///////////////////////////////////////
// HANDLERS & DAEMON ENTRY METHODS
///////////////////////////////////////

// handler for discovery hello/helloACK packets
// packets are distinguished by having non-forwardable
// (nil) destination MAC and destination IP addresses
bool discovery_service::udp_callback(const udp_packet& pkt, const autobuf& original) {

  // check if the packet is a discovery packet
  if (pkt.dest_mac.is_nil() && pkt.dest_ip.is_nil() && 
      (pkt.dest_port == NEIGHBOR_UPDATE_PORT || 
       pkt.dest_port == TOPOLOGY_UPDATE_PORT)) {

    // deserialize the packet
    discovery_packet disc_pkt;
    if (!disc_pkt.deserialize(pkt.udp_payload)) return false;
    //fprintf(stderr, "Discovery Packet Arrived from %s\n", disc_pkt.src_mac.to_string().c_str());

    if (pkt.dest_port == NEIGHBOR_UPDATE_PORT) {
      if (first_time_seen(disc_pkt)) {
	fwd_broadcast_change_udp_port(pkt,TOPOLOGY_UPDATE_PORT); 
      }
      topology.update_neighbor(pkt.in_port, disc_pkt.src_mac, disc_pkt.src_ip, &(disc_pkt.data));
    }

    else if  (pkt.dest_port == TOPOLOGY_UPDATE_PORT) {
      if (first_time_seen(disc_pkt)) {
	fwd_broadcast_same_udp_port(pkt);
	topology.update_topology(disc_pkt.src_mac, disc_pkt.src_ip, &(disc_pkt.data));
      }
    }

    return true;

    /*
    // if the packet has not been seen before and did not originate from this switch
    // broadcast it and update the network graph 
    if (first_time_seen(disc_pkt)) {

      // handle NEIGHBOR_UPDATE packets
      //if (pkt.dest_port == NEIGHBOR_UPDATE_PORT) {
      //	fwd_broadcast_change_udp_port(pkt,TOPOLOGY_UPDATE_PORT); 
      //	topology.update_neighbor(pkt.in_port, disc_pkt.src_mac, disc_pkt.src_ip, &(disc_pkt.data));
      //}

      if (pkt.dest_port == NEIGHBOR_UPDATE_PORT) {
	fwd_broadcast_change_udp_port(pkt,TOPOLOGY_UPDATE_PORT); 
      }
            
      // handle TOPOLOGY_UPDATE packets
      else if (pkt.dest_port == TOPOLOGY_UPDATE_PORT) {
	fwd_broadcast_same_udp_port(pkt);
	topology.update_topology(disc_pkt.src_mac, disc_pkt.src_ip, &(disc_pkt.data));
      }
    }
    return true;
    */
  }
  return false;
}

// handler for port state changes on local switch
void discovery_service::discovery_port_handler(uint16_t phy_port) {
  
  // ignore reserved ports, get the modified port, and update the topology graph
  if (phy_port > 0xff00)  return;
  openflow_port modified_port;
  if (!switch_state_service->get_switch_port(phy_port, &modified_port)) return;         
  topology.update_local_port(modified_port);
}

// discovery service daemon entrypoint
void discovery_service::discovery_daemon_entry() {
  while(initialized) {
    timer::sleep_for(CLEAN_BROADCASTS_SEEN_INTERVAL);
    clean_broadcasts_seen();
  }
}
    
///////////////////////////////////////////
// DISCOVERY PACKET BROADCAST METHODS 
///////////////////////////////////////////

//void discovery_service::send_discovery_packet(uint16_t udp_port, autobuf* data) {
void discovery_service::broadcast_data(autobuf* data) {
  
  // create the discovery packet
  discovery_packet disc_pkt;
  uint32_t time = timer::ms_since_epoch();
  disc_pkt.src_mac = switch_state_service->get_switch_mac();
  disc_pkt.src_ip = switch_state_service->get_switch_ip();
  disc_pkt.timestamp = time;
  if (time != last_timestamp) {
    next_seq_num = 0;
    last_timestamp = time;
  }
  disc_pkt.seq_num = next_seq_num++;
  disc_pkt.data = *data;
  
  // create udp packet carrier
  udp_packet udp_pkt;
  udp_pkt.src_port = NEIGHBOR_UPDATE_PORT;
  udp_pkt.dest_port = NEIGHBOR_UPDATE_PORT;
  udp_pkt.src_ip = "0.0.0.0";
  udp_pkt.dest_ip = "0.0.0.0";
  udp_pkt.src_mac.set_nil();
  udp_pkt.dest_mac.set_nil();
  disc_pkt.serialize(udp_pkt.udp_payload);

  // broadcast the packet
  std::vector<openflow_port> ports = switch_state_service->get_switch_ports();
  for (auto& port : ports) {
    if (port.port_number > 0xff00) continue; // ignore reserved ports
    autobuf* pkt = allocator->allocate();
    udp_pkt.serialize(*pkt);
    controller->send_packet(*pkt, port.port_number);
    allocator->free(pkt);
    //fprintf(stderr, "my_data_sent: %u\n", timer::ms_since_epoch());
  }
}

void discovery_service::fwd_broadcast_change_udp_port(const udp_packet& pkt, uint16_t port) {
  
  // create a new packet with the same payload but different port
  udp_packet new_pkt;
  new_pkt.src_port = port;
  new_pkt.dest_port = port;
  new_pkt.in_port = pkt.in_port;
  new_pkt.src_ip = "0.0.0.0";
  new_pkt.dest_ip = "0.0.0.0";
  new_pkt.src_mac.set_nil();
  new_pkt.dest_mac.set_nil();
  new_pkt.udp_payload = pkt.udp_payload;

  fwd_broadcast_same_udp_port(new_pkt);
}

void discovery_service::fwd_broadcast_same_udp_port(const udp_packet& pkt) {
  std::vector<openflow_port> ports = switch_state_service->get_switch_ports();
  for (auto& port : ports) {
    if (port.port_number != pkt.in_port && port.port_number <= 0xff00) {
      autobuf* pkt_cpy = allocator->allocate();
      pkt.serialize(*pkt_cpy);
      controller->send_packet(*pkt_cpy, port.port_number);
      allocator->free(pkt_cpy);
      //fprintf(stderr, "fwd_broadcast_sent: %u\n", timer::ms_since_epoch());
    }
  }
}

//////////////////////////////////////////////
// CHECK WHETHER PACKET HAS BEEN SEEN BEFORE
//////////////////////////////////////////////

bool discovery_service::first_time_seen(const discovery_packet& disc_pkt) {
  // packets sent from the local switch have always been seen before
  if (disc_pkt.src_mac == switch_state_service->get_switch_mac()) return false;

  std::lock_guard<std::mutex> g(broadcasts_seen_lk);
  auto search = broadcasts_seen.find(disc_pkt.src_mac);
  if (search != broadcasts_seen.end()) {

    // if the timestamp is more recent than previous updates from the same MAC address, the update is new
    if (disc_pkt.timestamp > std::get<0>(search->second)) {
      broadcasts_seen[disc_pkt.src_mac] = std::make_tuple(disc_pkt.timestamp, disc_pkt.seq_num);
      return true;
    }

    // if the timestamp is equal to previous updates but the sequence number is greater, the update is new
    else if ((disc_pkt.timestamp == std::get<0>(search->second)) &&
	     (disc_pkt.seq_num > std::get<1>(search->second))) {
      broadcasts_seen[disc_pkt.src_mac] = std::make_tuple(disc_pkt.timestamp, disc_pkt.seq_num);
      return true;
    }
  }

  // if no updates have been seen from the MAC address, the update is new
  else {
    broadcasts_seen[disc_pkt.src_mac] = std::make_tuple(disc_pkt.timestamp, disc_pkt.seq_num);
    return true;
  }

  // otherwise the packet is old
  return false;
}

// remove stale entries in the broadcasts_seen map
void discovery_service::clean_broadcasts_seen() {
  std::lock_guard<std::mutex> g(broadcasts_seen_lk);
  uint32_t time = timer::ms_since_epoch();
  auto i = broadcasts_seen.begin();
  while(i != broadcasts_seen.end()) {
    if (time > std::get<0>(i->second) && time - std::get<0>(i->second) > STALE_BROADCASTS_SEEN_INTERVAL) {
      broadcasts_seen.erase(i++);
    }
    else { 
      ++i; 
    }    
  }
}

////////////////////////////////////////////
// DISCOVERY PACKET METHODS
///////////////////////////////////////////

// constructor
discovery_service::discovery_packet::discovery_packet() {
  clear();
}  
  
// clears packet contents
void discovery_service::discovery_packet::clear() {
  src_mac.clear();
  src_ip.clear();
  name.clear();
  timestamp = 0;
  seq_num = 0;
  data.clear();
}

// serialize a discovery packet
uint32_t discovery_service::discovery_packet::serialize(autobuf& dest) const {
  
  // copy header fields into dest
  uint32_t pkt_size = sizeof(discovery_packet_hdr) + data.size();
  dest.create_empty_buffer(pkt_size, false);
  discovery_packet_hdr* hdr = (discovery_packet_hdr*)dest.get_content_ptr();
  src_mac.get(hdr->src_mac);
  src_ip.get(hdr->src_ip);
  pack_uint32(hdr->timestamp, timestamp);
  hdr->seq_num = seq_num;
  memset(hdr->name, 0, 40);
  strncpy((char*)(hdr->name), name.c_str(), 39);
  memcpy(hdr->data, data.get_content_ptr(), data.size());
  
  return pkt_size;
}

// deserialize a discovery packet
bool discovery_service::discovery_packet::deserialize(const autobuf& input) {
  clear();
  const discovery_packet_hdr* hdr = (const discovery_packet_hdr*) input.get_content_ptr();
  
  if (input.size() < sizeof(discovery_packet_hdr)) {
    //printf("Unable to deserialize discovery packet: input size (%u) < header size (%u)\n",
    //	   input.size(), sizeof(discovery_packet_hdr));
    return false;
  } 
  src_mac.set_from_network_buffer(hdr->src_mac);
  src_ip.set_from_network_buffer(hdr->src_ip);
  timestamp = unpack_uint32(hdr->timestamp);
  seq_num = hdr->seq_num;
  name = std::string((const char*)hdr->name);
  
  if (input.size() > sizeof(discovery_packet_hdr)) {
    uint32_t bytes_to_copy = input.size() - sizeof(discovery_packet_hdr);
    data.create_empty_buffer(bytes_to_copy);
    data.memcpy_from((const void*)hdr->data, bytes_to_copy); 
  }
  return true;
}

// convert discovery packet to string for debugging
std::string discovery_service::discovery_packet::to_string() const {
  std::string result = "Discovery Packet:";
  result += "\n\t src mac:   " + src_mac.to_string();
  result += "\n\t src ip:    " + src_ip.to_string();
  result += "\n\t timestamp: " + std::to_string(timestamp);
  result += "\n\t seq num:   " + std::to_string(seq_num);
  result += "\n\t name:      " + name;
  return result;
}
	  
