#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include "of_message.h"
#include "of_message_barrier_reply.h"
#include "of_message_barrier_request.h"
#include "of_message_hello.h"
#include "of_message_echo_request.h"
#include "of_message_echo_reply.h"
#include "of_message_error.h"
#include "of_message_features_request.h"
#include "of_message_features_reply.h"
#include "of_message_flow_removed.h"
#include "of_message_get_config_request.h"
#include "of_message_get_config_reply.h"
#include "of_message_packet_in.h"
#include "of_message_packet_out.h"
#include "of_message_port_status.h"
#include "of_message_queue_get_config_request.h"
#include "of_message_queue_get_config_reply.h"
#include "of_message_set_config.h"
#include "of_message_stats_request.h"
#include "of_message_stats_reply.h"
#include "of_message_vendor.h"
#include "of_message_modify_flow.h"

namespace ironstack {
namespace of_message_factory {

	// deserializes a raw message into an openflow message using the
	// placement new mechanism. message contents are stored in msg_buf.
	of_message* deserialize_message(const autobuf& input, autobuf& msg_buf);

	// deserializes a raw socket buffer into an openflow message.
	shared_ptr<of_message> deserialize_message(const autobuf& input);
}};

