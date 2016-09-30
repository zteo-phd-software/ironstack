#include "of_message_factory.h"
#include "../gui/output.h"

// deserializes a raw input stream into one of the message types
shared_ptr<of_message> ironstack::of_message_factory::deserialize_message(const autobuf& input) {

	of_message message_base;
	shared_ptr<of_message> result;
	if (!message_base.deserialize(input)) {
		return nullptr;
	}

	// switch between detected types
	switch (message_base.msg_type) {
		case (OFPT_HELLO):
			result.reset(new of_message_hello());
			break;

		case (OFPT_ERROR):
//			output::log(output::loglevel::ERROR, "\n** an error with the openflow switch has occurred. **\n");
			result.reset(new of_message_error());
			break;

		case (OFPT_ECHO_REQUEST):
			result.reset(new of_message_echo_request());
			break;

		case (OFPT_ECHO_REPLY):
			result.reset(new of_message_echo_reply());
			break;

		case (OFPT_VENDOR):
			result.reset(new of_message_vendor());
			break;

		case (OFPT_FEATURES_REQUEST):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_FEATURES_REPLY):
			result.reset(new of_message_features_reply());
			break;

		case (OFPT_GET_CONFIG_REQUEST):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_GET_CONFIG_REPLY):
			result.reset(new of_message_get_config_reply());
			break;

		case (OFPT_SET_CONFIG):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_PACKET_IN):
			result.reset(new of_message_packet_in());
			break;

		case (OFPT_FLOW_REMOVED):
			result.reset(new of_message_flow_removed());
			break;

		case (OFPT_PORT_STATUS):
			result.reset(new of_message_port_status());
			break;

		case (OFPT_PACKET_OUT):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_FLOW_MOD):
			result.reset(new of_message_flow_removed());
			break;

		case (OFPT_PORT_MOD):
			result.reset(new of_message_port_status());
			break;

		case (OFPT_STATS_REQUEST):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_STATS_REPLY):
			#ifndef __NO_OPENFLOW_SAFETY_CHECKS
			if (input.size() < sizeof(struct ofp_stats_reply)) {
				abort();
			}
			#endif
			
			switch(ntohs(((struct ofp_stats_reply*)input.get_content_ptr())->type)) {

				case OFPST_DESC:
					result.reset(new of_message_stats_reply_switch_description());
					break;

				case OFPST_FLOW:
					result.reset(new of_message_stats_reply_flow_stats());
					break;

				case OFPST_AGGREGATE:
					result.reset(new of_message_stats_reply_aggregate_stats());
					break;

				case OFPST_TABLE:
					result.reset(new of_message_stats_reply_table_stats());
					break;

				case OFPST_PORT:
					result.reset(new of_message_stats_reply_port_stats());
					break;
				case OFPST_QUEUE:
					result.reset(new of_message_stats_reply_queue_stats());
					break;
				case OFPST_VENDOR:
					output::log(output::loglevel::ERROR, "of_message_factory::deserialize() error -- vendor extensions not supported yet.\n");
					abort();
					break;

				default:
					output::log(output::loglevel::BUG, "of_message_factory::deserialize() error -- unknown of_message_stats_reply unknown type.\n");
					abort();
			}
			break;

		case (OFPT_BARRIER_REQUEST):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_BARRIER_REPLY):
			result.reset(new of_message_barrier_reply());
			break;

		case (OFPT_QUEUE_GET_CONFIG_REQUEST):
			// should not be called; controller to switch only
			abort();
			break;

		case (OFPT_QUEUE_GET_CONFIG_REPLY):
			result.reset(new of_message_queue_get_config_reply());
			break;

		default:
			output::log(output::loglevel::BUG, "of_message_factory::deserialize_message() error -- unknown serialization.\n");
			goto fail;
	};

	// perform actual deserialization
	if (!result->deserialize(input)) {
		goto fail;
	}
	return result;

fail:
	output::log(output::loglevel::ERROR, "of_message_factory::deserialize_message() error -- unable to deserialize message!\n");
	return nullptr;
}
