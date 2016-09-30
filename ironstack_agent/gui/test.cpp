#include "gui_controller.h"
#include "gui_component.h"
#include "input_menu.h"
#include "input_textbox.h"
#include "key_reader.h"
#include "output.h"
#include "progress_bar.h"
#include "stacktrace.h"
#include <unistd.h>

using namespace gui;

int main(int argc, char** argv) {
	stacktrace::enable();
	shared_ptr<gui_controller> controller(new gui_controller());
	output::init(controller);
	controller->init();
	//controller->set_logging("test.txt");

	shared_ptr<gui_component> component1(new gui_component());
	component1->load("x.cfg");

//	component1->set_origin(vec2d(1,1));
//	component1->set_z_position(1);
	component1->set_cursor_visible(true);
	component1->set_cursor_color(FG_WHITE | BG_YELLOW);
//	component1->set_dimensions(vec2d(10,10));
	component1->set_color(FG_WHITE | BG_BLUE);
	component1->clear();
	for (uint32_t counter=0; counter < 15; ++counter) {
		component1->printf(vec2d(0, counter), "test%u\n", counter);
	}

	controller->register_gui_component(component1);
	component1->commit_all();

	// test progress bar code here
	shared_ptr<progress_bar> progress(new progress_bar());
	progress->set_origin(vec2d(20, 15));
	progress->set_dimensions(vec2d(30,1));
	progress->set_z_position(3);
	progress->set_progress_bar_color(FG_WHITE | BG_YELLOW, FG_WHITE | BG_BLUE);
	progress->set_max(1000);
	progress->set_value(125);
	progress->set_view_type(gui::progress_bar::view_type::NUMERICAL);
	progress->commit_all();
	controller->register_gui_component(progress);

	// test menu with scrolling here
	shared_ptr<input_menu> menu(new input_menu());
	menu->set_origin(vec2d(20,1));
	menu->set_z_position(0);
	menu->set_dimensions(vec2d(20,10));
	menu->set_menu_color(FG_WHITE | BG_BLUE, FG_WHITE | BG_BLACK);
	menu->set_description("this is a menu", 1);
	for (int counter = 0; counter < 10; ++counter) {
		char buf[20];
		sprintf(buf, "item%d", counter);
		menu->add_option(buf);
	}
	menu->commit_all();
	controller->register_gui_component(static_pointer_cast<gui_component>(menu));
	controller->register_input_component(static_pointer_cast<input_component>(menu));
	controller->refresh();

	// test regular menu here
	shared_ptr<input_menu> menu2(new input_menu());
	menu2->set_origin(vec2d(1,15));
	menu2->set_z_position(2);
	menu2->set_cursor_color(FG_WHITE | BG_RED);
	menu2->set_dimensions(vec2d(10,10));
	menu2->set_color(FG_WHITE | BG_GREEN);
	menu2->add_option("ok");
	menu2->clear();
	menu2->commit_attributes();
	menu2->render();
	controller->register_gui_component(static_pointer_cast<gui_component>(menu2));
	controller->register_input_component(static_pointer_cast<input_component>(menu2));

	// test input box here
	shared_ptr<input_textbox> box(new input_textbox());
	box->set_origin(vec2d(1, 30));
	box->set_z_position(3);
	box->set_cursor_color(FG_WHITE | BG_RED);
	box->set_dimensions(vec2d(20,3));
	box->set_description("type something",1);
	box->commit_attributes();
	controller->register_gui_component(static_pointer_cast<gui_component>(box));
	controller->register_input_component(static_pointer_cast<input_component>(box));

	// test another input box here
	shared_ptr<input_textbox> box2(new input_textbox());
	box2->set_origin(vec2d(20, 30));
	box2->set_z_position(4);
	box2->set_cursor_color(FG_WHITE | BG_RED);
	box2->set_dimensions(vec2d(20,3));
	box2->set_description("type??",1);
	box2->commit_attributes();
	controller->register_gui_component(static_pointer_cast<gui_component>(box2));
	controller->register_input_component(static_pointer_cast<input_component>(box2));

//	shared_ptr<key_reader> keyboard_reader(new key_reader());
//	controller->register_input_component(keyboard_reader);
//	keyboard_reader->set_enable(true);
//	controller->set_focus(keyboard_reader);
//	while (!keyboard_reader->wait_for_key(1000).is_valid());

//	abort();


	// get input
	box->printf("arh\n");
	box->commit_output();
	string result = box->wait_for_input();
	box->printf("bullshit!\n");
	box->commit_output();
	box->wait_for_input();
	box->printf("ok\n");
	box->commit_output();

	while(1) {
//		menu->commit();
		sleep(1);
		progress->set_value(progress->get_value()+1);
	}

/*
	shared_ptr<gui_component> component2(new gui_component());
	component2->set_z_position(2);
	component2->set_dimensions(vec2d(10,10));
	component2->set_origin(vec2d(2,2));
	component2->set_color(FG_WHITE | BG_RED);
	component2->clear();
	component2->printf("testtt");
	controller->register_gui_component(component2);
	component2->commit();

	sleep(1);
	component2->set_origin(vec2d(3,2));
	component2->commit();
	sleep(1);
	component2 = nullptr;
	component1->set_origin(vec2d(2,2));
	component1->commit();
	sleep(1);
	component1->set_origin(vec2d(1,1));
	component1->commit();
	sleep(1);
*/
	sleep(2);
	controller = nullptr;
	return 0;
}
