#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <vector>

class hal;
class service;
using namespace std;

// catalog to track all services
class service_catalog {
public:

	// define service types
	enum class service_type { ARP = 1, 
														CAM = 2, 
														SWITCH_STATE = 3,
														FLOWS = 4,
														ECHO_SERVER = 5,
	                          OPERATIONAL_STATS = 6,
														FLOW_POLICY_CHECKER = 7,
														INTER_IRONSTACK = 8,
                            ALWAYS_AT_LAST = 9		// sentinel to mark end of services
														                      // not a real service!
	};

	// constructor
	service_catalog():controller(nullptr)	{}

	// removes all services (does not change controller)
	void clear();

	// service-related functions
	void                register_service(const shared_ptr<service>& svc);
	void                unregister_service(const shared_ptr<service>& svc);
	void                unregister_service(service_type svc_name);
	shared_ptr<service> get_service(service_type svc_name);
	bool                deferred_init_services();

	// controller-related functions
	void                set_controller(hal* controller);
	hal*                get_controller();

private:

	mutable mutex             lock;
	vector<weak_ptr<service>> services;
	hal*                      controller;
};

// service class; all services must subclass this
class service {
public:

	// constructor
	service(service_catalog* ptr,
		service_catalog::service_type type,
		int major,
		int minor):

		service_catalog_ptr(ptr),
		service_type(type),
		service_major_version(major),
		service_minor_version(minor) {

		controller = service_catalog_ptr->get_controller();
	}

	virtual ~service() {};

	// functions to start and shutdown the service. these must be implemented.
	// init() performs initialization of the service before processing starts
	// on the switch.
	// init2() performs initialization of the service after processing has
	// started (if for example, the service depends on having some controller
	// packet processing to work).
	// dependencies are honored only for init2() functions; init() functions
	// should not assume other services are online.
	virtual bool init() = 0;
	virtual bool init2() = 0;
	virtual void shutdown() = 0;

	// gets debugging information about the service
	// implement these functions
	virtual string get_service_info() const = 0;
	virtual string get_running_info() const = 0;

	// returns information about the service type
	service_catalog::service_type get_service_type() const { 
		return service_type;
	}

	// returns information about the service version
	tuple<int, int> get_version() const { 
		return make_tuple(service_major_version, service_minor_version);
	}

	// gets service dependencies
	set<service_catalog::service_type> get_dependencies() const { 
		return dependencies;
	}

protected:

	service_catalog*                   service_catalog_ptr;
	service_catalog::service_type      service_type;
	hal*                               controller;
	int                                service_major_version;
	int                                service_minor_version;
	set<service_catalog::service_type> dependencies;

};

