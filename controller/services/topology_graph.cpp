#include "topology_graph.h"

////////////////////////////////////////
// INITIALIZATION / SHUTDOWN METHODS
///////////////////////////////////////

// constructor
topology_graph::topology_graph() {
  initialized = false;
  last_update_sent_time = 0;
}

// clears the topology graph without stopping daemon
// equivalent to wiping topology memory of the controller
void topology_graph::clear() {
  //auto guard = lock_all_other_nodes();
  std::lock_guard<std::mutex> g(other_nodes_lk);
  std::lock_guard<std::mutex> me_guard(me_lk);
  me.clear();
  other_nodes.clear();
  //other_nodes_lks.clear();
}

// initialaizes the topology graph and starts the daemon
void topology_graph::init(mac_address my_mac_addr, ip_address my_ip_addr, std::string my_name, 
			  std::vector<openflow_port> my_ports,
			  std::function<void(autobuf*)> broadcast_data_fn,
			  preallocated<autobuf>* allocator_ptr) {
  if (initialized) return;
  //switch_state_service = switch_state_service_ptr;
  //tbarrier_id = timed_barrier_id;
  allocator = allocator_ptr;
  broadcast_data = broadcast_data_fn;
  
  // DO ALL SETUP OF ME NODE
  me.mac_addr = my_mac_addr; //switch_state_service->get_switch_mac();
  me.ip_addr = my_ip_addr; //switch_state_service->get_switch_ip();
  me.name = my_name; //switch_state_service->get_name();
  //vector<openflow_port> my_ports = my_ports; //switch_state_service->get_switch_ports();
  for (auto& p : my_ports) {
    me.ports[p.port_number].update(p);
  }

  initialized = true;
  topology_daemon_tid = std::thread(&topology_graph::topology_daemon_entry, this);
}

// shuts down the topology graph daemon,
void topology_graph::shutdown() {
  if (!initialized) return;
  initialized = false;
  topology_daemon_tid.join();
}

/////////////////////////////////////////////
// METHODS TO UPDATE THE TOPOLOGY GRAPH
/////////////////////////////////////////////

// update the status of a local port
void topology_graph::update_local_port(openflow_port& modified_port) {
  std::lock_guard<std::mutex> g(me_lk);
  bool changed = me.ports[modified_port.port_number].update(modified_port);
  //port_state& port_to_update = me.ports[modified_port.port_number];
  //bool changed = port_to_update.update(modified_port);
  if (changed) {
    //fprintf(stderr, "UPDATE PORT - UPDATE\n");
    send_update(); //timed_barrier::notify(tbarrier_id); }
  }
}

// update the status of a neighbor connection
void topology_graph::update_neighbor(uint16_t phy_port, mac_address neighbor_mac,
				     ip_address neighbor_ip, autobuf* data) {
  std::lock_guard<std::mutex> g(me_lk);
  bool update_needed = false;
  uint32_t local_time = timer::ms_since_epoch();
  //fprintf(stderr, "NEIGHBOR_PHY_POR: %d\n\n", phy_port);
  if (me.links.find(phy_port) == me.links.end() ||
      std::get<0>(me.links[phy_port]) != neighbor_mac) {
    me.links[phy_port] = std::make_tuple(neighbor_mac, local_time);
    update_needed = true;
  }
  else {
    std::get<1>(me.links[phy_port]) = local_time;
  }

  //if (me.links.find(neighbor_mac) == me.links.end() || 
  //    std::get<0>(me.links[neighbor_mac]) != phy_port) {
  // me.links[neighbor_mac] = std::make_tuple(phy_port,local_time);
  // update_needed = true;
  //}
  //else {
  // std::get<1>(me.links[neighbor_mac]) = local_time;
  //}
  update_topology(neighbor_mac, neighbor_ip, data);
  if (update_needed) {
    //fprintf(stderr, "UPDATE NEIGHBOR - UPDATE\n");
    send_update(); //timed_barrier::notify(tbarrier_id); }
  }
}

// update the status of a distant (non-neighbor) Ironstack node
void topology_graph::update_topology(mac_address mac_addr, ip_address ip_addr, 
				     autobuf* data) {
  std::lock_guard<std::mutex> g(other_nodes_lk);  //s[mac_addr]);
  network_node update_node;
  uint32_t local_time = timer::ms_since_epoch();
  update_node.deserialize(*data);
  if (other_nodes.find(mac_addr) == other_nodes.end() || 
      other_nodes[mac_addr].last_refresh < update_node.last_refresh ||
      (other_nodes[mac_addr].last_refresh == update_node.last_refresh &&
       other_nodes[mac_addr].seq_num < update_node.seq_num)) {
    if (other_nodes.find(mac_addr) == other_nodes.end()) {
      //fprintf(stderr, "Not Stabilized\n");
    }
    other_nodes[mac_addr].mac_addr = update_node.mac_addr;
    other_nodes[mac_addr].last_refresh = update_node.last_refresh;
    other_nodes[mac_addr].seq_num = update_node.seq_num;
    other_nodes[mac_addr].ip_addr = update_node.ip_addr;
    other_nodes[mac_addr].ports.swap(update_node.ports);
    if (other_nodes[mac_addr].links != update_node.links) {
      //fprintf(stderr, "Not Stabilized\n");
    }
    //for (auto entry = update_node.links.begin(); entry != update_node.links.end(); ++entry) {
    // if (other_nodes[mac_addr].links.find(entry->first) == other_nodes[mac_addr].links.end()) {
    //	fprintf(stderr, "Not Stabilized\n");
    //	break;
    //}
    //}
    other_nodes[mac_addr].links.swap(update_node.links);
    other_nodes[mac_addr].local_refresh = local_time;
    //fprintf(stderr, "Other Node Update - nothing sent\n");
  }
}


//////////////////////////////////////////////////////
//  TOPOLOGY GRAPH DAEMON
/////////////////////////////////////////////////////

void topology_graph::topology_daemon_entry() {
  
  // send initial update
  send_update();
  
  while(initialized) {
    //uint32_t sleep_time_ms = HEARTBEAT_INTERVAL - (timer::ms_since_epoch() - last_update_sent_time);
    //fprintf(stderr, "SLEEPTIME: %f", ((double)(sleep_time_ms)/(double)1000));
    //timer::sleep_for(((double)(sleep_time_ms)/(double)1000));
    timer::sleep_for(HEARTBEAT_INTERVAL);
    //std::vector<mac_address> stale;
    std::vector<uint16_t> stale_links;
    std::vector<mac_address> stale_nodes;
    //auto guard = lock_all_other_nodes();
    std::unique_lock<std::mutex> g1(other_nodes_lk, std::defer_lock);
    std::unique_lock<std::mutex> g2(me_lk, std::defer_lock);
    

    g2.lock();
    uint32_t time = timer::ms_since_epoch();
    bool update_needed = false;
    for (auto entry = me.links.begin(); entry != me.links.end(); ++entry) {
      if (std::get<1>(entry->second) <= time &&
	  time - std::get<1>(entry->second) > STALE_TIMEOUT) {
	stale_links.push_back(entry->first);
	update_needed = true;
      }
    }
    for (auto stale_entry = stale_links.begin(); stale_entry != stale_links.end(); ++stale_entry) {
      me.links.erase(*stale_entry);
    }
    if (update_needed) {
      //fprintf(stderr, "STALE LINK - UPDATE\n");
      send_update(); //timed_barrier::notify(tbarrier_t); }
    }
    else { //if (time > last_update_sent_time && time - last_update_sent_time >= HEARTBEAT_INTERVAL) {
      //fprintf(stderr, "HEARTBEAT - NO UPDATE\n");
      send_update();
    }
    g2.unlock();
    stale_links.clear();

    g1.lock();
    for (auto entry = other_nodes.begin(); entry != other_nodes.end(); ++entry) {
      if ((entry->second).local_refresh <= time && 
	  time - (entry->second).local_refresh > STALE_TIMEOUT) {
	stale_nodes.push_back(entry->first);
      }
    }
    for (auto stale_entry = stale_nodes.begin(); stale_entry != stale_nodes.end(); ++stale_entry){
      other_nodes.erase(*stale_entry);
      //other_nodes_lks.erase(*stale_entry);
    }
    g1.unlock();    
    stale_nodes.clear();
    //fprintf(stderr, "TOPOLOGY:\n%s",to_string().c_str());
  }
}

///////////////////////////////////////////////////////
// GRAPH INFO OUTPUT METHODS
///////////////////////////////////////////////////////
/*
uint32_t topology_graph::get_data(autobuf& dest) {
  //update last_refresh and seq_num
  std::lock_guard<std::mutex> g(me_lk);
  uint32_t time = timer::ms_since_epoch();
  if (time == me.last_refresh) { ++(me.seq_num); }
  else { me.last_refresh = time; me.seq_num = 0; }
  return me.serialize(dest);
}
*/

std::string topology_graph::to_string()  {
  std::lock_guard<std::mutex> g_me(me_lk);
  std::lock_guard<std::mutex> g_other(other_nodes_lk);
  
  std::string result = "My Information(Me Node):\n";
  result += me.to_string();
  result += "\n\n";
  result += "Other Nodes:";
  for (auto other = other_nodes.begin(); other != other_nodes.end(); ++other) {
    result += "\n MAC Address:  " + (other->first).to_string();
    result += "\n" + (other->second).to_string() + "\n";
  }
  result += "\n";
  return result;
}

bool topology_graph::get_neighbor_port(mac_address neighbor, uint16_t* port_num) {
  std::lock_guard<std::mutex> g(me_lk);
  for (auto link = me.links.begin(); link != me.links.end(); ++link) {
    if (std::get<0>(link->second) == neighbor) {
      *port_num = link->first;
      return true;
    }
  }
  return false;
}

uint32_t topology_graph::serialize_local_node(autobuf& dest) {
  std::lock_guard<std::mutex> g(me_lk);
  return me.serialize(dest);
}

////////////////////////////////////////////////////////
// TOPOLOGY GRAPH HELPER FUNCTIONS
////////////////////////////////////////////////////////

/*
std::vector<std::unique_lock<std::mutex>> lock_all_other_nodes() {
  std::vector<std::unique_lock<std::mutex>> lock_vector;
  for (auto i = other_nodes_lks.begin(); i != other_nodes_lks.end(); ++i) {
    lock_vector.push_back(std::unique_lock<std::mutex>(i->second));
  }
  return lock_vector;
}
*/

// ONLY TO BE CALLED WHEN HOLDING me_lk
void topology_graph::send_update() {
  autobuf *dest = allocator->allocate();
  uint32_t time = timer::ms_since_epoch();
  last_update_sent_time = time;
  if (time > me.last_refresh) {
    me.last_refresh = time;
    me.seq_num = 0;
  }
  else ++me.seq_num;
  me.serialize(*dest);
  last_update_sent_time = timer::ms_since_epoch();
  broadcast_data(dest);
  allocator->free(dest);
}

////////////////////////////////////////////////////////
// NETWORK NODE METHODS
///////////////////////////////////////////////////////

// constructor
topology_graph::network_node::network_node() {
  clear();
}

// clear 
void topology_graph::network_node::clear() {
  local_refresh = 0;
  last_refresh = 0;
  seq_num = 0;
  mac_addr.clear();
  ip_addr.clear();
  blacklisted = false;
  whitelisted = false;
  name = "";
  ports.clear();
  links.clear();
}

// serialize
uint32_t topology_graph::network_node::serialize(autobuf& dest) const {

  int next_item_index = sizeof(network_node_hdr);
  int port_increment = (sizeof(uint16_t) + sizeof(topology_graph::port_state::port_state_hdr));
  int link_increment = (6 + sizeof(uint16_t));
  uint8_t* ptr;

	uint32_t payload_size = sizeof(network_node_hdr) + ports.size()*port_increment + links.size()*link_increment;

	dest.create_empty_buffer(payload_size);

  network_node_hdr* hdr = (network_node_hdr*) dest.get_content_ptr();
  pack_uint32(hdr->last_refresh, last_refresh);
  hdr->seq_num = seq_num;
  mac_addr.get(hdr->mac_addr);
  ip_addr.get(hdr->ip_addr);
	memset(hdr->name, 0, 40);
  strncpy((char*) hdr->name, name.c_str(), 39);
  pack_uint32(hdr->num_ports, ports.size());
  pack_uint32(hdr->num_links, links.size());

  
  for (auto entry = ports.begin(); entry != ports.end(); ++entry) {
//		printf("packing ports offset %u\n", next_item_index-sizeof(network_node_hdr));
    ptr = (uint8_t*) dest.ptr_offset_mutable(next_item_index);
    pack_uint16(ptr, entry->first);

//		printf("port entry %hu\n", entry->first);

    ptr = (uint8_t*) dest.ptr_offset_mutable(next_item_index + sizeof(uint16_t));
    (entry->second).pack(ptr);
    next_item_index += port_increment;
  }

//	printf("after packing ports [%s]\n", dest.to_hex().c_str());

  //uint32_t last_value = next_item_index;

  for (auto entry = links.begin(); entry != links.end(); ++entry) {
//		printf("packing links offset %u\n", next_item_index-last_value);
    ptr = (uint8_t*) dest.ptr_offset_mutable(next_item_index);
    pack_uint16(ptr, entry->first);
    //(entry->first).get(ptr);
    ptr = (uint8_t*) dest.ptr_offset_mutable(next_item_index + 2);
    //pack_uint16(ptr, std::get<0>(entry->second));
    std::get<0>(entry->second).get(ptr);
    next_item_index += link_increment;
  }

//	printf("after packing links [%s]\n", dest.to_hex().c_str());
  return payload_size;

//  return (sizeof(network_node_hdr) + ports.size()*(port_increment) + links.size()*(link_increment));
}

// deserialize 
bool topology_graph::network_node::deserialize(const autobuf& input) {
  clear();
  const network_node_hdr* hdr = (const network_node_hdr*) input.get_content_ptr();
  
  if (input.size() < sizeof(network_node_hdr)) {
    // printf("Unable to deserialize network_node: input size (%u) < network node header size (%u)\n",
    //	   input.size(), sizeof(network_node_hdr));
    return false; 
  }
  last_refresh = unpack_uint32(hdr->last_refresh);
  seq_num = hdr->seq_num;
  mac_addr.set_from_network_buffer(hdr->mac_addr);
  ip_addr.set_from_network_buffer(hdr->ip_addr);
  name = std::string((const char*)hdr->name);

  uint32_t num_ports = unpack_uint32(hdr->num_ports);
  uint32_t num_links = unpack_uint32(hdr->num_links);
  int next_item_index = sizeof(network_node_hdr);
  int port_increment = (sizeof(uint16_t) + sizeof(topology_graph::port_state::port_state_hdr));
  int link_increment = (6 + sizeof(uint16_t));
  
  if (input.size() < sizeof(network_node_hdr) + num_ports*(port_increment) + num_links*(link_increment)) {
    //    printf("Unable to deserialize network_node: input size (%u) < size indicated in header (%u)\n",
    //	   input.size(), sizeof(network_node_hdr) + num_ports*(port_increment) + num_links*(link_increment));
    return false;
  }

  uint16_t next_port_num;
  for (uint32_t i = 0; i < num_ports; i++) {
    next_port_num = unpack_uint16(input.ptr_offset_const(next_item_index));
    ports[next_port_num].unpack((const uint8_t*)input.ptr_offset_const(next_item_index + sizeof(uint16_t)));
    next_item_index += port_increment;
  }

  for (uint32_t i = 0; i < num_links; i++) {
    //mac_address m_addr;
    //m_addr.set_from_network_buffer(input.ptr_offset_const(next_item_index));
    //uint16_t p_num = unpack_uint16(input.ptr_offset_const(next_item_index + 6));
    uint16_t p_num = unpack_uint16(input.ptr_offset_const(next_item_index));
    mac_address m_addr;
    m_addr.set_from_network_buffer(input.ptr_offset_const(next_item_index + 2));
    links[p_num] = std::make_tuple(m_addr,0);
    //links[m_addr] = std::make_tuple(p_num,0);
    next_item_index += link_increment;
  }

  return true;
}

// to_string
std::string topology_graph::network_node::to_string() const {
  std::string result = "Network Node:";
  result += "\n\t MAC Address:   " + mac_addr.to_string();
  result += "\n\t IP Address:    " + ip_addr.to_string();
  result += "\n\t Name:          " + name;
  result += "\n\t Local Refresh: " + std::to_string(local_refresh);
  result += "\n\t Last Refresh:  " + std::to_string(last_refresh);
  result += "\n\t Seq Num:       " + std::to_string(seq_num);
  result += "\n\t Blacklisted:   " + std::to_string(blacklisted);
  result += "\n\t Whitelisted:   " + std::to_string(whitelisted);
  result += "\n\t Ports: ";
  for (auto p = ports.begin(); p != ports.end(); ++p) {
    result += "\n\t\t Port Number " + std::to_string(p->first) + ": " + (p->second).to_string();
  }
  result += "\n\t Links: ";
  for (auto link = links.begin(); link != links.end(); ++link) {
    result += "\n\t\t Physical Port: " + std::to_string(link->first) 
      + " Neighbor MAC: " + std::get<0>(link->second).to_string() 
      + " Local Refresh: " + std::to_string(std::get<1>(link->second));
    //result += "\n\t\t Neighbor MAC: " + (link->first).to_string() + " Physical Port: " 
    //  + std::to_string(std::get<0>(link->second)) + " Local Refresh: " 
    //  + std::to_string(std::get<1>(link->second));
  }
  return result;
 }

///////////////////////////////////////////////////////
// PORT STATE METHODS
///////////////////////////////////////////////////////

topology_graph::port_state::port_state() {
  port_num = -1;
  online = false;
  is_stp_port = false;
  is_flood_port = false;
  name = "";
}

bool topology_graph::port_state::update(openflow_port& p) {
  if (port_num != p.port_number || online != p.is_up() || name != p.name) {
    port_num = p.port_number;
    online = p.is_up();
    name = p.name;
    return true;
  }
  return false;
}

void topology_graph::port_state::pack(uint8_t* dest) const {
  port_state_hdr* hdr = (port_state_hdr*) dest;
  pack_uint16(hdr->port_num, port_num);
//      printf("port number %hu\n", port_num);
  hdr->online = (uint8_t) online;
//      printf("online: %s\n", hdr->online ? "yes" : "no");
  hdr->is_stp_port = (uint8_t) is_stp_port;
//      printf("stp: %s\n", hdr->is_stp_port ? "yes" : "no");
  hdr->is_flood_port = (uint8_t) is_flood_port;
//      printf("flood: %s\n", hdr->is_flood_port ? "yes" : "no");
  memset(hdr->name, 0, 40);
  strncpy((char*) hdr->name, name.c_str(), 39);
}

void topology_graph::port_state::unpack(const uint8_t* input) {
  const port_state_hdr* hdr = (const port_state_hdr*) input;
  port_num = unpack_uint16(hdr->port_num);
  online = (bool) hdr->online;
  is_stp_port = (bool) hdr->is_stp_port;
  is_flood_port = (bool) hdr->is_flood_port;
  name = std::string((const char*)hdr->name);
}

std::string topology_graph::port_state::to_string() const {
  
  std::string result = "\n\t\t\t Port State:";
  result += "\n\t\t\t\t Port Number:   " + std::to_string(port_num);
  result += "\n\t\t\t\t Name:          " + name;
  result += "\n\t\t\t\t Online:        " + std::to_string(online);
  result += "\n\t\t\t\t Is STP Port:   " + std::to_string(is_stp_port);
  result += "\n\t\t\t\t Is Flood Port: " + std::to_string(is_flood_port);
  return result;
}

/*
// serialize
void port_state::serialize(autobuf& dest) {

}

// deserialize
uint32_t port_state::deserialize(const autobuf& source) {

}
*/


//////////////////////////////////////////////////////////////////////
// GRAPH PROCESSING FUNCTIONS
/////////////////////////////////////////////////////////////////////

// shortest path discovery
std::vector<mac_address> topology_graph::get_shortest_path(mac_address source, mac_address dest) {
  simple_graph g = to_simple_graph();
  return g.bfs(source, dest, false);
}


// disjoint path discovery
std::vector<std::vector<mac_address>> topology_graph::get_disjoint_paths(mac_address source, mac_address dest) {
  std::vector<std::vector<mac_address>> disjoint_paths;
  simple_graph g = to_simple_graph();

  // if source or dest aren't in the graph, there are no paths, so return
  // and empty path vector
  if (g.edges.count(source) == 0 || g.edges.count(dest) == 0) {
    return disjoint_paths;
  }

  // run the maxflow algorithm on the graph and 
  // get the number of disjoint paths
  uint32_t num_disjoint_paths = g.max_flow(source, dest);

  // find each of the disjoint paths
  for (uint32_t i = 0; i < num_disjoint_paths; i++) {
    std::vector<mac_address> path;
    std::set<mac_address> path_contents;
    mac_address current = source;
    path.push_back(current);
    path_contents.insert(current);

    // trace edges with flow from source to dest
    while (current != dest) {
      std::set<mac_address> neighbors = g.edges[current];

      // for each edge coming out of the current node, check whether it
      // has flow, take the first one that does, and set it's flow to 0
      for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor) {
	if (g.get_flow(current, *neighbor)) {
	  g.false_flow(current, *neighbor);
	  current = *neighbor;
	  path.push_back(current);
	  break;
	}
      }

      // remove cycles
      if (path_contents.count(current) > 0) {
	path.pop_back();
	while (path.back() != current) {
	  path_contents.erase(path.back());
	  path.pop_back();
	}
      }
      path_contents.insert(current);  
    }
    
    // add the path to the list of disjoint paths
    disjoint_paths.push_back(path);
  }
  return disjoint_paths;
}

// Edmonds-Karp algorithm for finding the max 0-1 flow
uint32_t topology_graph::simple_graph::max_flow(mac_address source, mac_address sink) {
  
  // set all flows to 0
  false_all_flows();

  // while there is a path from the source to the sink with 
  // remaining flow capacity on each edge, send flow along the 
  // path 
  std::vector<mac_address> path = bfs(source, sink, true);
  while(path.size() != 0) {
    for (uint32_t i = 0; i < path.size() - 1; i++) {
      true_flow(path[i], path[i+1]);
    }
    path = bfs(source, sink, true);
  }

  // false any flows of edges going both ways in graph
  for (auto start = edges.begin(); start != edges.end(); ++start) {
    for (auto end = start->second.begin(); end != start->second.end(); ++end) {
      if (get_flow(start->first, *end) && get_flow(*end, start->first)) {
	false_flow(start->first, *end);
	false_flow(*end, start->first);
      }
    }
  }

  // calculate max flow value
  uint32_t mflow = 0;
  std::set<mac_address> second_nodes = edges[source];
  for (auto node = second_nodes.begin(); node != second_nodes.end(); ++node) {
    if (get_flow(source, *node)) {
      ++mflow;
    }
  }
  return mflow;
}

std::vector<mac_address> topology_graph::simple_graph::bfs(mac_address source, mac_address dest, bool use_flows) {
  std::set<mac_address> visited;
  std::queue<mac_address> frontier;
  std::map<mac_address, mac_address> parent;
  std::vector<mac_address> path;

  frontier.push(source);
  while (!frontier.empty()) {
    mac_address next = frontier.front();
    frontier.pop();
    
    // target found
    if (next == dest) {
      mac_address p = next;
      path.push_back(p);
      while (parent.count(p) != 0) {
	path.push_back(parent[p]);
	p = parent[p];
      }
      for (uint32_t i = 0; i < path.size()/2; i++) {
       	mac_address temp = path[i];
	path[i] = path[path.size() - 1 - i];
       	path[path.size() - 1 - i] = temp;
      }
      return path;
    }
    
    // update parent map for backtracking
    // and add neighbors to frontier
    std::set<mac_address> neighbors = edges[next];
    for (auto neighbor = neighbors.begin(); neighbor != neighbors.end(); ++neighbor) {
      if (visited.count(*neighbor) == 0) {
	if (!get_flow(next, *neighbor) || !use_flows) {
	  parent[*neighbor] = next;
	  frontier.push(*neighbor);
	}
      }
    }
    
    // add next to visited
    visited.insert(next); 
  }
  
  return path;
}

topology_graph::simple_graph topology_graph::to_simple_graph() {
  simple_graph g;
  std::unique_lock<std::mutex> g1(me_lk, std::defer_lock);
  std::unique_lock<std::mutex> g2(other_nodes_lk, std::defer_lock);
  
  g1.lock();
  for (auto link = me.links.begin(); link != me.links.end(); ++link) {
    g.add_edge(me.mac_addr, std::get<0>(link->second));
  }
  g1.unlock();
  g2.lock();
  for (auto other_node = other_nodes.begin(); other_node != other_nodes.end(); ++other_node) {
    for (auto on_link = (other_node->second).links.begin(); on_link != (other_node->second).links.end(); ++on_link) {
      g.add_edge(other_node->first, std::get<0>(on_link->second));
    }
  }
  g2.unlock();
  return g;
}

void topology_graph::simple_graph::add_edge(mac_address source, mac_address dest) {
  edges[source].insert(dest);
}

void topology_graph::simple_graph::true_flow(mac_address source, mac_address dest) {
  flows[std::make_pair(source, dest)] = true;
}
  
void topology_graph::simple_graph::false_flow(mac_address source, mac_address dest) {
  flows[std::make_pair(source, dest)] = false;
}
  
bool topology_graph::simple_graph::get_flow(mac_address source, mac_address dest) {
    return flows[std::make_pair(source, dest)];
}

void topology_graph::simple_graph::false_all_flows() {
  for (auto src = edges.begin(); src != edges.end(); ++ src) {
    for (auto dst = src->second.begin(); dst != src->second.end(); ++dst) {
      flows[std::make_pair(src->first, *dst)] = false;
    }
  }
}
