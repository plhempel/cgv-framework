#include "input.h"

#include <cgv_g2d/msdf_font.h>
#include <cgv_g2d/msdf_gl_canvas_font_renderer.h>

using namespace cgv::render;

namespace cg {
namespace g2d {

bool input::is_allowed(char c) {
	switch(type) {
	case Type::kFloat: return std::isdigit(c) || c == '.' || c == 'e';
	case Type::kInteger: return std::isdigit(c);
	case Type::kString:
	default: return std::isprint(c);
	}
}

void input::erase_at_cursor() {
	buffer.erase(cursor_position, 1);

	do_callback();
}

void input::insert_at_cursor(char c) {
	if(is_allowed(c)) {
		buffer.insert(cursor_position, 1, c);
		++cursor_position;

		do_callback();
	}
}

bool input::set_value(const std::string& v) {
	if(v == buffer)
		return false;

	buffer = v;

	//cursor_position = std::min(cursor_position, buffer.length());
	cursor_position = buffer.length();
	focused = false;
	update();
	return true;
}

bool input::handle_key_event(cgv::gui::key_event& e) {
	if(focused && (e.get_action() == cgv::gui::KA_PRESS || e.get_action() == cgv::gui::KA_REPEAT)) {
		switch(e.get_key()) {
		case cgv::gui::KEY_Home:
			cursor_position = 0;
			return true;
		case cgv::gui::KEY_End:
			cursor_position = buffer.length();
			return true;
		case cgv::gui::KEY_Left:
			if(cursor_position > 0)
				--cursor_position;
			return true;
		case cgv::gui::KEY_Right:
			if(cursor_position < buffer.length())
				++cursor_position;
			return true;
		case cgv::gui::KEY_Back_Space:
			if(cursor_position > 0 && cursor_position <= buffer.length()) {
				--cursor_position;
				erase_at_cursor();
			}
			return true;
		case cgv::gui::KEY_Delete:
			if(cursor_position < buffer.length())
				erase_at_cursor();
			return true;
		case cgv::gui::KEY_Space:
			insert_at_cursor(' ');
			return true;
		case '\\':
			// capture escape sequences, e.g, when pressing '^'
			return true;
		default:
			if(e.get_key() < 256) {
				//std::cout << "key = " << ke.get_key() << ", char = '" << ke.get_char() << "', char_num = " << static_cast<int>(ke.get_char()) << std::endl;
				insert_at_cursor(e.get_char());
				return true;
			}
			break;
		}
	}

	return false;
}

bool input::handle_mouse_event(cgv::gui::mouse_event& e, ivec2 mouse_position) {
	cgv::gui::MouseAction action = e.get_action();

	if(e.get_button() == cgv::gui::MB_LEFT_BUTTON) {
		if(action == cgv::gui::MouseAction::MA_PRESS) {
			if(rectangle.is_inside(mouse_position)) {
				focused = true;
				do_focus = true;
				focus_position = mouse_position.x();
				cursor_position = buffer.length();
				return true;
			} else if(focused) {
				// TODO: do we even need to deactivate the input? FLTK for example only removes the caret when the mouse leaves the gui
				focused = false;
				cursor_position = 0;
				// TOOD: should issue redraw but not cancel event
				//return true;
			}
		}
	}

	return false;
}

void input::draw(context& ctx, cgv::g2d::canvas& cnvs, const styles& style) {
	auto& font = cgv::g2d::ref_msdf_font_regular(ctx);

	if(do_focus) {
		do_focus = false;
		cursor_position = 0;

		float position = rectangle.x() + 5.0f;
		float closest_distance = std::abs(position - focus_position);
		float last_distance = closest_distance;

		for(size_t i = 0; i < buffer.length(); ++i) {
			const auto& glyph = font.get_glyph_info(static_cast<unsigned char>(buffer[i]));

			position += glyph.advance * style.text.font_size;
			float distance = std::abs(position - focus_position);

			if(distance < closest_distance) {
				closest_distance = distance;
				cursor_position = i + 1;
			} else {
				break;
			}

			last_distance = distance;
		}
	}

	cnvs.enable_shader(ctx, "rectangle");
	cnvs.set_style(ctx, style.flat_box);
	cnvs.draw_shape(ctx, rectangle, style.control_color);
	cnvs.disable_current_shader(ctx);

	ivec2 position(rectangle.x(), rectangle.center().y());

	cgv::g2d::ref_msdf_gl_canvas_font_renderer(ctx).render(ctx, cnvs, font, label, style.text, position - ivec2(5, 0), TA_RIGHT);
	cgv::g2d::ref_msdf_gl_canvas_font_renderer(ctx).render(ctx, cnvs, font, buffer, style.text, position + ivec2(5, 0), TA_LEFT);

	if(focused) {
		cnvs.enable_shader(ctx, "rectangle");
		cnvs.set_style(ctx, style.flat_box);

		vec2 text_size = style.text.font_size * vec2(font.compute_length(buffer, cursor_position), 1.0f);

		vec2 position = rectangle.position;
		position.x() += text_size.x() + 5.0f;
		position.x() = std::floor(position.x()) + 0.5f;
		position.y() += 2.0f;

		cnvs.draw_shape(ctx, position, vec2(1.0f, 16.0f), style.text.fill_color);
		cnvs.disable_current_shader(ctx);
	}
}

}
}
