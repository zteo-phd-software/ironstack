#include "of_types.h"

std::string of_types::port_to_string(uint32_t port_id)
{
	std::string result;

	switch (port_id)
	{
		case ((uint32_t) OFPP_IN_PORT):
			result = "INPUT_PORT";
			break;
		case ((uint32_t) OFPP_TABLE):
			result = "TABLE_ACTION";
			break;
		case ((uint32_t) OFPP_NORMAL):
			result = "NORMAL_L2/L3_SWITCHING";
			break;
		case ((uint32_t) OFPP_FLOOD):
			result = "FLOOD_ALL_PORTS";
			break;
		case ((uint32_t) OFPP_ALL):
			result = "ALL_PORTS_EXCEPT_INPUT";
			break;
		case ((uint32_t) OFPP_CONTROLLER):
			result = "SEND_TO_CONTROLLER";
			break;
		case ((uint32_t) OFPP_LOCAL):
			result = "LOCAL_OPENFLOW_PORT";
			break;
		case ((uint32_t) OFPP_NONE):
			result = "NONE";
			break;
		default:
			result = "";
	}

	return result;
}
