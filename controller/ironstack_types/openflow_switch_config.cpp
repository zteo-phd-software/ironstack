#include "openflow_switch_config.h"

void openflow_switch_config::clear() {
	frag_normal = false;
	frag_drop = false;
	frag_reasm = false;
	frag_mask = false;
	max_msg_send_len = 0;
}

std::string openflow_switch_config::to_string() const {
	std::stringstream result;
	
	result << "switch configuration: "
		<< (frag_normal ? "FRAG_NORMAL " : "")
		<< (frag_drop ? "FRAG_DROP " : "")
		<< (frag_reasm ? "FRAG_REASM " : "")
		<< (frag_mask ? "FRAG_MASK" : "")
		<< "\nmax message_in summary len: "
		<< max_msg_send_len
		<< " bytes";

	return result.str();
}

autobuf openflow_switch_config::serialize() const {
	// should never get called
	assert(0);
	return autobuf();
}

bool openflow_switch_config::deserialize(const autobuf& input) {
	struct ofp_switch_config config;
	clear();

	if (input.size() != sizeof(config)) {
		return false;
	}

	memcpy(&config, input.get_content_ptr(), sizeof(config));
	max_msg_send_len = ntohs(config.miss_send_len);

	switch (ntohs(config.flags)) {
		case 0:
			frag_normal = true;
			frag_drop = false;
			frag_reasm = false;
			frag_mask = false;
			break;
		
		case 1:
			frag_normal = false;
			frag_drop  = true;
			frag_reasm = false;
			frag_mask = false;
			break;

		case 2:
			frag_normal = false;
			frag_drop = false;
			frag_reasm = true;
			frag_mask = false;
			break;

		case 3:
			frag_normal = false;
			frag_drop = false;
			frag_reasm = false;
			frag_mask = true;
			break;

		default:
			goto fail;
	}

	return true;

fail:
	clear();
	return false;
}

