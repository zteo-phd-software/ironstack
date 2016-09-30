#include "of_common_utils.h"

// converts from a uint32_t to a binary string representation
std::string of_common::uint32_to_binary(uint32_t input)
{
	std::string result;
	for (int counter = 31; counter >= 0; counter--)
	{
		if (input & (1 << counter))
			result += "1";
		else
			result += "0";
//		(input & (1 << counter)) ? result += "1" : result+= "0";
		if ((counter) % 8 == 0)
			result += " ";
	}

	return result;
}

// converts from a uint8_t to a binary string representation
std::string of_common::uint8_to_binary(uint8_t input)
{
	return uint8_array_to_binary(&input, 1);
}

// converts from an array of uint8_t to a binary string representation
std::string of_common::uint8_array_to_binary(uint8_t* input, uint32_t len)
{
	std::string result;
	uint32_t counter, counter2;
	for (counter = 0; counter < len; counter++)
	{
		for (counter2=7; counter2 >= 0; counter2--)
		{
			if (input[counter] & (1 << counter2))
				result += "1";
			else
				result += "0";
		}
		result += " ";
	}

	return result;
}

// convert from a binary input to a readable hex form
std::string of_common::binary_to_hex_output(const autobuf& input)
{
	char buf[16];
	std::string result;

	for (uint32_t counter = 0; counter < input.size(); counter++)
	{
		sprintf(buf, "[%02x] ", input[counter]);
		result += buf;

		if (((counter+1) % 16 == 0) && (counter+1 < input.size()))
			result += "\n";
		else if ((counter+1) % 4 == 0)
			result += " ";
	}

	return result;
}

// convert from a port number to a string form
std::string of_common::port_to_string(uint16_t port)
{
	char buf[64];
	std::string result;
	
	switch(port)
	{
		case ((uint16_t) OFPP_IN_PORT):
			result = "INPUT PORT";
			break;
		case ((uint16_t) OFPP_TABLE):
			result = "PERFORM ACTIONS IN FLOW TABLE";
			break;
		case ((uint16_t) OFPP_NORMAL):
			result = "PROCESS WITH NORMAL L2/L3 SWITCHING";
			break;
		case ((uint16_t) OFPP_FLOOD):
			result = "FLOOD ALL EXCEPT INPUT AND STP DISABLED PORTS";
			break;
		case ((uint16_t) OFPP_ALL):
			result = "FLOOD ALL EXCEPT INPUT PORT";
			break;
		case ((uint16_t) OFPP_CONTROLLER):
			result = "SEND TO CONTROLLER";
			break;
		case ((uint16_t) OFPP_LOCAL):
			result = "LOCAL OPENFLOW PORT";
			break;
		case ((uint16_t) OFPP_NONE):
			result = "NOT A PORT";
			break;
		default:
			sprintf(buf, "%u", port);
			result = buf;
	}

	return result;
}

// converts from an enum message type to a readable form
std::string of_common::msg_type_to_string(enum ofp_type type)
{
	std::string m_type;

	switch (type)
	{
		case OFPT_HELLO:
			m_type = "OFPT_HELLO";
			break;
		case OFPT_ERROR:
			m_type = "OFPT_ERROR";
			break;
		case OFPT_ECHO_REQUEST:
			m_type = "OFPT_ECHO_REQUEST";
			break;
		case OFPT_ECHO_REPLY:
			m_type = "OFPT_ECHO_REPLY";
			break;
		case OFPT_VENDOR:
			m_type = "OFPT_VENDOR";
			break;
		case OFPT_FEATURES_REQUEST:
			m_type = "OFPT_FEATURES_REQUEST";
			break;
		case OFPT_FEATURES_REPLY:
			m_type = "OFPT_FEATURES_REPLY";
			break;
		case OFPT_GET_CONFIG_REQUEST:
			m_type = "OFPT_GET_CONFIG_REQUEST";
			break;
		case OFPT_GET_CONFIG_REPLY:
			m_type = "OFPT_GET_CONFIG_REPLY";
			break;
		case OFPT_SET_CONFIG:
			m_type = "OFPT_SET_CONFIG";
			break;
		case OFPT_PACKET_IN:
			m_type = "OFPT_PACKET_IN";
			break;
		case OFPT_FLOW_REMOVED:
			m_type = "OFPT_FLOW_REMOVED";
			break;
		case OFPT_PORT_STATUS:
			m_type = "OFPT_PORT_STATUS";
			break;
		case OFPT_PACKET_OUT:
			m_type = "OFPT_PACKET_OUT";
			break;
		case OFPT_FLOW_MOD:
			m_type = "OFPT_FLOW_MOD";
			break;
		case OFPT_PORT_MOD:
			m_type = "OFPT_PORT_MOD";
			break;
		case OFPT_STATS_REQUEST:
			m_type = "OFPT_STATS_REQUEST";
			break;
		case OFPT_STATS_REPLY:
			m_type = "OFPT_STATS_REPLY";
			break;
		case OFPT_BARRIER_REQUEST:
			m_type = "OFPT_BARRIER_REQUEST";
			break;
		case OFPT_BARRIER_REPLY:
			m_type = "OFPT_BARRIER_REPLY";
			break;
		case OFPT_QUEUE_GET_CONFIG_REQUEST:
			m_type = "OFPT_QUEUE_GET_CONFIG_REQUEST";
			break;
		case OFPT_QUEUE_GET_CONFIG_REPLY:
			m_type = "OFPT_QUEUE_GET_CONFIG_REPLY";
			break;
		default:
			m_type = "UNKNOWN";
	}

	return m_type;
}


// pads to a certain position with whitespace
void of_common::pad_to_position(std::string& input, uint32_t position)
{
	char buf[512];
	int spaces_to_fill = position - input.size();
	if (spaces_to_fill <= 0 || spaces_to_fill >= (int)sizeof(buf)-1)
		return;

	memset(buf, ' ', sizeof(buf));
	buf[spaces_to_fill] = '\0';
	input += buf;
}


