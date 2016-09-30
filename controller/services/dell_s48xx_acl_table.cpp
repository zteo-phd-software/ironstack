#include "dell_s48xx_acl_table.h"

// the dell acl tables will generally accept all kinds of flows
bool dell_s48xx_acl_table::check_table_fit(const of_match& criteria,
	const openflow_action_list& action_list) const {

	return true;
}

// returns the table name
string dell_s48xx_acl_table::get_table_name() const {
	return string("dell_s48xx ACL table");
}
