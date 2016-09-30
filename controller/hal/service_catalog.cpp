#include <algorithm>
#include <set>
#include "service_catalog.h"
#include "../gui/output.h"
using namespace std;

// removes all services
void service_catalog::clear() {
	lock_guard<mutex> g(lock);
	services.clear();
	services.reserve((int)service_type::ALWAYS_AT_LAST);
	for (uint32_t counter = 0; counter < (int)service_type::ALWAYS_AT_LAST; ++counter) {
		services.push_back(shared_ptr<service>(nullptr));
	}
}

// initializes and registers a service, overwriting the old one
void service_catalog::register_service(const shared_ptr<service>& svc) {

	// init the service
	if (!svc->init()) {
		output::log(output::loglevel::ERROR, "service_catalog: unable to initialize service [%s].\n", svc->get_service_info().c_str());
		return;
	}

	// register it
	lock_guard<mutex> g(lock);
	service_type type = svc->get_service_type();
	services[(int)type] = svc;
	output::log(output::loglevel::INFO, "service_catalog: registered service [%s].\n", svc->get_service_info().c_str());
}

// unregisters a service
void service_catalog::unregister_service(const shared_ptr<service>& svc) {
	lock_guard<mutex> g(lock);
	shared_ptr<service> current_service;

	for (uint32_t counter = 0; counter < (int) service_type::ALWAYS_AT_LAST; ++counter) {
		current_service = services[counter].lock();
		if (current_service == svc) {
			output::log(output::loglevel::INFO, "service_catalog: unregistered service [%s].\n",
				current_service->get_service_info().c_str());
			services[counter] = shared_ptr<service>(nullptr);
			break;
		}
	}
}

// unregisters a service
void service_catalog::unregister_service(service_type svc_name) {
	lock_guard<mutex> g(lock);

	shared_ptr<service> svc = services[(int)svc_name].lock();
	if (svc != nullptr) {
		output::log(output::loglevel::INFO, "service_catalog: unregistered service [%s].\n",
			svc->get_service_info().c_str());
	} else {
		output::log(output::loglevel::WARNING, "service_catalog: service already unregistered.\n");
	}
	services[(int)svc_name] = shared_ptr<service>(nullptr);
}

// locks and retrieves a shared_ptr to the service
shared_ptr<service> service_catalog::get_service(service_type svc_name) {
	lock_guard<mutex> g(lock);
	return services[(int) svc_name].lock();
}

// initializes services by calling their init2() deferred init functions
bool service_catalog::deferred_init_services() {

	set<service_catalog::service_type> initialized_services;
	set<shared_ptr<service>> uninitialized_services;
	shared_ptr<service> current_service;

	// collect list of uninitialized services
	for (auto iterator = services.begin(); iterator != services.end(); ++iterator) {
		current_service = iterator->lock();
		if (current_service != nullptr) {
			uninitialized_services.insert(current_service);
		}
	}

	// now try and initialize all of them
	set<service_catalog::service_type> dependencies;
	bool initialized_this_round = true;
	while (!uninitialized_services.empty() && initialized_this_round) {

		initialized_this_round = false;

		auto iterator = uninitialized_services.begin();
		while (iterator != uninitialized_services.end()) {
			dependencies = (*iterator)->get_dependencies();

			// if dependencies have been fulfilled, initialize the service
			if (includes(initialized_services.begin(), initialized_services.end(),
				dependencies.begin(), dependencies.end())) {
				
				if ((*iterator)->init2()) {
					initialized_services.insert((*iterator)->get_service_type());
					iterator = uninitialized_services.erase(iterator);
					initialized_this_round = true;
				} else {
					return false;
				}
			} else {
				++iterator;
			}
		}
	}

	// did all services initialize? if not, report them and fail.
	if (!uninitialized_services.empty()) {
		output::log(output::loglevel::BUG, "service_catalog::deferred_init_services() -- the following services have dependency problems:\n");
		for (const auto& s : uninitialized_services) {
			output::log(output::loglevel::BUG, "  [%s]\n", s->get_service_info().c_str());
		}
		return false;
	}

	return true;
}

// sets the controller (this is a hard pointer but it is acceptable because
// the service catalog cannot exist without the controller)
void service_catalog::set_controller(hal* controller_) {
	controller = controller_;
}

// retrieves the controller
hal* service_catalog::get_controller() {
	return controller;
}

