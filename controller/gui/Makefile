CC = g++-4.8
CCOPTS = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -fno-exceptions -fno-rtti -std=c++11 -pg -rdynamic -fno-strict-aliasing -Wno-unused-result -D_GLIBCXX_USE_NANOSLEEP -Wno-deprecated-declarations -D__REDIRECT_PRINTF_TO_STDERR -D__STDC_FORMAT_MACROS
CCOPTS_EXCEPTIONS = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -std=c++11 -pg -rdynamic -fno-strict-aliasing -Wno-unused-result -D_GLIBCXX_USE_NANOSLEEP -Wno-deprecated-declarations -D__STDC_FORMAT_MACROS
CCOPTSFAST = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -fno-exceptions -std=c++11 -pg -Ofast -march=native -flto -rdynamic -fno-strict-aliasing -Wno-unused-result
LINKOPTS = -g -pthread -pg -std=c++11 -Ofast -march=native -flto -fno-exceptions -fno-rtti -rdynamic -Wno-unused-result

all: test

test: gui_component.o \
	gui_controller.o \
	gui_defs.o \
	input_menu.o \
	input_textbox.o \
	key_reader.o \
	progress_bar.o \
	stacktrace.o \
	output.o \
	test.o
	$(CC) $(LINKOPTS) -o $@ $^ -lncurses

compile_dir:
	mkdir -p bin

test.o: test.cpp
	$(CC) $(CCOPTS) -o $@ test.cpp

gui_component.o: gui_component.cpp gui_component.h
	$(CC) $(CCOPTS) -o $@ gui_component.cpp

gui_controller.o: gui_controller.cpp gui_controller.h
	$(CC) $(CCOPTS) -o $@ gui_controller.cpp

gui_defs.o: gui_defs.cpp gui_defs.h
	$(CC) $(CCOPTS) -o $@ gui_defs.cpp

input_menu.o: input_menu.cpp input_menu.h
	$(CC) $(CCOPTS) -o $@ input_menu.cpp

input_textbox.o: input_textbox.cpp input_textbox.h
	$(CC) $(CCOPTS) -o $@ input_textbox.cpp

key_reader.o: key_reader.cpp key_reader.h
	$(CC) $(CCOPTS) -o $@ key_reader.cpp

output.o: output.cpp output.h
	$(CC) $(CCOPTS) -o $@ output.cpp

progress_bar.o: progress_bar.cpp progress_bar.h
	$(CC) $(CCOPTS) -o $@ progress_bar.cpp

stacktrace.o: stacktrace.cpp stacktrace.h
	$(CC) $(CCOPTS) -o $@ stacktrace.cpp


bin/lookup.o: lookup.cpp
	$(CC) $(CCOPTS) -o $@ lookup.cpp

bin/arp_table.o: services/arp_table.cpp services/arp_table.h
	$(CC) $(CCOPTS) -o $@ services/arp_table.cpp

bin/aux_switch_info.o: services/aux_switch_info.cpp services/aux_switch_info.h
	$(CC) $(CCOPTS) -o $@ services/aux_switch_info.cpp

bin/cam_table.o: services/cam_table.cpp services/cam_table.h
	$(CC) $(CCOPTS) -o $@ services/cam_table.cpp

bin/dell_s48xx_acl_table.o: services/dell_s48xx_acl_table.cpp services/dell_s48xx_acl_table.h
	$(CC) $(CCOPTS) -o $@ services/dell_s48xx_acl_table.cpp

bin/dell_s48xx_l2_table.o: services/dell_s48xx_l2_table.cpp services/dell_s48xx_l2_table.h
	$(CC) $(CCOPTS) -o $@ services/dell_s48xx_l2_table.cpp

bin/flow_parser.o: utils/flow_parser.cpp utils/flow_parser.h
	$(CC) $(CCOPTS) -o $@ utils/flow_parser.cpp

bin/hal_transaction.o: hal/hal_transaction.cpp hal/hal_transaction.h
	$(CC) $(CCOPTS) -o $@ hal/hal_transaction.cpp

bin/openflow_flow_entry.o: ironstack_types/openflow_flow_entry.cpp ironstack_types/openflow_flow_entry.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_flow_entry.cpp

bin/openflow_vlan_port.o: ironstack_types/openflow_vlan_port.cpp ironstack_types/openflow_vlan_port.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_vlan_port.cpp

bin/packet_in_processor.o: hal/packet_in_processor.cpp hal/packet_in_processor.h
	$(CC) $(CCOPTS) -o $@ hal/packet_in_processor.cpp

../common/stacktrace.o: ../common/stacktrace.cpp ../common/stacktrace.h
	$(CC) $(CCOPTS) -o $@ ../common/stacktrace.cpp

bin/switch_db.o: services/switch_db.cpp services/switch_db.h
	$(CC) $(CCOPTS) -o $@ services/switch_db.cpp


# csv lookup utility
lookup: ../common/csv_parser.o \
	../common/ip_address.o \
	../common/mac_address.o \
	bin/aux_switch_info.o \
	bin/switch_db.o \
	bin/lookup.o
	$(CC) $(LINKOPTS) -o $@ $^

# openflow controller
ironstack2: ../common/autobuf.o \
	../common/tcp.o \
	../common/common_utils.o \
	../common/common_utils_oop.o \
	../common/csv_parser.o \
	../common/gui.o \
	../common/ip_port.o \
	../common/ip_address.o \
	../common/ipv6_address.o \
	../common/ironscale_packet.o \
	../common/mac_address.o \
	../common/stacktrace.o \
	../common/timed_barrier.o \
	../common/timer.o \
	bin/arp.o \
	bin/arp_table.o \
	bin/aux_switch_info.o \
	bin/cam.o \
	bin/cam_table.o \
	bin/dell_s48xx_acl_table.o \
	bin/dell_s48xx_l2_table.o \
	bin/flow_table.o \
	bin/flow_service.o \
	bin/hal.o \
	bin/hal_transaction.o \
	bin/ironstack_echo_daemon.o \
	bin/of_action.o \
	bin/of_actions_supported.o \
	bin/of_common_utils.o \
	bin/of_message.o \
	bin/of_message_barrier_reply.o \
	bin/of_message_barrier_request.o \
	bin/of_message_echo_reply.o \
	bin/of_message_echo_request.o \
	bin/of_message_error.o \
	bin/of_message_factory.o \
	bin/of_message_features_reply.o \
	bin/of_message_features_request.o \
	bin/of_message_flow_removed.o \
	bin/of_message_get_config_reply.o \
	bin/of_message_get_config_request.o \
	bin/of_message_hello.o \
	bin/of_message_modify_flow.o \
	bin/of_message_packet_in.o \
	bin/of_message_packet_out.o \
	bin/of_message_port_modification.o \
	bin/of_message_port_status.o \
	bin/of_message_queue_get_config_reply.o \
	bin/of_message_queue_get_config_request.o \
	bin/of_message_set_config.o \
	bin/of_message_stats_reply.o \
	bin/of_message_stats_request.o \
	bin/of_message_vendor.o \
	bin/of_match.o \
	bin/of_port_features.o \
	bin/of_port_state.o \
	bin/of_queue_config.o \
	bin/of_switch_capabilities.o \
	bin/of_types.o \
	bin/openflow_action_list.o \
	bin/openflow_aggregate_stats.o \
	bin/openflow_flow_description.o \
	bin/openflow_flow_description_and_stats.o \
	bin/openflow_flow_entry.o \
	bin/openflow_port.o \
	bin/openflow_port_config.o \
	bin/openflow_port_stats.o \
	bin/openflow_queue_stats.o \
	bin/openflow_switch_config.o \
	bin/openflow_switch_description.o \
	bin/openflow_switch_features.o \
	bin/openflow_vlan_port.o \
	bin/openflow_table_stats.o \
	bin/openflow_utils.o \
	bin/packet_in_processor.o \
	bin/service_catalog.o \
	bin/std_packet.o \
	bin/switch_db.o \
	bin/switch_state.o \
	bin/ironstack2.o
	$(CC) $(LINKOPTS) -o $@ $^ -lncurses

#switch diagnostics
switch_diagnostics: ../common/autobuf.o \
	../common/tcp.o \
	../common/common_utils.o \
	../common/common_utils_oop.o \
	../common/csv_parser.o \
	../common/gui.o \
	../common/ip_port.o \
	../common/ip_address.o \
	../common/ipv6_address.o \
	../common/ironscale_packet.o \
	../common/mac_address.o \
	../common/stacktrace.o \
	../common/timed_barrier.o \
	../common/timer.o \
	bin/arp.o \
	bin/arp_table.o \
	bin/aux_switch_info.o \
	bin/cam.o \
	bin/cam_table.o \
	bin/dell_s48xx_acl_table.o \
	bin/dell_s48xx_l2_table.o \
	bin/flow_parser.o \
	bin/flow_table.o \
	bin/flow_service.o \
	bin/hal.o \
	bin/hal_transaction.o \
	bin/ironstack_echo_daemon.o \
	bin/of_action.o \
	bin/of_actions_supported.o \
	bin/of_common_utils.o \
	bin/of_message.o \
	bin/of_message_barrier_reply.o \
	bin/of_message_barrier_request.o \
	bin/of_message_echo_reply.o \
	bin/of_message_echo_request.o \
	bin/of_message_error.o \
	bin/of_message_factory.o \
	bin/of_message_features_reply.o \
	bin/of_message_features_request.o \
	bin/of_message_flow_removed.o \
	bin/of_message_get_config_reply.o \
	bin/of_message_get_config_request.o \
	bin/of_message_hello.o \
	bin/of_message_modify_flow.o \
	bin/of_message_packet_in.o \
	bin/of_message_packet_out.o \
	bin/of_message_port_modification.o \
	bin/of_message_port_status.o \
	bin/of_message_queue_get_config_reply.o \
	bin/of_message_queue_get_config_request.o \
	bin/of_message_set_config.o \
	bin/of_message_stats_reply.o \
	bin/of_message_stats_request.o \
	bin/of_message_vendor.o \
	bin/of_match.o \
	bin/of_port_features.o \
	bin/of_port_state.o \
	bin/of_queue_config.o \
	bin/of_switch_capabilities.o \
	bin/of_types.o \
	bin/openflow_action_list.o \
	bin/openflow_aggregate_stats.o \
	bin/openflow_flow_description.o \
	bin/openflow_flow_description_and_stats.o \
	bin/openflow_flow_entry.o \
	bin/openflow_port.o \
	bin/openflow_port_config.o \
	bin/openflow_port_stats.o \
	bin/openflow_queue_stats.o \
	bin/openflow_switch_config.o \
	bin/openflow_switch_description.o \
	bin/openflow_switch_features.o \
	bin/openflow_vlan_port.o \
	bin/openflow_table_stats.o \
	bin/openflow_utils.o \
	bin/packet_in_processor.o \
	bin/service_catalog.o \
	bin/std_packet.o \
	bin/switch_db.o \
	bin/switch_state.o \
	bin/switch_diagnostics.o
	$(CC) $(LINKOPTS) -o $@ $^ -lncurses

port_chat: bin/port_chat.o \
	../common/common_utils.o \
	../common/common_utils_oop.o \
	../common/tcp.o \
	../common/ip_port.o \
	../common/ip_address.o \
	../common/ipv6_address.o \
	../common/autobuf.o \
	../common/timer.o
	$(CC) $(LINKOPTS) -o $@ $^


# top level object files come here
# please sort by alphabetical order

bin/anonymity_service.o: services/anonymity_service.cpp services/anonymity_service.h
	$(CC) $(CCOPTS) -o $@ services/anonymity_service.cpp

bin/arp.o: services/arp.cpp
	$(CC) $(CCOPTS) -o $@ services/arp.cpp

bin/cam.o: services/cam.cpp
	$(CC) $(CCOPTS) -o $@ services/cam.cpp

#bin/disjoint_paths.o: utils/disjoint_paths.cpp utils/disjoint_paths.h
#	$(CC) $(CCOPTS) -o $@ utils/disjoint_paths.cpp

bin/data_slicer.o: services/data_slicer.cpp services/data_slicer.h
	$(CC) $(CCOPTS_EXCEPTIONS) -o $@ services/data_slicer.cpp

bin/confidentiality_service.o: services/confidentiality_service.cpp services/confidentiality_service.h
	$(CC) $(CCOPTS_EXCEPTIONS) -o $@ services/confidentiality_service.cpp

bin/gui_interaction_service.o: services/gui_interaction_service.cpp services/gui_interaction_service.h
	$(CC) $(CCOPTS) -o $@ services/gui_interaction_service.cpp

bin/flow_id.o: ironstack_types/flow_id.cpp ironstack_types/flow_id.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/flow_id.cpp

bin/flow_service.o: services/flow_service.cpp services/flow_service.h
	$(CC) $(CCOPTS) -o $@ services/flow_service.cpp

bin/flow_table.o: services/flow_table.cpp services/flow_table.h
	$(CC) $(CCOPTS) -o $@ services/flow_table.cpp

bin/hal.o: hal/hal.h hal/hal.cpp
	$(CC) $(CCOPTS) -o $@ hal/hal.cpp

bin/ironscale_service.o: services/ironscale_service.cpp services/ironscale_service.h
	$(CC) $(CCOPTS) -o $@ services/ironscale_service.cpp

bin/ironstack.o: ironstack.cpp
	$(CC) $(CCOPTS) -o $@ ironstack.cpp

bin/ironstack2.o: ironstack2.cpp
	$(CC) $(CCOPTS) -o $@ ironstack2.cpp

bin/ironstack_discovery_daemon.o: services/ironstack_discovery_daemon.cpp services/ironstack_discovery_daemon.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_discovery_daemon.cpp

bin/ironstack_echo_daemon.o: services/ironstack_echo_daemon.cpp services/ironstack_echo_daemon.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_echo_daemon.cpp

bin/ironstack_flow_installer.o: services/ironstack_flow_installer.cpp services/ironstack_flow_installer.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_flow_installer.cpp

bin/ironstack_http_daemon.o: services/ironstack_http_daemon.cpp services/ironstack_http_daemon.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_http_daemon.cpp

bin/ironstack_tcp.o: services/ironstack_tcp.cpp services/ironstack_tcp.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_tcp.cpp

bin/ironstack_discovery_service.o: services/ironstack_discovery_service.h services/ironstack_discovery_service.cpp
	$(CC) $(CCOPTS) -o $@ services/ironstack_discovery_service.cpp

#bin/ironstack_topology_crawler.o: services/ironstack_topology_crawler.h services/ironstack_topology_crawler.cpp
#	$(CC) $(CCOPTS) -o $@ services/ironstack_topology_crawler.cpp

bin/json_generator.o: utils/json_generator.cpp utils/json_generator.h
	$(CC) $(CCOPTS) -o $@ utils/json_generator.cpp

bin/l3_handler.o: hal/l3_handler.cpp hal/l3_handler.h
	$(CC) $(CCOPTS) -o $@ hal/l3_handler.cpp

#bin/multipath_interface.o: services/multipath_interface.cpp services/multipath_interface.h
#	$(CC) $(CCOPTS) -o $@ services/multipath_interface.cpp

bin/topology_graph.o: services/topology_graph.h services/topology_graph.cpp 
	$(CC) $(CCOPTS) -o $@ services/topology_graph.cpp 

#bin/network_node.o: services/network_node.cpp services/network_node.h
#	$(CC) $(CCOPTS) -o $@ services/network_node.cpp

#bin/network_topology.o: services/network_topology.cpp services/network_topology.h
#	$(CC) $(CCOPTS) -o $@ services/network_topology.cpp

#bin/new_topology.o: services/new_topology.cpp services/new_topology.h
#	$(CC) $(CCOPTS) -o $@ services/new_topology.cpp

bin/path_id.o: ironstack_types/path_id.cpp ironstack_types/path_id.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/path_id.cpp

bin/port_chat.o: port_chat.cpp
	$(CC) $(CCOPTS) -o $@ port_chat.cpp

bin/service_catalog.o: hal/service_catalog.cpp hal/service_catalog.h
	$(CC) $(CCOPTS) -o $@ hal/service_catalog.cpp

#bin/topology.o: services/topology.cpp services/topology.h
#	$(CC) $(CCOPTS) -o $@ services/topology.cpp

# common utilities

../common/autobuf.o: ../common/autobuf.cpp ../common/autobuf.h
	$(CC) $(CCOPTS) -o $@ ../common/autobuf.cpp

../common/common_utils.o: ../common/common_utils.cpp ../common/common_utils.h
	$(CC) $(CCOPTS) -o $@ ../common/common_utils.cpp

../common/common_utils_oop.o: ../common/common_utils_oop.cpp ../common/common_utils_oop.h
	$(CC) $(CCOPTS) -o $@ ../common/common_utils_oop.cpp

../common/csv_parser.o: ../common/csv_parser.cpp ../common/csv_parser.h
	$(CC) $(CCOPTS) -o $@ ../common/csv_parser.cpp

../common/gui.o: ../common/gui.cpp ../common/gui.h
	$(CC) $(CCOPTS) -o $@ ../common/gui.cpp

../common/ip_address.o: ../common/ip_address.cpp ../common/ip_address.h
	$(CC) $(CCOPTS) -o $@ ../common/ip_address.cpp

../common/ipv6_address.o: ../common/ipv6_address.cpp ../common/ipv6_address.h
	$(CC) $(CCOPTS) -o $@ ../common/ipv6_address.cpp

../common/ip_port.o: ../common/ip_port.cpp ../common/ip_port.h
	$(CC) $(CCOPTS) -o $@ ../common/ip_port.cpp

../common/ironscale_packet.o: ../common/ironscale_packet.cpp ../common/ironscale_packet.h
	$(CC) $(CCOPTS) -o $@ ../common/ironscale_packet.cpp

../common/mac_address.o: ../common/mac_address.cpp ../common/mac_address.h
	$(CC) $(CCOPTS) -o $@ ../common/mac_address.cpp

../common/serializable.o: ../common/serializable.cpp ../common/serializable.h
	$(CC) $(CCOPTS) -o $@ ../common/serializable.cpp

bin/std_packet.o: ../common/std_packet.cpp ../common/std_packet.h
	$(CC) $(CCOPTS) -o $@ ../common/std_packet.cpp

../common/tcp.o: ../common/tcp.cpp ../common/tcp.h
	$(CC) $(CCOPTS) -o $@ ../common/tcp.cpp

../common/timed_barrier.o: ../common/timed_barrier.cpp ../common/timed_barrier.h
	$(CC) $(CCOPTS) -o $@ ../common/timed_barrier.cpp

../common/timer.o: ../common/timer.cpp ../common/timer.h
	$(CC) $(CCOPTS) -o $@ ../common/timer.cpp

../common/z_allocator.o: ../common/z_allocator.cpp ../common/z_allocator.h
	$(CC) $(CCOPTS) -o $@ ../common/z_allocator.cpp

# ironstack related objects

bin/of_action.o: openflow_types/of_action.cpp openflow_types/of_action.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_action.cpp

bin/of_actions_supported.o: openflow_types/of_actions_supported.cpp openflow_types/of_actions_supported.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_actions_supported.cpp

bin/of_common_utils.o: utils/of_common_utils.cpp utils/of_common_utils.h
	$(CC) $(CCOPTS) -o $@ utils/of_common_utils.cpp

bin/of_match.o: openflow_types/of_match.cpp openflow_types/of_match.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_match.cpp

bin/of_message.o: openflow_messages/of_message.cpp openflow_messages/of_message.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message.cpp

bin/of_message_barrier_reply.o: openflow_messages/of_message_barrier_reply.cpp openflow_messages/of_message_barrier_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_barrier_reply.cpp

bin/of_message_barrier_request.o: openflow_messages/of_message_barrier_request.cpp openflow_messages/of_message_barrier_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_barrier_request.cpp

bin/of_message_echo_request.o: openflow_messages/of_message_echo_request.cpp openflow_messages/of_message_echo_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_echo_request.cpp

bin/of_message_echo_reply.o: openflow_messages/of_message_echo_reply.cpp openflow_messages/of_message_echo_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_echo_reply.cpp

bin/of_message_error.o: openflow_messages/of_message_error.cpp openflow_messages/of_message_error.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_error.cpp

bin/of_message_flow_removed.o: openflow_messages/of_message_flow_removed.cpp openflow_messages/of_message_flow_removed.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_flow_removed.cpp

bin/of_message_get_config_request.o: openflow_messages/of_message_get_config_request.cpp openflow_messages/of_message_get_config_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_get_config_request.cpp

bin/of_message_get_config_reply.o: openflow_messages/of_message_get_config_reply.cpp openflow_messages/of_message_get_config_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_get_config_reply.cpp

bin/of_message_hello.o: openflow_messages/of_message_hello.cpp openflow_messages/of_message_hello.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_hello.cpp

bin/of_message_factory.o: openflow_messages/of_message_factory.cpp openflow_messages/of_message_factory.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_factory.cpp

bin/of_message_features_request.o: openflow_messages/of_message_features_request.cpp openflow_messages/of_message_features_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_features_request.cpp

bin/of_message_features_reply.o: openflow_messages/of_message_features_reply.cpp openflow_messages/of_message_features_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_features_reply.cpp

bin/of_message_modify_flow.o: openflow_messages/of_message_modify_flow.cpp openflow_messages/of_message_modify_flow.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_modify_flow.cpp

bin/of_message_packet_in.o: openflow_messages/of_message_packet_in.cpp openflow_messages/of_message_packet_in.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_packet_in.cpp

bin/of_message_packet_out.o: openflow_messages/of_message_packet_out.cpp openflow_messages/of_message_packet_out.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_packet_out.cpp

bin/of_message_port_modification.o: openflow_messages/of_message_port_modification.cpp openflow_messages/of_message_port_modification.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_port_modification.cpp

bin/of_message_port_status.o: openflow_messages/of_message_port_status.cpp openflow_messages/of_message_port_status.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_port_status.cpp

bin/of_message_queue_get_config_reply.o: openflow_messages/of_message_queue_get_config_reply.cpp openflow_messages/of_message_queue_get_config_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_queue_get_config_reply.cpp

bin/of_message_queue_get_config_request.o: openflow_messages/of_message_queue_get_config_request.cpp openflow_messages/of_message_queue_get_config_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_queue_get_config_request.cpp

bin/of_message_set_config.o: openflow_messages/of_message_set_config.cpp openflow_messages/of_message_set_config.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_set_config.cpp

bin/of_message_stats_request.o: openflow_messages/of_message_stats_request.cpp openflow_messages/of_message_stats_request.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_stats_request.cpp

bin/of_message_stats_reply.o: openflow_messages/of_message_stats_reply.cpp openflow_messages/of_message_stats_reply.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_stats_reply.cpp

bin/of_message_vendor.o: openflow_messages/of_message_vendor.cpp openflow_messages/of_message_vendor.h
	$(CC) $(CCOPTS) -o $@ openflow_messages/of_message_vendor.cpp

bin/of_network_utils.o: utils/of_network_utils.cpp utils/of_network_utils.h
	$(CC) $(CCOPTS) -o $@ utils/of_network_utils.cpp

bin/of_port_action.o: openflow_types/of_port_action.cpp openflow_types/of_port_action.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_port_action.cpp

bin/of_port_features.o: openflow_types/of_port_features.cpp openflow_types/of_port_features.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_port_features.cpp

bin/of_port_state.o: openflow_types/of_port_state.cpp openflow_types/of_port_state.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_port_state.cpp

bin/of_queue_config.o: openflow_types/of_queue_config.cpp openflow_types/of_queue_config.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_queue_config.cpp

bin/of_switch_capabilities.o: openflow_types/of_switch_capabilities.cpp openflow_types/of_switch_capabilities.h
	$(CC) $(CCOPTS) -o $@ openflow_types/of_switch_capabilities.cpp

bin/of_types.o: ironstack_types/of_types.cpp ironstack_types/of_types.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/of_types.cpp

bin/openflow_action_list.o: ironstack_types/openflow_action_list.cpp ironstack_types/openflow_action_list.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_action_list.cpp

bin/openflow_aggregate_stats.o: ironstack_types/openflow_aggregate_stats.cpp ironstack_types/openflow_aggregate_stats.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_aggregate_stats.cpp

bin/openflow_flow_description.o: ironstack_types/openflow_flow_description.cpp ironstack_types/openflow_flow_description.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_flow_description.cpp

bin/openflow_flow_description_and_stats.o: ironstack_types/openflow_flow_description_and_stats.cpp ironstack_types/openflow_flow_description_and_stats.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_flow_description_and_stats.cpp

bin/openflow_port.o: ironstack_types/openflow_port.cpp ironstack_types/openflow_port.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_port.cpp

bin/openflow_port_config.o: ironstack_types/openflow_port_config.cpp ironstack_types/openflow_port_config.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_port_config.cpp

bin/openflow_port_stats.o: ironstack_types/openflow_port_stats.cpp ironstack_types/openflow_port_stats.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_port_stats.cpp

bin/openflow_queue_stats.o: ironstack_types/openflow_queue_stats.cpp ironstack_types/openflow_queue_stats.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_queue_stats.cpp

bin/openflow_switch_config.o: ironstack_types/openflow_switch_config.cpp ironstack_types/openflow_switch_config.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_switch_config.cpp

bin/openflow_switch_description.o: ironstack_types/openflow_switch_description.cpp ironstack_types/openflow_switch_description.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_switch_description.cpp

bin/openflow_switch_features.o: ironstack_types/openflow_switch_features.cpp ironstack_types/openflow_switch_features.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_switch_features.cpp

bin/openflow_table_stats.o: ironstack_types/openflow_table_stats.cpp ironstack_types/openflow_table_stats.h
	$(CC) $(CCOPTS) -o $@ ironstack_types/openflow_table_stats.cpp

bin/openflow_utils.o: utils/openflow_utils.cpp utils/openflow_utils.h
	$(CC) $(CCOPTS) -o $@ utils/openflow_utils.cpp

bin/operational_stats.o: services/operational_stats.cpp services/operational_stats.h
	$(CC) $(CCOPTS) -o $@ services/operational_stats.cpp

bin/switch_diagnostics.o: switch_diagnostics.cpp
	$(CC) $(CCOPTS) -o $@ switch_diagnostics.cpp

bin/switch_state.o: services/switch_state.cpp services/switch_state.h
	$(CC) $(CCOPTS) -o $@ services/switch_state.cpp

clean:
	rm -rf *.o switch_diagnostics ironstack port_chat bin/* ../common/*.o

