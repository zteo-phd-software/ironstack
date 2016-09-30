#include "gui.h"

#ifndef NO_GUI

text_panel::text_panel()
{
//	pthread_mutexattr_t mutex_attributes;

	abs_x_origin = 0;
	abs_y_origin = 0;
	rows = 0;
	columns = 0;
	z_position = display_controller::z_depth_auto_assignment();

	visible = true;
	current_cursor_row = 0;
	current_cursor_column = 0;
	current_color = COLOR_BACKGROUND_BLACK | COLOR_FOREGROUND_WHITE;

	// panel data
	commit_flag = true;
	render_buffer = NULL;

//	pthread_mutexattr_init(&mutex_attributes);
//	pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);
//	pthread_mutex_init(&internal_lock, &mutex_attributes);
//	pthread_mutexattr_destroy(&mutex_attributes);

	pthread_mutex_init(&intermediate_buffer_lock, NULL);
	intermediate_buffer = NULL;
	
	pthread_mutex_init(&output_buffer_lock, NULL);
	output_buffer = NULL;
}

text_panel::~text_panel()
{
	// TODO: this is causing program to crash on exist because unregister panel
	// accesses a static data member that might possibly have been destroyed.
	// Please perform the unregistration somewhere else.
	//unregister_panel();

	if (render_buffer != NULL)
	{
		free(render_buffer);
		free(intermediate_buffer);
		free(output_buffer);
	}

//	pthread_mutex_destroy(&internal_lock);
	pthread_mutex_destroy(&intermediate_buffer_lock);
	pthread_mutex_destroy(&output_buffer_lock);
}

void text_panel::set_panel_origin(int32_t new_x_origin, int32_t new_y_origin)
{
	// trivial check -- don't block up display controller unnecessarily
	if (new_x_origin == abs_x_origin && new_y_origin == abs_y_origin)
		return;
	
	pthread_mutex_lock(&output_buffer_lock);
	pthread_mutex_lock(&intermediate_buffer_lock);
	abs_x_origin = new_x_origin;
	abs_y_origin = new_y_origin;
	pthread_mutex_unlock(&intermediate_buffer_lock);
	pthread_mutex_unlock(&output_buffer_lock);
}

void text_panel::set_panel_dimensions(uint32_t new_x_width, uint32_t new_y_height)
{
	// trivial check -- don't block up display controller unnecessarily
	if (new_x_width == columns && new_y_height == rows)
		return;
	
	pthread_mutex_lock(&output_buffer_lock);
	pthread_mutex_lock(&intermediate_buffer_lock);

	if (render_buffer != NULL)
	{
		free(render_buffer);
		free(intermediate_buffer);
		free(output_buffer);
	}

	columns = new_x_width;
	rows = new_y_height;
		
	// reallocate buffers
	render_buffer = (output_char_t*) cu_alloc(rows*columns*sizeof(output_char_t));
	intermediate_buffer = (output_char_t*) cu_alloc(rows*columns*sizeof(output_char_t));
	output_buffer = (output_char_t*) cu_alloc(rows*columns*sizeof(output_char_t));

	// clear all buffers
	clear_no_commit();
	memcpy(intermediate_buffer, render_buffer, rows*columns*sizeof(output_char_t));
	memcpy(output_buffer, render_buffer, rows*columns*sizeof(output_char_t));

	pthread_mutex_unlock(&intermediate_buffer_lock);
	pthread_mutex_unlock(&output_buffer_lock);
}

void text_panel::set_z_position(int32_t new_z_depth)
{
	//pthread_mutex_lock(&internal_lock);
	z_position = new_z_depth;
	//pthread_mutex_unlock(&internal_lock);
}

void text_panel::set_visibility(bool new_visibility_state)
{
	//pthread_mutex_lock(&internal_lock);
	visible = new_visibility_state;
	//pthread_mutex_unlock(&internal_lock);
}

void text_panel::register_panel()
{
	display_controller::register_output_panel(this);
}

void text_panel::unregister_panel()
{
	display_controller::unregister_output_panel(this);
}

void text_panel::write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...)
{
	char message_buffer[MESSAGE_BUFFER_SIZE];
	va_list args;

	memset(message_buffer, 0, sizeof(message_buffer));
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*)message, args);
	va_end(args);

//	pthread_mutex_lock(&internal_lock);
	current_cursor_row = rel_y_position;
	current_cursor_column = rel_x_position;
	if (current_cursor_row >= rows)
		current_cursor_row = rows-1;
	else if (current_cursor_column >= columns)
		current_cursor_column = columns-1;

	write("%s", message_buffer);
//	pthread_mutex_unlock(&internal_lock);
}

void text_panel::write_no_commit(const void* message, ...)
{
	char message_buffer[MESSAGE_BUFFER_SIZE];
	va_list args;

	memset(message_buffer, 0, sizeof(message_buffer));
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*)message, args);
	va_end(args);

//	pthread_mutex_lock(&internal_lock);
	commit_flag = false;
	write("%s", message_buffer);
//	pthread_mutex_unlock(&internal_lock);
}

void text_panel::write_pos_no_commit(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...)
{
	char message_buffer[MESSAGE_BUFFER_SIZE];
	va_list args;

	memset(message_buffer, 0, sizeof(message_buffer));
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*)message, args);
	va_end(args);

//	pthread_mutex_lock(&internal_lock);
	commit_flag = false;
	write_pos(rel_x_position, rel_y_position, "%s", message_buffer);
//	pthread_mutex_unlock(&internal_lock);
}

void text_panel::move_cursor_absolute(uint32_t rel_x_position, uint32_t rel_y_position)
{
	if (rel_x_position >= columns)
		rel_x_position = columns-1;

	if (rel_y_position >= rows)
		rel_y_position = rows-1;

	current_cursor_row = rel_y_position;
	current_cursor_column = rel_x_position;
}

void text_panel::move_cursor_relative(int32_t x_displacement, int32_t y_displacement)
{
	if ((int32_t) current_cursor_column+x_displacement < 0)
		current_cursor_column = 0;
	else if ((int32_t) current_cursor_column+x_displacement >= (int32_t) columns)
		current_cursor_column = columns-1;
	else
		current_cursor_column += x_displacement;
	
	if ((int32_t) current_cursor_row+y_displacement < 0)
		current_cursor_row = 0;
	else if ((int32_t) current_cursor_row+y_displacement >= (int32_t) rows)
		current_cursor_row = rows-1;
	else
		current_cursor_row += y_displacement;
}

void text_panel::clear()
{
	uint32_t counter;
	
//	pthread_mutex_lock(&internal_lock);
	for (counter = 0; counter < rows*columns; counter++)
	{
		render_buffer[counter].character = ' ';
		render_buffer[counter].composite_color = current_color;
	}
	current_cursor_row = 0;
	current_cursor_column = 0;
	commit_render_buffer();
//	pthread_mutex_unlock(&internal_lock);
}

void text_panel::clear_no_commit()
{
	uint32_t counter;
//	pthread_mutex_lock(&internal_lock);
	for (counter = 0; counter < rows*columns; counter++)
	{
		render_buffer[counter].character = ' ';
		render_buffer[counter].composite_color = current_color;
	}
	current_cursor_row = 0;
	current_cursor_column = 0;
//	pthread_mutex_unlock(&internal_lock);
}

void text_panel::repaint()
{
	display_controller::repaint();
}

void text_panel::scroll_render_buffer(uint32_t rows_to_scroll)
{
	uint32_t counter;

	if (rows_to_scroll == 0)
		return;

	if (rows_to_scroll >= rows)
	{
		// empty out screen if lines to scroll exceeds buffer size
		for (counter = 0; counter < rows*columns; counter++)
		{
			(render_buffer+counter)->character = ' ';
			(render_buffer+counter)->composite_color = current_color;
		}
	}
	else
	{
		// scroll the screen buffer
		for (counter = 0; counter < rows*columns-rows_to_scroll*columns; counter++)
		{
			(render_buffer+counter)->character = (render_buffer+rows_to_scroll*columns+counter)->character;
			(render_buffer+counter)->composite_color = (render_buffer+rows_to_scroll*columns+counter)->composite_color;
		}
		
		// clear to the end of the screen buffer
		for (counter = rows*columns-rows_to_scroll*columns; counter < rows*columns; counter++)
		{
			(render_buffer+counter)->character = ' ';
			(render_buffer+counter)->composite_color = current_color;
		}
	}
}

void text_panel::commit_render_buffer()
{
//	pthread_mutex_lock(&internal_lock);
	pthread_mutex_lock(&intermediate_buffer_lock);
	memcpy(intermediate_buffer, render_buffer, rows*columns*sizeof(output_char_t));
	pthread_mutex_unlock(&intermediate_buffer_lock);
//	pthread_mutex_unlock(&internal_lock);
}

#endif

#ifndef NO_GUI

void textbox_t::write(const void* message, ...)
{
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	va_list args;

	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*) message, args);
	va_end(args);

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);

	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// process newlines or carriage returns
			current_cursor_column = 0;
			
			if (current_cursor_row+1 >= rows)
			{
				scroll_render_buffer(1);
				current_cursor_row = rows-1;
			}
			else
				current_cursor_row++;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > 0)
				current_cursor_column--;
			else if (current_cursor_row > 0)
			{
				current_cursor_column = columns-1;
				current_cursor_row--;
			}
			else
			{
				current_cursor_column = 0;
				current_cursor_row = 0;
			}
		}
		else
		{
			// convert into printables if necessary
#ifdef _WIN32
			if (message_buffer[counter] < 32)
				message_buffer[counter] = ' ';
#endif
#ifndef _WIN32
			if (message_buffer[counter] == 196)
				message_buffer[counter] = '-';
			else if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';
#endif
			

			// write to buffer
			if (current_cursor_column < columns)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
			else if (current_cursor_column >= columns)
			{
				// update coordinates first
				current_cursor_row++;
				current_cursor_column = 0;

				// check for scrolling
				if (current_cursor_row >= rows)
				{
					current_cursor_row = rows-1;
					scroll_render_buffer(1);
				}

				// write into buffer and update coordinates
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
		}
	}

	// exit critical region
	if (commit_flag)
		commit_render_buffer();
	else
		commit_flag = true;
}

#endif





#ifndef NO_GUI

input_panel::input_panel()
{
	registered = false;
	focus_requires_cursor = true;

	accepts_keypress = false;
	accepts_mouse = false;

	is_modal = false;
	wants_focus = false;
}

input_panel::~input_panel()
{

}

void input_panel::set_visibility(bool new_visibility_state)
{
	display_controller::signal_list_change();
	text_panel::set_visibility(new_visibility_state);
}

void input_panel::register_panel()
{
	text_panel::register_panel();
	display_controller::register_input_panel(this);
	registered = true;
}

void input_panel::unregister_panel()
{
	display_controller::unregister_input_panel(this);
	text_panel::unregister_panel(); 
	registered = false;
}

void input_panel::set_modal(bool state)
{
	is_modal = state;
}

void input_panel::set_focus()
{
	wants_focus = true;
	display_controller::update_focus_on_list_change();
}

#endif

#ifndef NO_GUI

enhanced_textbox_t::enhanced_textbox_t()
{
	display_timestamp = false;
	divider_position = (2*columns)/3;
	message_id_counter = 0;
	translation_table.clear();
}

enhanced_textbox_t::~enhanced_textbox_t()
{

}

void enhanced_textbox_t::write(const void* message, ...)
{
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	uint32_t message_offset = 0;
	uint32_t left_offset_correction = 0;
	time_t current_time;
	va_list args;

	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));
	if (display_timestamp)
	{
		current_time = time(NULL);
		strftime(message_buffer, sizeof(message_buffer), "[%H:%M:%S] ", localtime(&current_time));
		left_offset_correction = strlen(message_buffer);
		message_offset = left_offset_correction;
	}

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer+message_offset, sizeof(message_buffer), (char*) message, args);
	va_end(args);

	// cursor safety
	if (display_timestamp && current_cursor_column != left_offset_correction)
	{
		current_cursor_column = 0;
		current_cursor_row++;
		if (current_cursor_row >= rows)
		{
			scroll_render_buffer(1);
			current_cursor_row = rows-1;
			current_cursor_column = 0;
		}
	}
	else if (display_timestamp && current_cursor_column == left_offset_correction)
		current_cursor_column = 0;

	/*
	if (columns > 0 && rows > 0)
	{
		if (current_cursor_column >= columns)
			current_cursor_column = columns-1;
		if (current_cursor_row >= rows)
			current_cursor_row = rows-1;
	}
	*/

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);

	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// process newlines or carriage returns
			current_cursor_column = left_offset_correction;
			
			if (current_cursor_row+1 >= rows)
			{
				scroll_render_buffer(1);
				current_cursor_row = rows-1;
			}
			else
				current_cursor_row++;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > left_offset_correction)
				current_cursor_column--;
			else if (current_cursor_row > 0)
			{
				current_cursor_column = columns-1;
				current_cursor_row--;
			}
			else
			{
				current_cursor_column = left_offset_correction;
				current_cursor_row = 0;
			}
		}
		else
		{
			// convert into printables if necessary
			if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';

			// write to buffer
			if (current_cursor_column < columns)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
			else if (current_cursor_column >= columns)
			{
				// update coordinates first
				current_cursor_row++;
				current_cursor_column = left_offset_correction;

				// check for scrolling
				if (current_cursor_row >= rows)
				{
					current_cursor_row = rows-1;
					scroll_render_buffer(1);
				}

				// write into buffer and update coordinates
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
		}
	}

	// exit critical region
	if (commit_flag)
		commit_render_buffer();
	else
		commit_flag = true;
//	pthread_mutex_unlock(&internal_lock);
}

uint32_t enhanced_textbox_t::write_lhs(const void* message, ...)
{
	uint32_t return_value = ++message_id_counter;
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	uint32_t message_offset = 0;
	time_t current_time;
	va_list args;
	std::pair<uint32_t, uint32_t> id_to_row_pair;

	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));
	if (display_timestamp)
	{
		current_time = time(NULL);
		strftime(message_buffer, sizeof(message_buffer), "[%H:%M:%S] ", localtime(&current_time));
		message_offset = strlen(message_buffer);
	}

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer+message_offset, sizeof(message_buffer)-message_offset, (char*) message, args);
	va_end(args);

	// cursor safety
	if (columns > 0 && rows > 0)
	{
		if (current_cursor_column > 0)
		{
			current_cursor_column = 0;
			current_cursor_row++;
		}

		if (current_cursor_row >= rows)
		{
			scroll_render_buffer(1);
			current_cursor_row = rows-1;
		}
	}

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);

	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// process newlines or carriage returns
			current_cursor_column = 0;
			
			if (current_cursor_row+1 >= rows)
			{
				scroll_render_buffer(1);
				current_cursor_row = rows-1;
			}
			else
				current_cursor_row++;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > 0)
				current_cursor_column--;
			else if (current_cursor_row > 0)
			{
				current_cursor_column = divider_position-1;
				current_cursor_row--;
			}
			else
			{
				current_cursor_column = 0;
				current_cursor_row = 0;
			}
		}
		else
		{
			// convert into printables if necessary
			if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';

			// write to buffer
			if (current_cursor_column < divider_position)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
			else if (current_cursor_column >= divider_position)
			{
				// update coordinates first
				current_cursor_row++;
				current_cursor_column = 0;

				// check for scrolling
				if (current_cursor_row >= rows)
				{
					current_cursor_row = rows-1;
					scroll_render_buffer(1);
				}

				// write into buffer and update coordinates
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
		}
	}

	// draw the colon at the end of the line
	write_offset = current_cursor_row*columns+divider_position;
	(render_buffer+write_offset)->character = ':';
	(render_buffer+write_offset)->composite_color = current_color;

	// create table translation
	id_to_row_pair.first = return_value;
	id_to_row_pair.second = current_cursor_row;
	translation_table.push_back(id_to_row_pair);

	// scroll to next line
	current_cursor_column = 0;
	current_cursor_row++;

	if (current_cursor_row >= rows)
	{
		scroll_render_buffer(1);
		current_cursor_row = rows-1;
	}

	// exit critical region
	if (commit_flag)
		commit_render_buffer();
	else
		commit_flag = true;

//	pthread_mutex_unlock(&internal_lock);

	return return_value;
}

void enhanced_textbox_t::write_rhs(uint32_t lhs_id, const void* message, ...)
{
	std::list<std::pair<uint32_t, uint32_t> >::const_iterator translation_iterator;
	bool found = false;
	uint32_t target_row = 0;
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	uint32_t backup_cursor_column, backup_cursor_row;
	va_list args;
	
	// search for id first
//	pthread_mutex_lock(&internal_lock);
	for (translation_iterator = translation_table.begin(); translation_iterator != translation_table.end(); translation_iterator++)
	{
		if (translation_iterator->first == lhs_id)
		{
			found = true;
			target_row = translation_iterator->second;
			break;
		}
	}

	if (!found)
		return;

	// id found, now perform printing
	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*) message, args);
	va_end(args);

	// set cursor location
	backup_cursor_row = current_cursor_row;
	backup_cursor_column = current_cursor_column;

	current_cursor_row = target_row;
	current_cursor_column = divider_position+2;

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);

	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// restrict output to only one line -- ignore newlines and carriage returns
			break;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > divider_position+2)
				current_cursor_column--;
		}
		else
		{
			// convert into printables if necessary
			if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';

			// write to buffer
			if (current_cursor_column < columns)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
			else
				break;
		}
	}

	current_cursor_row = backup_cursor_row;
	current_cursor_column = backup_cursor_column;

	// exit critical region
	if (commit_flag)
		commit_render_buffer();
	else
		commit_flag = true;

//	pthread_mutex_unlock(&internal_lock);
}

void enhanced_textbox_t::set_divider_position(uint32_t divider_position_in)
{
	if (divider_position_in < 1 || divider_position_in >= columns-1)
		assert(0);

	divider_position = divider_position_in;
}

void enhanced_textbox_t::scroll_render_buffer(uint32_t rows_to_scroll)
{
	// scroll all translation tables
	std::list<std::pair<uint32_t, uint32_t> >::iterator translation_iterator;

	translation_iterator = translation_table.begin();
	while (translation_iterator != translation_table.end())
	{
		if (rows_to_scroll > translation_iterator->second)
			translation_iterator = translation_table.erase(translation_iterator);
		else
		{
			translation_iterator->second -= rows_to_scroll;
			translation_iterator++;
		}
	}	

	// use text panel's built in scroll function
	text_panel::scroll_render_buffer(rows_to_scroll);
}

#endif

#ifndef NO_GUI

constrained_textbox_t::constrained_textbox_t()
{
	unwriteable = false;
}

constrained_textbox_t::constrained_textbox_t(const constrained_textbox_t& original)
{
	unwriteable = original.unwriteable;
}

constrained_textbox_t& constrained_textbox_t::operator=(const constrained_textbox_t& original)
{
	unwriteable = original.unwriteable;
	return *this;
}

constrained_textbox_t::~constrained_textbox_t()
{

}

void constrained_textbox_t::write(const void* message, ...)
{
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	va_list args;
	bool modified = false;

	if (current_cursor_row == 0 && current_cursor_column == 0)
		unwriteable = false;

	if (unwriteable)
		return;

	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*) message, args);
	va_end(args);

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);
	
//	pthread_mutex_lock(&internal_lock);
	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// process newlines or carriage returns
			current_cursor_column = 0;
			
			if (current_cursor_row+1 >= rows)
			{
				unwriteable = true;
				break;
			}
			else
				current_cursor_row++;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > 0)
				current_cursor_column--;
			else if (current_cursor_row > 0)
			{
				current_cursor_column = columns-1;
				current_cursor_row--;
			}
			else
			{
				current_cursor_column = 0;
				current_cursor_row = 0;
			}
		}
		else
		{
			// convert into printables if necessary
			if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';

			// write to buffer
			if (current_cursor_column < columns)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
				modified = true;
			}
			else if (current_cursor_column >= columns)
			{
				// update coordinates first
				current_cursor_row++;
				current_cursor_column = 0;

				// check for scrolling
				if (current_cursor_row >= rows)
				{
					unwriteable = true;
					break;
				}

				// write into buffer and update coordinates
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
				modified = true;
			}
		}
	}

	// exit critical region
	if (modified)
	{
		if (commit_flag)
			commit_render_buffer();
		else
			commit_flag = true;
	}

//	pthread_mutex_unlock(&internal_lock);
}

void constrained_textbox_t::write(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...)
{
	uint32_t counter;
	uint32_t bytes_to_print;
	char message_buffer[MESSAGE_BUFFER_SIZE];
	uint32_t write_offset;
	va_list args;
	bool modified = false;

//	pthread_mutex_lock(&internal_lock);
	if (current_cursor_row == 0 && current_cursor_column == 0)
		unwriteable = false;

	current_cursor_column = rel_x_position;
	current_cursor_row = rel_y_position;
	if (columns > 0 && rows > 0)
	{
		if (rel_x_position >= columns)
			rel_x_position = columns-1;
		if (rel_y_position >= rows)
			unwriteable = true;
	}

	if (unwriteable)
	{
//		pthread_mutex_unlock(&internal_lock);
		return;
	}

	// blank out buffer
	memset(message_buffer, 0, sizeof(message_buffer));

	// generate the message into a contiguous buffer
	va_start(args, message);
	vsnprintf(message_buffer, sizeof(message_buffer), (char*) message, args);
	va_end(args);

	// collect metadata
	bytes_to_print = (uint32_t) strlen(message_buffer);

	for (counter = 0; counter < bytes_to_print; counter++)
	{
		if (message_buffer[counter] == '\n' || message_buffer[counter] == '\r')
		{
			// process newlines or carriage returns
			current_cursor_column = 0;
			
			if (current_cursor_row+1 >= rows)
			{
				unwriteable = true;
				break;
			}
			else
				current_cursor_row++;
		}
		else if (message_buffer[counter] == '\b')
		{
			// process backspace
			if (current_cursor_column > 0)
				current_cursor_column--;
			else if (current_cursor_row > 0)
			{
				current_cursor_column = columns-1;
				current_cursor_row--;
			}
			else
			{
				current_cursor_column = 0;
				current_cursor_row = 0;
			}
		}
		else
		{
			// convert into printables if necessary
			if (message_buffer[counter] < 32 || message_buffer[counter] > 127)
				message_buffer[counter] = ' ';

			// write to buffer
			if (current_cursor_column < columns)
			{
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
			}
			else if (current_cursor_column >= columns)
			{
				// update coordinates first
				current_cursor_row++;
				current_cursor_column = 0;

				// check for scrolling
				if (current_cursor_row >= rows)
				{
					unwriteable = true;
					break;
				}

				// write into buffer and update coordinates
				write_offset = current_cursor_row*columns+current_cursor_column;
				(render_buffer+write_offset)->character = message_buffer[counter];
				(render_buffer+write_offset)->composite_color = current_color;
				current_cursor_column++;
				modified = true;
			}
		}
	}

	// exit critical region
	if (modified)
	{
		if (commit_flag)
			commit_render_buffer();
		else
			commit_flag = true;
	}

//	pthread_mutex_unlock(&internal_lock);
}
#endif

#ifndef NO_GUI

progress_bar_t::progress_bar_t()
{
	display_numerics = false;
	display_percentage = true;
	raw_value_field_width = 7;
	max_progress_value = 100.0;
	current_value = 0.0;
	progress_bar_fill_char = ' ';
	progress_bar_fill_color = COLOR_FOREGROUND_BLACK | COLOR_BACKGROUND_WHITE;
	progress_bar_background_color = COLOR_FOREGROUND_WHITE | COLOR_BACKGROUND_BLACK;
	progress_bar_text_color = COLOR_FOREGROUND_WHITE | COLOR_BACKGROUND_BLACK;
}

void progress_bar_t::render()
{
	uint32_t progress_bar_cols = columns;
	uint32_t filled_cols;
	uint32_t unfilled_cols;
	uint32_t counter;
	uint32_t rows_to_fill = rows;
	uint32_t middle_row = rows/2;
	uint32_t small_buffer_length;
	char small_buffer[128];
	
	if (display_numerics)
	{
		if (display_percentage)
		{
			if (progress_bar_cols >= 7)
				progress_bar_cols -= 7;
			else
			{
				// can't display
				return;
			}
		}
		else
		{
			if (progress_bar_cols >= raw_value_field_width)
				progress_bar_cols -= raw_value_field_width;
			else
			{
				// can't display
				return;
			}
		}
	}
	
	// begin render sequence
	clear_no_commit();
	filled_cols = (uint32_t) ((current_value/max_progress_value)* (double)progress_bar_cols);
	unfilled_cols = progress_bar_cols - filled_cols;
	
	while(rows_to_fill > 0)
	{
		set_color(progress_bar_fill_color);
		for (counter = 0; counter < filled_cols; counter++)
			write_no_commit("%c", progress_bar_fill_char);

		set_color(progress_bar_background_color);
		for (counter = 0; counter < unfilled_cols; counter++)
			write_no_commit(" ");

		if (current_cursor_row == middle_row)
		{
			if (display_numerics && display_percentage)
			{
				memset(small_buffer, 0, sizeof(small_buffer));
				sprintf(small_buffer, "%3.2f%%", (double) current_value * 100.0 / (double)max_progress_value);
				small_buffer_length = (uint32_t) strlen(small_buffer);
				set_color(progress_bar_text_color);

				for (counter = 0; counter < 7-small_buffer_length; counter++)
					write_no_commit(" ");
				write_no_commit("%s", small_buffer);
			}
			else if (display_numerics)
			{
				memset(small_buffer, 0, sizeof(small_buffer));
				sprintf(small_buffer, "%d", (int) current_value);
				if (raw_value_field_width > 0)
					small_buffer[raw_value_field_width] = '\0';
				else
					small_buffer[0] = '\0';
				write_no_commit("%s", small_buffer);
			}
		}

		rows_to_fill--;
		if (rows_to_fill > 0)
			write("\n");
	}

	commit_render_buffer();
}

#endif

#ifndef NO_GUI

input_textbox_t::input_textbox_t()
{
	pthread_mutex_init(&keypress_lock, NULL);
	pthread_cond_init(&keypress_cond, NULL);
}

input_textbox_t::~input_textbox_t()
{
	// TODO: this is causing program to crash on exist because unregister_panel
	// accesses a static data member that might possibly have been destroyed.
	// Please perform the unregistration somewhere else.
	//display_controller::unregister_input_panel(this);
	//display_controller::unregister_output_panel(this);
	pthread_mutex_destroy(&keypress_lock);
	pthread_cond_destroy(&keypress_cond);
	display_controller::repaint();
}

void input_textbox_t::get_keypress(uint8_t* ascii_value, uint16_t* special_value)
{
	std::pair<uint8_t, uint16_t> current_key_tuple;

	set_keypress_processing(true);
	display_controller::signal_list_change();
	repaint();

	pthread_mutex_lock(&keypress_lock);
	while(1)
	{
		while(keypress_buffer.size() == 0)
			pthread_cond_wait(&keypress_cond, &keypress_lock);
		
		current_key_tuple = keypress_buffer.front();
		keypress_buffer.pop_front();
		break;
	}
	pthread_mutex_unlock(&keypress_lock);

	set_keypress_processing(false);
	display_controller::signal_list_change();

	// output to screen
	if (current_key_tuple.first >= ' ' && current_key_tuple.first <= '~')
		write("%c", current_key_tuple.first);
	repaint();

	if (ascii_value != NULL)
		*ascii_value = current_key_tuple.first;

	if (special_value != NULL)
		*special_value = current_key_tuple.second;
}

void input_textbox_t::get_line(uint8_t* input_buffer, uint32_t input_buffer_size)
{
	uint32_t write_offset = 0;
	std::pair<uint8_t, uint16_t> current_key_tuple;
	uint8_t ascii_value;
//	uint16_t special_value;
	output_char_t* backup_buffer = NULL;
	uint32_t initial_column = current_cursor_column;
	uint32_t initial_row = current_cursor_row;

	// backup input buffer
	backup_buffer = (output_char_t*) cu_alloc(rows*columns*sizeof(output_char_t));
	memcpy(backup_buffer, render_buffer, rows*columns*sizeof(output_char_t));

	// reset input buffer and turn on keypress processing
	memset(input_buffer, 0, input_buffer_size);
	set_keypress_processing(true);
	display_controller::signal_list_change();
	repaint();

	pthread_mutex_lock(&keypress_lock);
	while(1)
	{
		// grab keypress
		while(keypress_buffer.size() == 0)
			pthread_cond_wait(&keypress_cond, &keypress_lock);
		current_key_tuple = keypress_buffer.front();
		keypress_buffer.pop_front();
		ascii_value = current_key_tuple.first;
//		special_value = current_key_tuple.second;

		// process keypress
		if (ascii_value >= ' ' && ascii_value <= '~')
		{
			// normal keypress
			if (write_offset < input_buffer_size-1)
				input_buffer[write_offset++] = ascii_value;
			else
				continue;
		}
		else if (ascii_value == '\n' || ascii_value == '\r')
		{
			// enter
			break;
		}
		else if (ascii_value == '\b')
		{
			// backspace
			if (write_offset == 0)
				continue;
			else
				input_buffer[--write_offset] = '\0';
		}
		else if (ascii_value == '\t')
			display_controller::update_focus_on_tab();
		else
		{
			//printf("%d\n", special_value);
			continue;
		}

		clear_no_commit();
		memcpy(render_buffer, backup_buffer, rows*columns*sizeof(output_char_t));
		current_cursor_column = initial_column;
		current_cursor_row = initial_row;
		write("%s", input_buffer);
		repaint();
	}
	pthread_mutex_unlock(&keypress_lock);

	clear_no_commit();
	memcpy(render_buffer, backup_buffer, rows*columns*sizeof(output_char_t));
	current_cursor_column = initial_column;
	current_cursor_row = initial_row;
	write("%s\n", input_buffer);
	repaint();

	set_keypress_processing(false);
	display_controller::signal_list_change();
	repaint();

	free(backup_buffer);
}

void input_textbox_t::process_keypress(uint8_t ascii_value, uint16_t special_value)
{
	std::pair<uint8_t, uint16_t> keypress_tuple;

	keypress_tuple.first = ascii_value;
	keypress_tuple.second = special_value;
	pthread_mutex_lock(&keypress_lock);
	keypress_buffer.push_back(keypress_tuple);
	pthread_cond_broadcast(&keypress_cond);
	pthread_mutex_unlock(&keypress_lock);
}

#endif

#ifndef NO_GUI

input_menu_t::input_menu_t()
{
	number_of_items = 0;
	menu_items = NULL;

	selector_color = COLOR_BACKGROUND_LIGHT_GREEN | COLOR_FOREGROUND_BRIGHT_WHITE;
	previous_selection = 0;
	menu_render_row = 0;
	backup_buffer = NULL;

	pthread_mutex_init(&keypress_lock, NULL);
	pthread_cond_init(&keypress_cond, NULL);
}

input_menu_t::~input_menu_t()
{
	uint32_t counter;

	display_controller::unregister_input_panel(this);
	display_controller::unregister_output_panel(this);

	if (menu_items != NULL)
	{
		for (counter = 0; counter < number_of_items; counter++)
			free(menu_items[counter]);
		free(menu_items);
	}

	if (backup_buffer != NULL)
		free(backup_buffer);

	pthread_mutex_destroy(&keypress_lock);
	pthread_cond_destroy(&keypress_cond);
	display_controller::repaint();
}

void input_menu_t::set_selector_color(uint8_t selector_color_in)
{
	selector_color = selector_color_in;
}

void input_menu_t::add_menu_item(const void* item_descriptor)
{
	char* descriptor = NULL;
	uint32_t number_of_bytes = strlen((const char*)item_descriptor)+1;

	descriptor = (char*)malloc(number_of_bytes);
	strcpy(descriptor, (const char*)item_descriptor);

	number_of_items++;
	menu_items = (char**) realloc(menu_items, number_of_items*sizeof(char*));
	if (menu_items == NULL)
		assert(0);

	menu_items[number_of_items-1] = descriptor;
}

uint32_t input_menu_t::get_menu_input()
{
	std::pair<uint8_t, uint16_t> current_key_tuple;
	uint8_t ascii_value;
	uint16_t special_value;
	
	// backup renderable data
	menu_render_row = current_cursor_row;
	if (backup_buffer != NULL)
		free(backup_buffer);
	backup_buffer = (output_char_t*) cu_alloc(rows*columns*sizeof(output_char_t));
	memcpy(backup_buffer, render_buffer, rows*columns*sizeof(output_char_t));

	// render menu items
	off_focus();

	// enable keypress detection
	set_cursor_on_focus(false);
	set_keypress_processing(true);
	display_controller::signal_list_change();

	// do trivial test
	if (number_of_items == 0)
		return 0;

	pthread_mutex_lock(&keypress_lock);
	while(1)
	{
		// get the keypress
		while(keypress_buffer.size() == 0)
			pthread_cond_wait(&keypress_cond, &keypress_lock);
		current_key_tuple = keypress_buffer.front();
		keypress_buffer.pop_front();
		ascii_value = current_key_tuple.first;
		special_value = current_key_tuple.second;

		// process the keypress
		if (special_value == 38)
		{
			// up
			if (previous_selection == 0)
				continue;
			else
				previous_selection--;
		}
		else if (special_value == 40)
		{
			// down
			if (previous_selection == number_of_items-1)
				continue;
			else
				previous_selection++;

		}
		else if (ascii_value == '\n' || ascii_value == '\r')
			break;
		else if (ascii_value == '\t')
		{
			display_controller::update_focus_on_tab();
			continue;
		}
		else
			continue;

		// render
		on_focus();
	}

	pthread_mutex_unlock(&keypress_lock);
	set_keypress_processing(false);
	display_controller::signal_list_change();
	repaint();

	free(backup_buffer);
	backup_buffer = NULL;

	return previous_selection;
}

// buffer incoming keypresses
void input_menu_t::process_keypress(uint8_t ascii_value, uint16_t special_value)
{
	std::pair<uint8_t, uint16_t> keypress_tuple;

	keypress_tuple.first = ascii_value;
	keypress_tuple.second = special_value;
	pthread_mutex_lock(&keypress_lock);
	keypress_buffer.push_back(keypress_tuple);
	pthread_cond_broadcast(&keypress_cond);
	pthread_mutex_unlock(&keypress_lock);
}

void input_menu_t::on_focus()
{
	uint32_t current_cursor_y;
	uint32_t string_length;
	uint32_t counter, counter2;

	// render menu items to buffer first
	memcpy(render_buffer, backup_buffer, rows*columns*sizeof(output_char_t));
	current_cursor_y = menu_render_row;
	for (counter = 0; counter < number_of_items; counter++)
	{
		// render to middle
		string_length = strlen(menu_items[counter]);
		write_pos_no_commit((columns-string_length)/2, current_cursor_y, "%s", menu_items[counter]);
		
		// add selection if necessary
		if (counter == previous_selection)
		{
			for (counter2 = 0; counter2 < columns; counter2++)
				render_buffer[current_cursor_y*columns+counter2].composite_color = selector_color;
		}

		current_cursor_y++;
	}

	commit_render_buffer();
	repaint();
}

void input_menu_t::off_focus()
{
	uint32_t current_cursor_y;
	uint32_t string_length;
	uint32_t counter;

	// render menu items to buffer first
	memcpy(render_buffer, backup_buffer, rows*columns*sizeof(output_char_t));
	current_cursor_y = menu_render_row;
	for (counter = 0; counter < number_of_items; counter++)
	{
		// render to middle
		string_length = strlen(menu_items[counter]);
		write_pos_no_commit((columns-string_length)/2, current_cursor_y, "%s", menu_items[counter]);
		current_cursor_y++;
	}

	commit_render_buffer();
	repaint();

}

#endif

#ifndef NO_GUI

// function prototypes for internal sort
bool z_sort(text_panel* panel1, text_panel* panel2);

// initialize static fields
pthread_mutex_t display_controller::display_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef _WIN32
HANDLE display_controller::hIn = NULL;
HANDLE display_controller::hOut = NULL;
HANDLE display_controller::hError = NULL;
#endif

uint32_t display_controller::screen_rows = 0;
uint32_t display_controller::screen_columns = 0;
output_char_t* display_controller::render_buffer = NULL;
output_char_t* display_controller::output_buffer = NULL;

int32_t display_controller::z_depth_counter = 100;
pthread_mutex_t display_controller::panel_list_lock = PTHREAD_MUTEX_INITIALIZER;
std::list<text_panel*> display_controller::output_text_panels = std::list<text_panel*>();
textbox_t display_controller::background_textbox;
pthread_mutex_t display_controller::input_panel_list_lock = PTHREAD_MUTEX_INITIALIZER;
std::list<input_panel*> display_controller::input_panel_list;
input_panel* display_controller::current_focus = NULL;

pthread_mutex_t display_controller::input_event_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t display_controller::input_event_cond = PTHREAD_COND_INITIALIZER;
bool display_controller::input_panel_list_changed = false;
std::list<input_event_t> display_controller::input_event_list = std::list<input_event_t>();

struct timeval dummy_struct_timeval;
const double display_controller::DISPLAY_REFRESH_INTERVAL = 0.4;
struct timeval display_controller::last_refresh_time = dummy_struct_timeval;
bool display_controller::refresh_signalled = false;
pthread_mutex_t display_controller::refresh_signal_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t display_controller::refresh_signal_cond = PTHREAD_COND_INITIALIZER;

// inits all handles, set console title and create input/auto refresh threads
void display_controller::startup()
{
	pthread_t tid, tid2, tid3, tid4;

	int screen_cols = 80;
	int screen_rows = 24;

#ifdef _WIN32
	// windows code
	// get standard handles
	hIn = GetStdHandle(STD_INPUT_HANDLE);
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hError = GetStdHandle(STD_ERROR_HANDLE);
#endif

#ifndef _WIN32
	#ifdef TIOCGSIZE
	struct ttysize ts;
	ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
	screen_cols = ts.ts_cols;
	screen_rows = ts.ts_lines;
	#elif defined(TIOCGWINSZ)
	struct winsize ts;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
	screen_cols = ts.ws_col;
	screen_rows = ts.ws_row;
	#endif 	

	initscr();
	start_color();
	setup_colors();
	keypad(stdscr, TRUE);
	noecho();
#endif

	// set window title and console window size
	set_screen_title("Untitled");
	set_screen_resolution(screen_rows, screen_cols);
	hide_system_cursor();

	//pthread_mutexattr_t mutex_attributes;
	//pthread_mutexattr_init(&mutex_attributes);
	//pthread_mutexattr_settype(&mutex_attributes, PTHREAD_MUTEX_RECURSIVE);
	//pthread_mutex_init(&input_panel_list_lock, &mutex_attributes);
	//pthread_mutexattr_destroy(&mutex_attributes);

	pthread_create(&tid, NULL, periodic_refresh_thread, NULL);
	pthread_create(&tid2, NULL, input_reader_thread, NULL);
	pthread_create(&tid3, NULL, focus_arbitration_thread, NULL);
	pthread_create(&tid4, NULL, repaint_thread, NULL);
}

void display_controller::shutdown()
{
#ifndef _WIN32
	endwin();
#endif
}

void display_controller::set_screen_resolution(uint32_t max_rows, uint32_t max_columns)
{
	uint32_t counter;

	pthread_mutex_lock(&display_lock);
	if (max_rows == screen_rows && max_columns == screen_columns)
	{
		pthread_mutex_unlock(&display_lock);
		return;
	}

	screen_rows = max_rows;
	screen_columns = max_columns;

#ifdef _WIN32
	COORD new_size;
	SMALL_RECT display_area;


	// set screen buffering
	new_size.X = screen_columns;
	new_size.Y = screen_rows;
	SetConsoleScreenBufferSize(hOut, new_size);

	// set screen size
	display_area.Bottom = screen_rows-1;
	display_area.Top = 0;
	display_area.Left = 0;
	display_area.Right = screen_columns-1;
	SetConsoleWindowInfo(hOut, TRUE, &display_area);
#endif

#ifndef _WIN32
	int term_rows, term_columns;

	// set terminal size
	getmaxyx(stdscr, term_rows, term_columns);
	if (resizeterm(screen_rows, screen_columns) == ERR)
	{
		endwin();
		printf("dimensions of screen is too small! needed: %d rows, %d columns.\n", screen_rows, screen_columns);
		printf("available: %d rows, %d columns.\n", term_rows, term_columns);
		exit(0);
	}

	// set colors
	if (COLOR_PAIRS < 64)
	{
		endwin();
		printf("insufficient color palette capacity on system!\n");
		exit(0);
	}
	setup_colors();

#endif

	// reallocate render and output buffers
	screen_rows = max_rows;
	screen_columns = max_columns;

	if (render_buffer != NULL)
	{
		free(render_buffer);
		free(output_buffer);
	}

	render_buffer = (output_char_t*) cu_alloc(screen_rows*screen_columns*sizeof(output_char_t));
	output_buffer = (output_char_t*) cu_alloc(screen_rows*screen_columns*sizeof(output_char_t));

	// clear render and output buffers
	for (counter = 0; counter < screen_rows*screen_columns; counter++)
	{
		render_buffer[counter].character = ' ';
		render_buffer[counter].composite_color = COLOR_BACKGROUND_BLACK | COLOR_FOREGROUND_WHITE;
		
		output_buffer[counter].character = ' ';
		output_buffer[counter].composite_color = COLOR_BACKGROUND_BLACK | COLOR_FOREGROUND_WHITE;
	}

	// reset size of background box and clear it
	background_textbox.set_panel_dimensions(screen_columns, screen_rows);
	background_textbox.clear();

	pthread_mutex_unlock(&display_lock);
}

void display_controller::get_screen_resolution(uint32_t* max_rows, uint32_t* max_columns)
{
	pthread_mutex_lock(&display_lock);
	*max_rows = screen_rows;
	*max_columns = screen_columns;
	pthread_mutex_unlock(&display_lock);
}

void display_controller::set_screen_title(const void* title)
{
	// not available in UNIX
	
#ifdef _WIN32
	SetConsoleTitle((char*)title);
#endif
}

void display_controller::register_output_panel(text_panel* panel)
{
	pthread_mutex_lock(&panel_list_lock);
	output_text_panels.push_back(panel);
	pthread_mutex_unlock(&panel_list_lock);
}

void display_controller::unregister_output_panel(text_panel* panel)
{
	std::list<text_panel*>::iterator panel_list_iterator;
	pthread_mutex_lock(&panel_list_lock);
	for (panel_list_iterator = output_text_panels.begin(); panel_list_iterator != output_text_panels.end(); panel_list_iterator++)
	{
		if (*panel_list_iterator == panel)
		{
			output_text_panels.erase(panel_list_iterator);
			break;
		}
	}
	pthread_mutex_unlock(&panel_list_lock);
}

void display_controller::register_input_panel(input_panel* target)
{
	pthread_mutex_lock(&input_panel_list_lock);
	input_panel_list.push_back(target);
	pthread_mutex_unlock(&input_panel_list_lock);
	update_focus_on_list_change();
}

void display_controller::unregister_input_panel(input_panel* target)
{
	std::list<input_panel*>::iterator input_panel_list_iterator;
	bool found = false;

	pthread_mutex_lock(&input_panel_list_lock);
	for (input_panel_list_iterator = input_panel_list.begin(); input_panel_list_iterator != input_panel_list.end(); input_panel_list_iterator++)
	{
		if (*input_panel_list_iterator == target)
		{
			found = true;
			input_panel_list.erase(input_panel_list_iterator);
			break;
		}
	}
	pthread_mutex_unlock(&input_panel_list_lock);
	if (found)
		update_focus_on_list_change();
}

int32_t display_controller::z_depth_auto_assignment()
{
	return z_depth_counter++;
}

void display_controller::set_color(uint8_t composite_color) {
	background_textbox.set_color(composite_color);
}

void display_controller::write(const void* message, ...)
{
	char write_buffer[1024];
	va_list args;

	memset(write_buffer, 0, sizeof(write_buffer));
	va_start(args, message);
	vsnprintf(write_buffer, sizeof(write_buffer), (char*) message, args);
	va_end(args);

	pthread_mutex_lock(&display_lock);
	background_textbox.write(write_buffer);
	pthread_mutex_unlock(&display_lock);
}

void display_controller::write_pos(uint32_t rel_x_position, uint32_t rel_y_position, const void* message, ...)
{
	char write_buffer[1024];
	va_list args;

	memset(write_buffer, 0, sizeof(write_buffer));
	va_start(args, message);
	vsnprintf(write_buffer, sizeof(write_buffer), (char*) message, args);
	va_end(args);

	pthread_mutex_lock(&display_lock);
	background_textbox.write_pos(rel_x_position, rel_y_position, write_buffer);
	pthread_mutex_unlock(&display_lock);
}

void display_controller::repaint()
{
	pthread_mutex_lock(&refresh_signal_lock);
	refresh_signalled = true;
	pthread_cond_broadcast(&refresh_signal_cond);
	pthread_mutex_unlock(&refresh_signal_lock);
}

void display_controller::signal_list_change()
{
	pthread_mutex_lock(&input_event_lock);
	input_panel_list_changed = true;
	pthread_cond_broadcast(&input_event_cond);
	pthread_mutex_unlock(&input_event_lock);
}

void display_controller::hide_system_cursor()
{
#ifdef _WIN32
	CONSOLE_CURSOR_INFO cursor_info;
	cursor_info.dwSize = 15;
	cursor_info.bVisible = FALSE;
	SetConsoleCursorInfo(hOut, &cursor_info);
#endif

#ifndef _WIN32
	curs_set(0);
#endif
}

void* display_controller::periodic_refresh_thread(void* arguments)
{
	pthread_detach(pthread_self());
	while(1)
	{
		cu_improved_sleep(DISPLAY_REFRESH_INTERVAL);

		pthread_mutex_lock(&refresh_signal_lock);
		refresh_signalled = true;
		pthread_cond_broadcast(&refresh_signal_cond);
		pthread_mutex_unlock(&refresh_signal_lock);

//		pthread_mutex_lock(&display_lock);
//		gettimeofday(&current_time, NULL);
//		pthread_mutex_unlock(&display_lock);

//		if (cu_calculate_time_difference(current_time, last_refresh_time) >= DISPLAY_REFRESH_INTERVAL -0.1)
//			repaint();
	}
	return NULL;
}

// this thread does nothing else except read mouse/keyboard events and enqueue them
void* display_controller::input_reader_thread(void* arguments)
{
#ifdef _WIN32

	INPUT_RECORD input_record;
	DWORD events_read;
	input_event_t current_event;
//	uint32_t last_mouse_x = 0;
//	uint32_t last_mouse_y = 0;
//	uint32_t current_mouse_x, current_mouse_y;
//	bool mouse_down_registered;
//	uint32_t mouse_down_x = 0;
//	uint32_t mouse_down_y = 0;

	pthread_detach(pthread_self());

	while(1)
	{
		// read input
		ReadConsoleInput(hIn, &input_record, 1, &events_read);

		// universal: mouse coordinates are always supplied
		current_event.clear();
		current_event.abs_x_coordinates = input_record.Event.MouseEvent.dwMousePosition.X;
		current_event.abs_y_coordinates = input_record.Event.MouseEvent.dwMousePosition.Y;

		if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
		{
			// describe key event
			current_event.keyboard_event = true;
			current_event.ascii_value = input_record.Event.KeyEvent.uChar.AsciiChar;
			if (current_event.ascii_value == 0)
			{
				current_event.special_keyboard_key_activated = true;
				current_event.special_key_value = (uint16_t) input_record.Event.KeyEvent.wVirtualKeyCode;
			}
			else
			{
				current_event.special_keyboard_key_activated = false;
				current_event.special_key_value = 0;
			}
		}
		/* mouse events disabled in v3
		else if (input_record.EventType == MOUSE_EVENT)
		{
			// find out mouse action
			if (input_record.Event.MouseEvent.dwEventFlags == 0)
			{
				// check if left button was clicked
				if (input_record.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
				{
					// left mouse down
					mouse_down_registered = true;
					mouse_down_x = input_record.Event.MouseEvent.dwMousePosition.X;
					mouse_down_y = input_record.Event.MouseEvent.dwMousePosition.Y;
					continue;
				}
				else
				{
					// left mouse up
					if (mouse_down_registered)
					{
						mouse_down_registered = false;
						if (input_record.Event.MouseEvent.dwMousePosition.X == mouse_down_x
							&& input_record.Event.MouseEvent.dwMousePosition.Y == mouse_down_y)
							current_event.mouse_clicked = true;
						else
							continue;
					}

				}
			}
			else if (input_record.Event.MouseEvent.dwEventFlags == MOUSE_MOVED)
			{
				// don't trigger another event for the same coordinates
				current_mouse_x = input_record.Event.MouseEvent.dwMousePosition.X;
				current_mouse_y = input_record.Event.MouseEvent.dwMousePosition.Y;

				if (current_mouse_x == last_mouse_x && current_mouse_y == last_mouse_y)
					continue;
				else
				{
					current_event.mouse_clicked = false;
					current_event.abs_x_coordinates = current_mouse_x;
					current_event.abs_x_coordinates = current_mouse_y;
				}
			}
		}
		*/
		else
			continue;

		// enqueue the input event
		pthread_mutex_lock(&input_event_lock);
		input_event_list.push_back(current_event);
		pthread_cond_broadcast(&input_event_cond);
		pthread_mutex_unlock(&input_event_lock);
	}
#endif

#ifndef _WIN32
	
	int keypress;
	input_event_t current_event;

	while(1)
	{
		keypress = getch();
		current_event.clear();

		if (keypress == KEY_DOWN || keypress == KEY_UP || keypress == KEY_LEFT || keypress == KEY_RIGHT
			|| keypress == KEY_HOME || keypress == KEY_NPAGE || keypress == KEY_PPAGE || keypress == KEY_END)
		{
			current_event.keyboard_event = true;
			current_event.ascii_value = 0;
			current_event.special_keyboard_key_activated = true;

			if (keypress == KEY_DOWN)
				current_event.special_key_value = KEYBOARD_KEY_DOWN;
			else if (keypress == KEY_UP)
				current_event.special_key_value = KEYBOARD_KEY_UP;
			else if (keypress == KEY_LEFT)
				current_event.special_key_value = KEYBOARD_KEY_LEFT;
			else if (keypress == KEY_RIGHT)
				current_event.special_key_value = KEYBOARD_KEY_RIGHT;
			else if (keypress == KEY_HOME)
				current_event.special_key_value = KEYBOARD_KEY_HOME;
			else if (keypress == KEY_NPAGE)
				current_event.special_key_value = KEYBOARD_KEY_NPAGE;
			else if (keypress == KEY_PPAGE)
				current_event.special_key_value = KEYBOARD_KEY_PPAGE;
			else if (keypress == KEY_END)
				current_event.special_key_value = KEYBOARD_KEY_END;
			else
				continue;
		}
		else if (keypress == KEY_BACKSPACE)
		{
			current_event.keyboard_event = true;
			current_event.ascii_value = '\b';
			current_event.special_keyboard_key_activated = false;
			current_event.special_key_value = 0;
		}
		else
		{
			current_event.keyboard_event = true;
			current_event.ascii_value = (uint8_t) keypress;
			current_event.special_keyboard_key_activated = false;
			current_event.special_key_value = 0;
		}

		// enqueue the input event
		pthread_mutex_lock(&input_event_lock);
		input_event_list.push_back(current_event);
		pthread_cond_broadcast(&input_event_cond);
		pthread_mutex_unlock(&input_event_lock);
	}


#endif

	return NULL;
}

void* display_controller::focus_arbitration_thread(void* arguments)
{
//	input_panel* last_mouseover_panel = NULL;
//	input_panel* current_mouseover_panel = NULL;
	input_event_t current_event;
	pthread_detach(pthread_self());
	std::list<input_panel*>::iterator input_panel_iterator;

	pthread_mutex_lock(&input_event_lock);
	while(1)
	{
		// wait till input event
		while (input_event_list.size() == 0 && !input_panel_list_changed)
			pthread_cond_wait(&input_event_cond, &input_event_lock);

		// process panel list changes
		if (input_panel_list_changed)
		{
			//pthread_mutex_lock(&input_panel_list_lock);
			update_focus_on_list_change();
			input_panel_list_changed = false;
			//pthread_mutex_unlock(&input_panel_list_lock);
		}

		// process one input event
		if (input_event_list.size() > 0)
		{
			current_event.clear();
			current_event = input_event_list.front();
			input_event_list.pop_front();
			
			// note: need to lock input panel list lock to use the current_focus variable
			pthread_mutex_lock(&input_panel_list_lock);
			if (current_focus != NULL)
			{
				if (current_event.keyboard_event)
					current_focus->process_keypress(current_event.ascii_value, current_event.special_key_value);
				/* mouse events disabled in v3
				else if (current_event.mouse_clicked)
					current_focus->process_click(current_event.abs_x_coordinates, current_event.abs_y_coordinates);
				else
				{
					// mouse move event -- search for panel with mousemove
					current_mouseover_panel = NULL;
					for (input_panel_iterator = input_panel_list.begin(); input_panel_iterator != input_panel_list.end(); input_panel_iterator++)
					{
						if (input_panel_iterator->visible
							&& input_panel_iterator->mouse_input_enabled
							&& (int32_t) current_event.abs_x_coordinates >= input_panel_iterator->abs_x_origin
							&& (int32_t) current_event.abs_x_coordinates < (int32_t) input_panel_iterator->abs_x_origin + (int32_t) input_panel_iterator->width
							&& (int32_t) current_event.abs_y_coordinates >= input_panel_iterator->abs_y_origin
							&& (int32_t) current_event.abs_y_coordinates < input_panel_iterator->abs_y_origin + (int32_t) input_panel_iterator->height)
						{
							// perform mouseover for the current panel
							current_mouseover_panel = input_panel_iterator->ptr;
							current_mouseover_panel->process_mousemove(current_event.abs_x_coordinates-input_panel_iterator->abs_x_origin, current_event.abs_y_coordinates-input_panel_iterator->abs_y_origin);
							break;
						}
					}

					if (current_mouseover_panel != NULL
						&& last_mouseover_panel != NULL
						&& current_mouseover_panel != last_mouseover_panel)
					{
						// perform mouseout for the last panel with mouseover
						for (input_panel_iterator = input_panel_list.begin(); input_panel_iterator != input_panel_list.end(); input_panel_iterator++)
						{
							if (input_panel_iterator->ptr == last_mouseover_panel
								&& input_panel_iterator->mouse_input_enabled
								&& input_panel_iterator->visible)
							{
								last_mouseover_panel->process_mouseout();
								break;
							}
						}
					}
				} */
			}
			pthread_mutex_unlock(&input_panel_list_lock);
		}
	}

	return NULL;
}

void* display_controller::repaint_thread(void* arguments)
{
	// variables for copy phase
	std::list<text_panel*>::iterator panel_iterator;
	text_panel* current_panel = NULL;
	uint32_t current_panel_rows, current_panel_columns;
	uint32_t current_panel_x, current_panel_y;
	uint32_t counter;
	uint32_t rel_x, rel_y;
	int32_t abs_x, abs_y;
	uint32_t render_buffer_index;

	// variables for write phase
	uint32_t last_row, last_column;
#ifdef _WIN32
	COORD position;
#endif
	bool requires_flush;
	char accumulator_buffer[2048];
	uint32_t accumulator_index;
	uint8_t last_composite_color;

	pthread_detach(pthread_self());

	// perform all repaint work in the loop here
	//pthread_mutex_lock(&refresh_signal_lock);
	while(1)
	{
		pthread_mutex_lock(&refresh_signal_lock);
		while (!refresh_signalled)
			pthread_cond_wait(&refresh_signal_cond, &refresh_signal_lock);
		pthread_mutex_unlock(&refresh_signal_lock);

		refresh_signalled = false;
	
		// lock display object for rendering
		pthread_mutex_lock(&display_lock);

		// sort the list by depth
		pthread_mutex_lock(&panel_list_lock);
		output_text_panels.sort(z_sort);

		// background textbox is always lowest -- copy into render buffer first
		pthread_mutex_lock(&background_textbox.output_buffer_lock);
		pthread_mutex_lock(&background_textbox.intermediate_buffer_lock);
		memcpy(render_buffer, background_textbox.intermediate_buffer, background_textbox.rows*background_textbox.columns*sizeof(output_char_t));
		pthread_mutex_unlock(&background_textbox.intermediate_buffer_lock);
		pthread_mutex_unlock(&background_textbox.output_buffer_lock);

		// background textbox copied; now copy the rest
		for (panel_iterator = output_text_panels.begin(); panel_iterator != output_text_panels.end(); panel_iterator++)
		{
			current_panel = *panel_iterator;

			// trivial test: check for visibility
			if (!current_panel->visible)
				continue;

			pthread_mutex_lock(&(current_panel->output_buffer_lock));

			// quickly copy content to output buffer
			pthread_mutex_lock(&(current_panel->intermediate_buffer_lock));
			current_panel_rows = current_panel->rows;
			current_panel_columns = current_panel->columns;
			current_panel_x = current_panel->abs_x_origin;
			current_panel_y = current_panel->abs_y_origin;
			memcpy(current_panel->output_buffer, current_panel->intermediate_buffer, current_panel_rows*current_panel_columns*sizeof(output_char_t));
			pthread_mutex_unlock(&(current_panel->intermediate_buffer_lock));

			// unlock intermediate buffer so writing can proceed, but keep the output buffer locked for processing
			for (counter = 0; counter < current_panel_rows*current_panel_columns; counter++)
			{
				rel_x = counter % current_panel_columns;
				rel_y = counter / current_panel_columns;
				abs_x = rel_x + current_panel_x;
				abs_y = rel_y + current_panel_y;

				// perform clipping
				if (abs_x < 0 || abs_x >= (int32_t) screen_columns || abs_y < 0 || abs_y >= (int32_t) screen_rows)
					continue;

				// not clipped, transform data to local render buffer
				render_buffer_index = abs_y*screen_columns+abs_x;
				render_buffer[render_buffer_index].character = current_panel->output_buffer[counter].character;
				render_buffer[render_buffer_index].composite_color = current_panel->output_buffer[counter].composite_color;
			}

			pthread_mutex_unlock(&(current_panel->output_buffer_lock));
		}
		pthread_mutex_unlock(&panel_list_lock);

		// finished processing all panels; now render to screen
		requires_flush = false;
		last_row = 0;
		last_column = 0;
		accumulator_index = 0;
		last_composite_color = COLOR_BACKGROUND_BLACK | COLOR_FOREGROUND_WHITE;
		for (counter = 0; counter < screen_rows*screen_columns; counter++)
		{
			if (output_buffer[counter].character == render_buffer[counter].character
				&& output_buffer[counter].composite_color == render_buffer[counter].composite_color)
			{
				if (requires_flush)
				{
					accumulator_buffer[accumulator_index] = '\0';
#ifdef _WIN32
					// flush output to screen
					position.Y = last_row;
					position.X = last_column;
					SetConsoleCursorPosition(hOut, position);
					SetConsoleTextAttribute(hOut, last_composite_color);
					printf("%s", accumulator_buffer);
#endif
#ifndef _WIN32
					set_active_color(last_composite_color);
					mvprintw(last_row, last_column, "%s", accumulator_buffer);
#endif
						
					accumulator_index = 0;
					requires_flush = false;
				}
			}
			else
			{
				// if accumulator is fresh, initialize it
				if (!requires_flush)
				{
					last_composite_color = render_buffer[counter].composite_color;
					accumulator_buffer[accumulator_index++] = render_buffer[counter].character;
					last_row = counter / screen_columns;
					last_column = counter % screen_columns;
					requires_flush = true;
				}
				else
				{
					// accumulator buffer already has content and current offset is different in content
					// contents differ, add to accumulator buffer only if color is same
					if (last_composite_color == render_buffer[counter].composite_color)
					{
						if (accumulator_index+1 == sizeof(accumulator_buffer))
						{
							accumulator_buffer[accumulator_index] = '\0';
#ifdef _WIN32
							// buffer full, flush to output
							position.Y = last_row;
							position.X = last_column;
							SetConsoleCursorPosition(hOut, position);
							SetConsoleTextAttribute(hOut, last_composite_color);
							printf("%s", accumulator_buffer);
#endif
#ifndef _WIN32
							set_active_color(last_composite_color);
							mvprintw(last_row, last_column, "%s", accumulator_buffer);
#endif
							accumulator_index = 0;
							requires_flush = false;
						}
						else
							accumulator_buffer[accumulator_index++] = render_buffer[counter].character;
					}
					else
					{
						// colors not same; need to flush
						accumulator_buffer[accumulator_index] = '\0';
#ifdef _WIN32
						position.Y = last_row;
						position.X = last_column;
						SetConsoleCursorPosition(hOut, position);
						SetConsoleTextAttribute(hOut, last_composite_color);
						printf("%s", accumulator_buffer);
#endif
#ifndef _WIN32
						set_active_color(last_composite_color);
						mvprintw(last_row, last_column, "%s", accumulator_buffer);
#endif

						// after flushing, set new parameters
						accumulator_index = 0;
						last_composite_color = render_buffer[counter].composite_color;
						accumulator_buffer[accumulator_index++] = render_buffer[counter].character;
						last_row = counter / screen_columns;
						last_column = counter % screen_columns;
						requires_flush = true;
					}
				}
			}
		}

		// perform a final flush if required
		if (requires_flush)
		{
			accumulator_buffer[accumulator_index] = '\0';
#ifdef _WIN32
			position.Y = last_row;
			position.X = last_column;
			SetConsoleCursorPosition(hOut, position);
			SetConsoleTextAttribute(hOut, last_composite_color);
			printf("%s", accumulator_buffer);
#endif
#ifndef _WIN32
			set_active_color(last_composite_color);
			mvprintw(last_row, last_column, "%s", accumulator_buffer);
#endif
		}

		// screens synchronized; copy contents
		memcpy(output_buffer, render_buffer, screen_rows*screen_columns*sizeof(output_char_t));

		// restore cursor state if required
		pthread_mutex_lock(&input_panel_list_lock);
		if (current_focus != NULL && current_focus->focus_requires_cursor)
		{
#ifdef _WIN32
			position.X = current_focus->current_cursor_column + current_focus->abs_x_origin;
			position.Y = current_focus->current_cursor_row + current_focus->abs_y_origin;
			pthread_mutex_unlock(&input_panel_list_lock);

			// screen wrap safety check
			if (position.X >= (int32_t) screen_columns)
				position.X = screen_columns-1;
			if (position.Y >= (int32_t) screen_rows)
				position.Y = screen_rows-1;

			SetConsoleCursorPosition(hOut, position);
			SetConsoleTextAttribute(hOut, COLOR_BACKGROUND_LIGHT_GREEN | COLOR_FOREGROUND_BRIGHT_WHITE);
			printf("%c", output_buffer[position.X+ position.Y*screen_columns].character);
#endif
#ifndef _WIN32
			int cursor_position_x = current_focus->current_cursor_column + current_focus->abs_x_origin;
			int cursor_position_y = current_focus->current_cursor_row + current_focus->abs_y_origin;
			pthread_mutex_unlock(&input_panel_list_lock);

			if (cursor_position_x >= (int32_t) screen_columns)
				cursor_position_x = screen_columns-1;
			if (cursor_position_y >= (int32_t) screen_rows)
				cursor_position_y = screen_rows-1;

			move(cursor_position_y, cursor_position_x);
			set_active_color(COLOR_BACKGROUND_LIGHT_GREEN | COLOR_FOREGROUND_BRIGHT_WHITE);
			addch(output_buffer[cursor_position_x+cursor_position_y*screen_columns].character);
#endif

			// update output buffer
			//output_buffer[position.X+ position.Y*screen_columns].character = ' ';
#ifdef _WIN32
			output_buffer[position.X+ position.Y*screen_columns].composite_color = COLOR_BACKGROUND_LIGHT_GREEN | COLOR_FOREGROUND_BRIGHT_WHITE;
#endif
#ifndef _WIN32
			output_buffer[cursor_position_x+cursor_position_y*screen_columns].composite_color = COLOR_BACKGROUND_LIGHT_GREEN | COLOR_FOREGROUND_BRIGHT_WHITE;
#endif
		}
		else
			pthread_mutex_unlock(&input_panel_list_lock);

		gettimeofday(&last_refresh_time, NULL);
		pthread_mutex_unlock(&display_lock);
#ifndef _WIN32
		refresh();
#endif

	}

	return NULL;
}

void display_controller::update_focus_on_list_change()
{
	std::list<input_panel*>::iterator panel_iterator;

	// lock already acquired, just update the current_focus variable
	// note: z-cursor clipping not yet performed (overlapping panels won't disable cursor appearance)

	pthread_mutex_lock(&input_panel_list_lock);

	// first check if current focus is still valid
	if (!validate_current_focus_no_lock())
		current_focus = NULL;

	// check if current focus is modal -- if it is, don't allow an update focus change
	if (current_focus != NULL && current_focus->is_modal)
	{
		pthread_mutex_unlock(&input_panel_list_lock);
		return;
	}

	// search for a panel that is modal
	for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
	{
		if ((*panel_iterator)->is_modal && (*panel_iterator)->visible && (*panel_iterator)->accepts_keypress)
		{
			if (current_focus != NULL)
				current_focus->off_focus();
			current_focus = *panel_iterator;
			current_focus->on_focus();
			pthread_mutex_unlock(&input_panel_list_lock);
			return;
		}
	}

	// search for a panel that is calling for focus
	for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
	{
		if ((*panel_iterator)->wants_focus && (*panel_iterator)->visible && (*panel_iterator)->accepts_keypress)
		{
			(*panel_iterator)->wants_focus = false;
			if (current_focus != NULL)
				current_focus->off_focus();
			current_focus = *panel_iterator;
			current_focus->on_focus();
			pthread_mutex_unlock(&input_panel_list_lock);
			return;
		}
	}

	// simple processing: if there is no current focus, arbitrarily pick the first one
	if (current_focus == NULL)
	{
		for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
		{
			if ((*panel_iterator)->visible && (*panel_iterator)->accepts_keypress)
			{
				current_focus = *panel_iterator;
				current_focus->on_focus();
				pthread_mutex_unlock(&input_panel_list_lock);
				return;
			}
		}
	}

	// else keep the current focus
	pthread_mutex_unlock(&input_panel_list_lock);
}

void display_controller::update_focus_on_tab()
{
	bool found;
	std::list<input_panel*>::iterator panel_iterator;
	input_panel* current_panel;
	input_panel* last_panel;
	uint32_t number_of_panels, counter;

	pthread_mutex_lock(&input_panel_list_lock);

	// first check if current focus is still valid
	if (!validate_current_focus_no_lock())
		current_focus = NULL;

	// check if current focus is modal -- if it is, don't allow an update focus change
	if (current_focus != NULL && current_focus->is_modal)
	{
		pthread_mutex_unlock(&input_panel_list_lock);
		return;
	}

	if (current_focus != NULL)
	{
		// search for the focus panel in the panel list
		found = false;
		for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
		{
			if (*panel_iterator == current_focus && current_focus->visible && current_focus->accepts_keypress)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			// pick first panel in list
			for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
			{
				if ((*panel_iterator)->visible && (*panel_iterator)->accepts_keypress)
				{
					current_focus = *panel_iterator;
					current_focus->on_focus();
					pthread_mutex_unlock(&input_panel_list_lock);
					return;
				}
			}
		}
	}

	// current focus is valid -- pick the next panel in the list
	last_panel = current_focus;
	number_of_panels = (uint32_t) input_panel_list.size();
	counter = 0;
	while(counter < number_of_panels)
	{
		current_panel = input_panel_list.front();
		input_panel_list.pop_front();
		input_panel_list.push_back(current_panel);

		if (current_panel->visible && current_panel->accepts_keypress && current_panel != current_focus)
		{
			current_focus = current_panel;
			last_panel->off_focus();
			current_focus->on_focus();
			pthread_mutex_unlock(&input_panel_list_lock);
			return;
		}

		counter++;
	}

	// current focus got repicked again, so nothing to do
	pthread_mutex_unlock(&input_panel_list_lock);

}

void display_controller::update_focus_on_mouse_click(const input_event_t& mouse_event)
{
	/*
	// check through every input panel and find out which one should receive
	// focus. if no such panel found, don't update current focus
	std::list<input_panel_descriptor_t>::iterator panel_iterator;
	input_panel* topmost_panel = NULL;

	for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
	{
		if (panel_iterator->visible && panel_iterator->mouse_input_enabled
			&& mouse_event.mouse_clicked
			&& (int32_t) mouse_event.abs_x_coordinates >= panel_iterator->abs_x_origin
			&& (int32_t) mouse_event.abs_x_coordinates < panel_iterator->abs_x_origin + panel_iterator->width
			&& (int32_t) mouse_event.abs_y_coordinates >= panel_iterator->abs_y_origin
			&& (int32_t) mouse_event.abs_y_coordinates < panel_iterator->abs_y_origin + panel_iterator->height)
		{
			// get uppermost z panel
			if (topmost_panel == NULL)
				topmost_panel = panel_iterator->ptr;
			else
			{
				if (panel_iterator->z_position > topmost_panel->z_position)
					topmost_panel = panel_iterator->ptr;
			}
		}
	}

	// update focus if required
	if (topmost_panel != NULL)
	{
		current_focus = topmost_panel;
		show_cursor(current_focus->current_cursor_row, current_focus->current_cursor_column);
	}
	*/
}


bool display_controller::validate_current_focus_no_lock()
{
	std::list<input_panel*>::iterator panel_iterator;

	if (current_focus == NULL)
		return false;

	for (panel_iterator = input_panel_list.begin(); panel_iterator != input_panel_list.end(); panel_iterator++)
	{
		if (*panel_iterator == current_focus)
		{
			if (current_focus->visible && current_focus->accepts_keypress)
				return true;
			else
				return false;
		}
	}

	return false;
}

#ifndef _WIN32

void display_controller::set_active_color(uint8_t composite_color)
{
	uint8_t base_foreground_color;
	bool foreground_color_bright;
	uint8_t base_background_color;
	uint8_t translated_color_index;

	base_foreground_color = composite_color & 7;
	foreground_color_bright = ((composite_color & 8) == 1 ? true : false);
	base_background_color = (composite_color & (7 << 4)) >> 4;

	translated_color_index = base_foreground_color;
	translated_color_index += (base_background_color*8);

	if (foreground_color_bright)
		attrset(COLOR_PAIR(translated_color_index) | A_BOLD);
	else
		attrset(COLOR_PAIR(translated_color_index));

}

void display_controller::setup_colors()
{
	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_YELLOW, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);

	init_pair(8, COLOR_BLACK, COLOR_BLUE);
	init_pair(9, COLOR_BLUE, COLOR_BLUE);
	init_pair(10, COLOR_GREEN, COLOR_BLUE);
	init_pair(11, COLOR_CYAN, COLOR_BLUE);
	init_pair(12, COLOR_RED, COLOR_BLUE);
	init_pair(13, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(14, COLOR_YELLOW, COLOR_BLUE);
	init_pair(15, COLOR_WHITE, COLOR_BLUE);

	init_pair(16, COLOR_BLACK, COLOR_GREEN);
	init_pair(17, COLOR_BLUE, COLOR_GREEN);
	init_pair(18, COLOR_GREEN, COLOR_GREEN);
	init_pair(19, COLOR_CYAN, COLOR_GREEN);
	init_pair(20, COLOR_RED, COLOR_GREEN);
	init_pair(21, COLOR_MAGENTA, COLOR_GREEN);
	init_pair(22, COLOR_YELLOW, COLOR_GREEN);
	init_pair(23, COLOR_WHITE, COLOR_GREEN);

	init_pair(24, COLOR_BLACK, COLOR_CYAN);
	init_pair(25, COLOR_BLUE, COLOR_CYAN);
	init_pair(26, COLOR_GREEN, COLOR_CYAN);
	init_pair(27, COLOR_CYAN, COLOR_CYAN);
	init_pair(28, COLOR_RED, COLOR_CYAN);
	init_pair(29, COLOR_MAGENTA, COLOR_CYAN);
	init_pair(30, COLOR_YELLOW, COLOR_CYAN);
	init_pair(31, COLOR_WHITE, COLOR_CYAN);

	init_pair(32, COLOR_BLACK, COLOR_RED);
	init_pair(33, COLOR_BLUE, COLOR_RED);
	init_pair(34, COLOR_GREEN, COLOR_RED);
	init_pair(35, COLOR_CYAN, COLOR_RED);
	init_pair(36, COLOR_RED, COLOR_RED);
	init_pair(37, COLOR_MAGENTA, COLOR_RED);
	init_pair(38, COLOR_YELLOW, COLOR_RED);
	init_pair(39, COLOR_WHITE, COLOR_RED);

	init_pair(40, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(41, COLOR_BLUE, COLOR_MAGENTA);
	init_pair(42, COLOR_GREEN, COLOR_MAGENTA);
	init_pair(43, COLOR_CYAN, COLOR_MAGENTA);
	init_pair(44, COLOR_RED, COLOR_MAGENTA);
	init_pair(45, COLOR_MAGENTA, COLOR_MAGENTA);
	init_pair(46, COLOR_YELLOW, COLOR_MAGENTA);
	init_pair(47, COLOR_WHITE, COLOR_MAGENTA);

	init_pair(48, COLOR_BLACK, COLOR_YELLOW);
	init_pair(49, COLOR_BLUE, COLOR_YELLOW);
	init_pair(50, COLOR_GREEN, COLOR_YELLOW);
	init_pair(51, COLOR_CYAN, COLOR_YELLOW);
	init_pair(52, COLOR_RED, COLOR_YELLOW);
	init_pair(53, COLOR_MAGENTA, COLOR_YELLOW);
	init_pair(54, COLOR_YELLOW, COLOR_YELLOW);
	init_pair(55, COLOR_WHITE, COLOR_YELLOW);

	init_pair(56, COLOR_BLACK, COLOR_WHITE);
	init_pair(57, COLOR_BLUE, COLOR_WHITE);
	init_pair(58, COLOR_GREEN, COLOR_WHITE);
	init_pair(59, COLOR_CYAN, COLOR_WHITE);
	init_pair(60, COLOR_RED, COLOR_WHITE);
	init_pair(61, COLOR_MAGENTA, COLOR_WHITE);
	init_pair(62, COLOR_YELLOW, COLOR_WHITE);
	init_pair(63, COLOR_WHITE, COLOR_WHITE);
}



#endif


// internal routine to do the z sort
bool z_sort(text_panel* panel1, text_panel* panel2)
{	
	int32_t position1, position2;
	panel1->get_z_position(&position1);
	panel2->get_z_position(&position2);

	if (position1 >= position2)
		return false;

	return true;
}

#endif

