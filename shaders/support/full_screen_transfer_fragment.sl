//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "full_screen_transfer_pipeline_layout.sli"

brx_root_signature(full_screen_transfer_root_signature_macro, full_screen_transfer_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_image, 0)
brx_pixel_shader_parameter_end(main)
{
    // TODO: HDR swapchain
    brx_float3 color_linear = brx_load_2d(g_in_texture[0], brx_int3(brx_frag_coord.xy, 0)).xyz;
    brx_float3 color_srgb = brx_float3(brx_pow(brx_clamp(color_linear.x, 0.0, 1.0), (1.0 / 2.2)), brx_pow(brx_clamp(color_linear.y, 0.0, 1.0), (1.0 / 2.2)), brx_pow(brx_clamp(color_linear.z, 0.0, 1.0), (1.0 / 2.2)));
    out_image = brx_float4(color_srgb, 1.0);
}