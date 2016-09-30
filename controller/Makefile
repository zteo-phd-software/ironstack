#CC = /usr/local/gcc-5.1/bin/g++
CC = g++-4.8
CCOPTS = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -fexceptions -std=c++1y -pg -rdynamic -fno-strict-aliasing -Wno-unused-result -D_GLIBCXX_USE_NANOSLEEP -Wno-deprecated-declarations -D__STDC_FORMAT_MACROS -fno-builtin-printf -D__TEXT_GUI
CCOPTS_EXCEPTIONS = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -std=c++1y -pg -rdynamic -fno-strict-aliasing -Wno-unused-result -D_GLIBCXX_USE_NANOSLEEP -Wno-deprecated-declarations -D__STDC_FORMAT_MACROS
CCOPTSFAST = -c -g -Wall -Wformat-nonliteral -ggdb -funsigned-char -fno-exceptions -std=c++1y -pg -Ofast -march=native -flto -rdynamic -fno-strict-aliasing -Wno-unused-result
LINKOPTS = -g -pthread -pg -std=c++1y -Ofast -march=native -flto -rdynamic -Wno-unused-result
LIBS = -lncurses

# build all targets
all: compile_dir lookup ironstack port_chat

# generates a 'bin' directory to stage binary files
compile_dir:
	mkdir -p bin

# cleanup built files
clean:
	rm -rf *.o switch_diagnostics ironstack port_chat bin/* ../common/*.o



# openflow controller
ironstack: ../common/autobuf.o \
	../common/autobuf_packer.o \
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
	../common/switch_telnet.o \
	../common/timed_barrier.o \
	../common/timer.o \
	bin/gui_component.o \
	bin/gui_controller.o \
	bin/gui_defs.o \
	bin/input_menu.o \
	bin/input_textbox.o \
	bin/key_reader.o \
	bin/output.o \
	bin/progress_bar.o \
	bin/inter_ironstack_message.o \
	bin/ethernet_ping.o \
	bin/ethernet_pong.o \
	bin/invalidate_mac.o \
	bin/arp.o \
	bin/arp_table.o \
	bin/aux_switch_info.o \
	bin/cam.o \
	bin/cam_table.o \
	bin/dell_s48xx_acl_table.o \
	bin/dell_s48xx_l2_table.o \
	bin/ethernet_mac_db.o \
	bin/flow_parser.o \
	bin/flow_policy_checker.o \
	bin/flow_table.o \
	bin/flow_service.o \
	bin/hal.o \
	bin/hal_transaction.o \
	bin/ironstack_echo_daemon.o \
	bin/ironstack_gui.o \
	bin/inter_ironstack_message.o \
	bin/inter_ironstack_service.o \
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
	bin/openflow_flow_rate.o \
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
	bin/operational_stats.o \
	bin/packet_in_processor.o \
	bin/service_catalog.o \
	bin/stacktrace.o \
	bin/std_packet.o \
	bin/switch_commander.o \
	bin/switch_db.o \
	bin/switch_state.o \
	bin/ironstack.o
	$(CC) $(LINKOPTS) -o $@ $^ $(LIBS)

# switch diagnostics
switch_diagnostics: ../common/autobuf.o \
	../common/autobuf_packer.o \
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
	bin/stacktrace.o \
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
	bin/flow_policy_checker.o \
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
	bin/operational_stats.o \
	bin/packet_in_processor.o \
	bin/service_catalog.o \
	bin/std_packet.o \
	bin/switch_db.o \
	bin/switch_state.o \
	bin/switch_diagnostics.o
	$(CC) $(LINKOPTS) -o $@ $^ $(LIBS)


# csv lookup utility
lookup: ../common/csv_parser.o \
	../common/ip_address.o \
	../common/mac_address.o \
	bin/aux_switch_info.o \
	bin/output.o \
	bin/gui_controller.o \
	bin/gui_component.o \
	bin/gui_defs.o \
	bin/switch_db.o \
	bin/lookup.o 
	$(CC) $(LINKOPTS) -o $@ $^ $(LIBS)


# simple chat utility
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


# main executives
bin/ironstack.o: ironstack.cpp
	$(CC) $(CCOPTS) -o $@ $<

bin/lookup.o: lookup.cpp
	$(CC) $(CCOPTS) -o $@ $<

bin/port_chat.o: port_chat.cpp
	$(CC) $(CCOPTS) -o $@ $<

bin/switch_diagnostics.o: switch_diagnostics.cpp
	$(CC) $(CCOPTS) -o $@ $<


# hardware abstraction layer
bin/hal.o: hal/hal.cpp hal/hal.h
	$(CC) $(CCOPTS) -o $@ $<

bin/hal_transaction.o: hal/hal_transaction.cpp hal/hal_transaction.h
	$(CC) $(CCOPTS) -o $@ $<

bin/packet_in_processor.o: hal/packet_in_processor.cpp hal/packet_in_processor.h
	$(CC) $(CCOPTS) -o $@ $<

bin/service_catalog.o: hal/service_catalog.cpp hal/service_catalog.h
	$(CC) $(CCOPTS) -o $@ $<


# services and service-related components
bin/arp.o: services/arp.cpp services/arp.h
	$(CC) $(CCOPTS) -o $@ $<

bin/arp_table.o: services/arp_table.cpp services/arp_table.h
	$(CC) $(CCOPTS) -o $@ $<

bin/aux_switch_info.o: services/aux_switch_info.cpp services/aux_switch_info.h
	$(CC) $(CCOPTS) -o $@ $<

bin/cam.o: services/cam.cpp services/cam.h
	$(CC) $(CCOPTS) -o $@ $<

bin/cam_table.o: services/cam_table.cpp services/cam_table.h
	$(CC) $(CCOPTS) -o $@ $<

bin/dell_s48xx_acl_table.o: services/dell_s48xx_acl_table.cpp services/dell_s48xx_acl_table.h
	$(CC) $(CCOPTS) -o $@ $<

bin/dell_s48xx_l2_table.o: services/dell_s48xx_l2_table.cpp services/dell_s48xx_l2_table.h
	$(CC) $(CCOPTS) -o $@ $<

bin/flow_parser.o: utils/flow_parser.cpp utils/flow_parser.h
	$(CC) $(CCOPTS) -o $@ $<

bin/flow_policy_checker.o: services/flow_policy_checker.cpp services/flow_policy_checker.h
	$(CC) $(CCOPTS) -o $@ $<

bin/flow_service.o: services/flow_service.cpp services/flow_service.h
	$(CC) $(CCOPTS) -o $@ $<

bin/flow_table.o: services/flow_table.cpp services/flow_table.h
	$(CC) $(CCOPTS) -o $@ $<

bin/ironstack_echo_daemon.o: services/ironstack_echo_daemon.cpp services/ironstack_echo_daemon.h
	$(CC) $(CCOPTS) -o $@ services/ironstack_echo_daemon.cpp

bin/operational_stats.o: services/operational_stats.cpp services/operational_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/switch_state.o: services/switch_state.cpp services/switch_state.h
	$(CC) $(CCOPTS) -o $@ $<


# deprecated ironstack stuff or not currently in use
bin/ironstack_discovery_daemon.o: services/ironstack_discovery_daemon.cpp services/ironstack_discovery_daemon.h
	$(CC) $(CCOPTS) -o $@ $<

bin/inter_ironstack_service.o: services/inter_ironstack_service.cpp services/inter_ironstack_service.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_flow_rate.o: ironstack_types/openflow_flow_rate.cpp ironstack_types/openflow_flow_rate.h
	$(CC) $(CCOPTS) -o $@ $<

bin/inter_ironstack_message.o: inter_ironstack_messages/inter_ironstack_message.cpp inter_ironstack_messages/inter_ironstack_message.h
	$(CC) $(CCOPTS) -o $@ $<

bin/invalidate_mac.o: inter_ironstack_messages/invalidate_mac.cpp inter_ironstack_messages/invalidate_mac.h
	$(CC) $(CCOPTS) -o $@ $<

bin/ethernet_ping.o: inter_ironstack_messages/ethernet_ping.cpp inter_ironstack_messages/ethernet_ping.h
	$(CC) $(CCOPTS) -o $@ $<

bin/ethernet_pong.o: inter_ironstack_messages/ethernet_pong.cpp inter_ironstack_messages/ethernet_pong.h
	$(CC) $(CCOPTS) -o $@ $<

bin/ironstack_discovery_service.o: services/ironstack_discovery_service.h services/ironstack_discovery_service.cpp
	$(CC) $(CCOPTS) -o $@ $<

bin/topology_graph.o: services/topology_graph.h services/topology_graph.cpp 
	$(CC) $(CCOPTS) -o $@ $<


# GUI-related
bin/ironstack_gui.o: ironstack_gui.cpp ironstack_gui.h
	$(CC) $(CCOPTS) -o $@ $<

bin/output.o: gui/output.cpp gui/output.h
	$(CC) $(CCOPTS) -o $@ $<

bin/gui_component.o: gui/gui_component.cpp gui/gui_component.h
	$(CC) $(CCOPTS) -o $@ $<

bin/gui_defs.o: gui/gui_defs.cpp gui/gui_defs.h
	$(CC) $(CCOPTS) -o $@ $<

bin/gui_controller.o: gui/gui_controller.cpp gui/gui_controller.h
	$(CC) $(CCOPTS) -o $@ $<

bin/input_menu.o: gui/input_menu.cpp gui/input_menu.h
	$(CC) $(CCOPTS) -o $@ $<

bin/input_textbox.o: gui/input_textbox.cpp gui/input_textbox.h
	$(CC) $(CCOPTS) -o $@ $<

bin/key_reader.o: gui/key_reader.cpp gui/key_reader.h
	$(CC) $(CCOPTS) -o $@ $<

bin/progress_bar.o: gui/progress_bar.cpp gui/progress_bar.h
	$(CC) $(CCOPTS) -o $@ $<


# support utilities
bin/ethernet_mac_db.o: utils/ethernet_mac_db.cpp utils/ethernet_mac_db.h
	$(CC) $(CCOPTS) -o $@ $<

bin/stacktrace.o: utils/stacktrace.cpp utils/stacktrace.h
	$(CC) $(CCOPTS) -o $@ $<

bin/switch_commander.o: switch_commander/switch_commander.cpp switch_commander/switch_commander.h
	$(CC) $(CCOPTS) -o $@ $<

bin/switch_db.o: services/switch_db.cpp services/switch_db.h
	$(CC) $(CCOPTS) -o $@ $<


# openflow-protocol related messages/objects
bin/of_action.o: openflow_types/of_action.cpp openflow_types/of_action.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_actions_supported.o: openflow_types/of_actions_supported.cpp openflow_types/of_actions_supported.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_common_utils.o: utils/of_common_utils.cpp utils/of_common_utils.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_match.o: openflow_types/of_match.cpp openflow_types/of_match.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message.o: openflow_messages/of_message.cpp openflow_messages/of_message.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_barrier_reply.o: openflow_messages/of_message_barrier_reply.cpp openflow_messages/of_message_barrier_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_barrier_request.o: openflow_messages/of_message_barrier_request.cpp openflow_messages/of_message_barrier_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_echo_request.o: openflow_messages/of_message_echo_request.cpp openflow_messages/of_message_echo_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_echo_reply.o: openflow_messages/of_message_echo_reply.cpp openflow_messages/of_message_echo_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_error.o: openflow_messages/of_message_error.cpp openflow_messages/of_message_error.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_flow_removed.o: openflow_messages/of_message_flow_removed.cpp openflow_messages/of_message_flow_removed.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_get_config_request.o: openflow_messages/of_message_get_config_request.cpp openflow_messages/of_message_get_config_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_get_config_reply.o: openflow_messages/of_message_get_config_reply.cpp openflow_messages/of_message_get_config_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_hello.o: openflow_messages/of_message_hello.cpp openflow_messages/of_message_hello.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_factory.o: openflow_messages/of_message_factory.cpp openflow_messages/of_message_factory.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_features_request.o: openflow_messages/of_message_features_request.cpp openflow_messages/of_message_features_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_features_reply.o: openflow_messages/of_message_features_reply.cpp openflow_messages/of_message_features_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_modify_flow.o: openflow_messages/of_message_modify_flow.cpp openflow_messages/of_message_modify_flow.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_packet_in.o: openflow_messages/of_message_packet_in.cpp openflow_messages/of_message_packet_in.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_packet_out.o: openflow_messages/of_message_packet_out.cpp openflow_messages/of_message_packet_out.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_port_modification.o: openflow_messages/of_message_port_modification.cpp openflow_messages/of_message_port_modification.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_port_status.o: openflow_messages/of_message_port_status.cpp openflow_messages/of_message_port_status.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_queue_get_config_reply.o: openflow_messages/of_message_queue_get_config_reply.cpp openflow_messages/of_message_queue_get_config_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_queue_get_config_request.o: openflow_messages/of_message_queue_get_config_request.cpp openflow_messages/of_message_queue_get_config_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_set_config.o: openflow_messages/of_message_set_config.cpp openflow_messages/of_message_set_config.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_stats_request.o: openflow_messages/of_message_stats_request.cpp openflow_messages/of_message_stats_request.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_stats_reply.o: openflow_messages/of_message_stats_reply.cpp openflow_messages/of_message_stats_reply.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_message_vendor.o: openflow_messages/of_message_vendor.cpp openflow_messages/of_message_vendor.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_network_utils.o: utils/of_network_utils.cpp utils/of_network_utils.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_port_action.o: openflow_types/of_port_action.cpp openflow_types/of_port_action.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_port_features.o: openflow_types/of_port_features.cpp openflow_types/of_port_features.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_port_state.o: openflow_types/of_port_state.cpp openflow_types/of_port_state.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_queue_config.o: openflow_types/of_queue_config.cpp openflow_types/of_queue_config.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_switch_capabilities.o: openflow_types/of_switch_capabilities.cpp openflow_types/of_switch_capabilities.h
	$(CC) $(CCOPTS) -o $@ $<

bin/of_types.o: ironstack_types/of_types.cpp ironstack_types/of_types.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_action_list.o: ironstack_types/openflow_action_list.cpp ironstack_types/openflow_action_list.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_aggregate_stats.o: ironstack_types/openflow_aggregate_stats.cpp ironstack_types/openflow_aggregate_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_flow_description.o: ironstack_types/openflow_flow_description.cpp ironstack_types/openflow_flow_description.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_flow_description_and_stats.o: ironstack_types/openflow_flow_description_and_stats.cpp ironstack_types/openflow_flow_description_and_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_flow_entry.o: ironstack_types/openflow_flow_entry.cpp ironstack_types/openflow_flow_entry.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_port.o: ironstack_types/openflow_port.cpp ironstack_types/openflow_port.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_port_config.o: ironstack_types/openflow_port_config.cpp ironstack_types/openflow_port_config.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_port_stats.o: ironstack_types/openflow_port_stats.cpp ironstack_types/openflow_port_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_queue_stats.o: ironstack_types/openflow_queue_stats.cpp ironstack_types/openflow_queue_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_switch_config.o: ironstack_types/openflow_switch_config.cpp ironstack_types/openflow_switch_config.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_switch_description.o: ironstack_types/openflow_switch_description.cpp ironstack_types/openflow_switch_description.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_switch_features.o: ironstack_types/openflow_switch_features.cpp ironstack_types/openflow_switch_features.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_table_stats.o: ironstack_types/openflow_table_stats.cpp ironstack_types/openflow_table_stats.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_utils.o: utils/openflow_utils.cpp utils/openflow_utils.h
	$(CC) $(CCOPTS) -o $@ $<

bin/openflow_vlan_port.o: ironstack_types/openflow_vlan_port.cpp ironstack_types/openflow_vlan_port.h
	$(CC) $(CCOPTS) -o $@ $<


# common utilities
../common/autobuf.o: ../common/autobuf.cpp ../common/autobuf.h
	$(CC) $(CCOPTS) -o $@ $<

../common/csv_parser.o: ../common/csv_parser.cpp ../common/csv_parser.h
	$(CC) $(CCOPTS) -o $@ $<

../common/ip_address.o: ../common/ip_address.cpp ../common/ip_address.h
	$(CC) $(CCOPTS) -o $@ $<

../common/ip_port.o: ../common/ip_port.cpp ../common/ip_port.h
	$(CC) $(CCOPTS) -o $@ $<

../common/mac_address.o: ../common/mac_address.cpp ../common/mac_address.h
	$(CC) $(CCOPTS) -o $@ $<

bin/std_packet.o: utils/std_packet.cpp utils/std_packet.h
	$(CC) $(CCOPTS) -o $@ $<

../common/switch_telnet.o: ../common/switch_telnet.cpp ../common/switch_telnet.h
	$(CC) $(CCOPTS) -o $@ $<

../common/tcp.o: ../common/tcp.cpp ../common/tcp.h
	$(CC) $(CCOPTS) -o $@ $<

../common/timer.o: ../common/timer.cpp ../common/timer.h
	$(CC) $(CCOPTS) -o $@ $<

../common/timed_barrier.o: ../common/timed_barrier.cpp ../common/timed_barrier.h
	$(CC) $(CCOPTS) -o $@ $<

# common utilities not currently in use
../common/autobuf_packer.o: ../common/autobuf_packer.cpp ../common/autobuf_packer.h
	$(CC) $(CCOPTS) -o $@ $<

../common/ipv6_address.o: ../common/ipv6_address.cpp ../common/ipv6_address.h
	$(CC) $(CCOPTS) -o $@ $<

../common/ironscale_packet.o: ../common/ironscale_packet.cpp ../common/ironscale_packet.h
	$(CC) $(CCOPTS) -o $@ $<

../common/z_allocator.o: ../common/z_allocator.cpp ../common/z_allocator.h
	$(CC) $(CCOPTS) -o $@ $<

# common utilities to be deprecated soon
../common/common_utils.o: ../common/common_utils.cpp ../common/common_utils.h
	$(CC) $(CCOPTS) -o $@ $<

../common/common_utils_oop.o: ../common/common_utils_oop.cpp ../common/common_utils_oop.h
	$(CC) $(CCOPTS) -o $@ $<

../common/gui.o: ../common/gui.cpp ../common/gui.h
	$(CC) $(CCOPTS) -o $@ $<
