#include "rgbd_point_renderer.h"

namespace rgbd {

void rgbd_point_renderer::update_defines(cgv::render::shader_define_map& defines) 
{
	point_renderer::update_defines(defines);
	cgv::render::shader_code::set_define(defines, "USE_UNDISTORTION_MAP", use_undistortion_map, false);
}
bool rgbd_point_renderer::build_shader_program(cgv::render::context& ctx, cgv::render::shader_program& prog, const cgv::render::shader_define_map& defines)
{
	return prog.build_program(ctx, "rgbd_pc.glpr", true, defines);
}
rgbd_point_renderer::rgbd_point_renderer() : undistortion_tex("flt32[R,G]") 
{
	undistortion_tex.set_mag_filter(cgv::render::TF_NEAREST);
}
void rgbd_point_renderer::configure_invalid_color_handling(bool discard, const rgba& color)
{
	discard_invalid_color_points = discard;
	invalid_color = color;
}
void rgbd_point_renderer::set_geomtry_less_rendering(bool active) 
{
	geometry_less_rendering = active; 
}
bool rgbd_point_renderer::do_geometry_less_rendering() const 
{
	return geometry_less_rendering; 
}
void rgbd_point_renderer::set_color_lookup(bool active) 
{
	lookup_color = active; 
}
bool rgbd_point_renderer::do_lookup_color() const 
{
	return lookup_color; 
}
void rgbd_point_renderer::set_undistortion_map_usage(bool do_use)
{
	use_undistortion_map = do_use;
	if (do_use && calib_set)
		calib.depth.compute_undistortion_map(undistortion_map);
}
void rgbd_point_renderer::set_calibration(const rgbd::rgbd_calibration& _calib)
{
	calib = _calib;
	calib_set = true;
	if (use_undistortion_map) {
		calib.depth.compute_undistortion_map(undistortion_map);
		undistortion_map_outofdate = true;
	}
}
bool rgbd_point_renderer::validate_attributes(const cgv::render::context& ctx) const
{
	if (geometry_less_rendering)
		return true;
	else
		return cgv::render::point_renderer::validate_attributes(ctx);
}
bool rgbd_point_renderer::enable(cgv::render::context& ctx)
{
	if (!point_renderer::enable(ctx))
		return false;
	set_rgbd_calibration_uniforms(ctx, ref_prog(), calib);
	ref_prog().set_uniform(ctx, "invalid_color", invalid_color);
	ref_prog().set_uniform(ctx, "discard_invalid_color_points", discard_invalid_color_points);
	ref_prog().set_uniform(ctx, "geometry_less_rendering", geometry_less_rendering);
	ref_prog().set_uniform(ctx, "do_lookup_color", lookup_color);
	if (use_undistortion_map) {
		if (undistortion_map_outofdate) {
			if (undistortion_tex.is_created())
				undistortion_tex.destruct(ctx);
			cgv::data::data_format df(calib.depth.w, calib.depth.h, cgv::type::info::TI_FLT32, cgv::data::CF_RG);
			cgv::data::data_view dv(&df, undistortion_map.data());
			undistortion_tex.create(ctx, dv, 0);
			undistortion_map_outofdate = false;
		}
		undistortion_tex.enable(ctx, 2);
		ref_prog().set_uniform(ctx, "undistortion_map", 2);
	}
	return true;
}
bool rgbd_point_renderer::disable(cgv::render::context& ctx)
{
	if (use_undistortion_map)
		undistortion_tex.disable(ctx);
	return point_renderer::disable(ctx);
}
void rgbd_point_renderer::draw(cgv::render::context& ctx, size_t start, size_t count, bool use_strips, bool use_adjacency, uint32_t strip_restart_index)
{
	if (geometry_less_rendering)
		draw_impl_instanced(ctx, cgv::render::PT_POINTS, 0, 1, calib.depth.w * calib.depth.h);
	else
		draw_impl(ctx, cgv::render::PT_POINTS, start, count);
}
// convenience function to add UI elements
void rgbd_point_renderer::create_gui(cgv::base::base* bp, cgv::gui::provider& p)
{
	p.add_member_control(bp, "geometry_less_rendering", geometry_less_rendering, "check");
	p.add_member_control(bp, "lookup_color", lookup_color, "check");
	p.add_member_control(bp, "discard_invalid_color_points", discard_invalid_color_points, "check");
	p.add_member_control(bp, "invalid_color", invalid_color);
}

}