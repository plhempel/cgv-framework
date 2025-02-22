#include "msdf_gl_font_renderer.h"

namespace cgv {
namespace g2d {

bool msdf_gl_font_renderer::build_shader_program(const cgv::render::context& ctx) {
	return prog.build_program(ctx, "sdf_font2d.glpr", true);
}

void msdf_gl_font_renderer::manage_singleton(cgv::render::context& ctx, const std::string& name, int& ref_count, int ref_count_change) {
	switch(ref_count_change) {
	case 1:
		if(ref_count == 0) {
			if(!init(ctx))
				ctx.error(std::string("unable to initialize ") + name + " singleton");
		}
		++ref_count;
		break;
	case 0:
		break;
	case -1:
		if(ref_count == 0)
			ctx.error(std::string("attempt to decrease reference count of ") + name + " singleton below 0");
		else {
			if(--ref_count == 0)
				destruct(ctx);
		}
		break;
	default:
		ctx.error(std::string("invalid change reference count outside {-1,0,1} for ") + name + " singleton");
	}
}

void msdf_gl_font_renderer::destruct(cgv::render::context& ctx) {
	prog.destruct(ctx);
}

bool msdf_gl_font_renderer::init(cgv::render::context& ctx) {
	return build_shader_program(ctx);
}

cgv::render::shader_program& msdf_gl_font_renderer::ref_prog() {
	return prog;
}

bool msdf_gl_font_renderer::enable(cgv::render::context& ctx, const ivec2& viewport_resolution, msdf_text_geometry& tg, const text2d_style& style) {
	bool res = prog.is_enabled() ? true : prog.enable(ctx);
	res &= tg.enable(ctx);
	if(res) {
		prog.set_uniform(ctx, "resolution", viewport_resolution);
		prog.set_uniform(ctx, "src_size", tg.get_msdf_font()->get_initial_font_size());
		prog.set_uniform(ctx, "pixel_range", tg.get_msdf_font()->get_pixel_range());

		style.apply(ctx, prog);

		use_subpixel_rendering = style.enable_subpixel_rendering;
		if(use_subpixel_rendering) {
			glGetBooleanv(GL_BLEND, &blending_was_enabled);
			glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src_color);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_src_alpha);
			glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst_color);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_dst_alpha);

			if(!blending_was_enabled)
				glEnable(GL_BLEND);

			glBlendFuncSeparate(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBlendColor(style.fill_color.R(), style.fill_color.G(), style.fill_color.B(), 1.0f);
		}
	}
	return res;
}

bool msdf_gl_font_renderer::disable(cgv::render::context& ctx, msdf_text_geometry& tg) {
	bool res = prog.disable(ctx);
	tg.disable(ctx);

	if(res) {
		if(use_subpixel_rendering) {
			if(!blending_was_enabled)
				glDisable(GL_BLEND);

			glBlendFuncSeparate(blend_src_color, blend_dst_color, blend_src_alpha, blend_dst_alpha);
		}
	}

	return res;
}

void msdf_gl_font_renderer::draw(cgv::render::context& ctx, msdf_text_geometry& tg, size_t offset, int count) {
	if(offset >= tg.size())
		return;

	size_t end = count < 0 ? tg.size() : offset + static_cast<size_t>(count);
	end = std::min(end, tg.size());

	float alignment_offset_scale = tg.get_msdf_font()->get_cap_height();

	for(size_t i = offset; i < end; ++i) {
		const auto& text = tg.ref_texts()[i];

		vec2 alignment_offset_factors(-0.5f);

		if(text.alignment & cgv::render::TA_LEFT)
			alignment_offset_factors.x() = 0.0f;
		else if(text.alignment & cgv::render::TA_RIGHT)
			alignment_offset_factors.x() = -1.0f;

		if(text.alignment & cgv::render::TA_TOP)
			alignment_offset_factors.y() = -1.0f;
		else if(text.alignment & cgv::render::TA_BOTTOM)
			alignment_offset_factors.y() = 0.0f;

		vec2 size_scale = text.size;
		size_scale.x() *= text.size.y();

		alignment_offset_factors.y() *= alignment_offset_scale;

		prog.set_uniform(ctx, "position_offset", text.position);
		prog.set_uniform(ctx, "alignment_offset_factors", alignment_offset_factors);
		prog.set_uniform(ctx, "text_size", size_scale);
		prog.set_uniform(ctx, "angle", text.angle);
		prog.set_uniform(ctx, "color", text.color);
		
		glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, (GLint)0, (GLsizei)4, (GLsizei)text.str.size(), (GLuint)text.offset);
	}
}

bool msdf_gl_font_renderer::render(cgv::render::context& ctx, const ivec2& viewport_resolution, msdf_text_geometry& tg, const text2d_style& style, size_t offset, int count) {
	if(!enable(ctx, viewport_resolution, tg, style))
		return false;
	draw(ctx, tg, offset, count);
	return disable(ctx, tg);
}

msdf_gl_font_renderer& ref_msdf_gl_font_renderer(cgv::render::context& ctx, int ref_count_change) {
	static int ref_count = 0;
	static msdf_gl_font_renderer r;
	r.manage_singleton(ctx, "msdf_gl_font_renderer", ref_count, ref_count_change);
	return r;
}

}
}
