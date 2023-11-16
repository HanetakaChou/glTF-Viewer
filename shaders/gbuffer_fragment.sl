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

#include "gbuffer_pipeline_resource_binding.sli"
#include "octahedron_mapping.sli"
#include "packed_vector.sli"

brx_root_signature(gbuffer_root_signature_macro, gbuffer_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float4, in_interpolated_tangent, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_uint4, out_gbuffer, 0)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 geometry_normal_world_sapce = brx_normalize(in_interpolated_normal);
    brx_float4 tangent_world_sapce = brx_float4(brx_normalize(in_interpolated_tangent.xyz), in_interpolated_tangent.w);
    brx_float2 texcoord = in_interpolated_texcoord;

    brx_uint4 packed_material_buffer_vectors[3];
    packed_material_buffer_vectors[0] = brx_byte_address_buffer_load4(g_geometry_material_buffers[3], 0);
    packed_material_buffer_vectors[1] = brx_byte_address_buffer_load4(g_geometry_material_buffers[3], 4 * 4);
    packed_material_buffer_vectors[2].xy = brx_byte_address_buffer_load2(g_geometry_material_buffers[3], 4 * 4 + 4 * 4);
    brx_uint material_texture_enable_flags = packed_material_buffer_vectors[0].x;
    brx_float normal_texture_scale = brx_uint_as_float(packed_material_buffer_vectors[0].y);
    brx_float3 emissive_factor = brx_uint_as_float(brx_uint3(packed_material_buffer_vectors[0].zw, packed_material_buffer_vectors[1].x));
    brx_float3 base_color_factor = brx_uint_as_float(packed_material_buffer_vectors[1].yzw);
    brx_float metallic_factor = brx_uint_as_float(packed_material_buffer_vectors[2].x);
    brx_float roughness_factor = brx_uint_as_float(packed_material_buffer_vectors[2].y);

    brx_float3 shading_normal_world_space;
    brx_branch
    if(0u != (material_texture_enable_flags & Material_Texture_Enable_Normal))
    {
        // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
        brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(g_material_textures[0], g_sampler[0], texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_texture_scale, normal_texture_scale, 1.0));
        brx_float3 bitangent_world_sapce = brx_cross(geometry_normal_world_sapce, tangent_world_sapce.xyz) * tangent_world_sapce.w;
        shading_normal_world_space = brx_normalize(tangent_world_sapce.xyz * shading_normal_tangent_space.x + bitangent_world_sapce * shading_normal_tangent_space.y + geometry_normal_world_sapce * shading_normal_tangent_space.z);  
    }
    else
    {
        shading_normal_world_space = geometry_normal_world_sapce;
    }
  
    brx_float3 emissive = emissive_factor;
    brx_branch
    if(0u != (material_texture_enable_flags & Material_Texture_Enable_Emissive))
    {
        emissive *= brx_sample_2d(g_material_textures[1], g_sampler[0], texcoord).xyz;
    }

    brx_float3 base_color = base_color_factor;
    brx_branch
    if(0u != (material_texture_enable_flags & Material_Texture_Enable_Base_Color))
    {
        base_color *= brx_sample_2d(g_material_textures[2], g_sampler[0], texcoord).xyz;
    }

    brx_float metallic = metallic_factor;
    brx_float roughness = roughness_factor;
    brx_branch
    if(0u != (material_texture_enable_flags & Material_Texture_Enable_Base_Color))
    {
        brx_float2 metallic_roughness = brx_sample_2d(g_material_textures[3], g_sampler[0], texcoord).bg;
	    metallic *= metallic_roughness.x;
        roughness *= metallic_roughness.y;
    }

    // TODO: Specular Antialiasing
    // Toksvig

    brx_uint packed_shading_normal_world_space = FLOAT2_to_R16G16_SNORM(octahedron_map(shading_normal_world_space));
    brx_uint packed_base_color = FLOAT4_to_R10G10B10A2_UNORM(brx_float4(base_color.x, base_color.y, base_color.z, 1.0));
    brx_uint packed_metallic_roughness = FLOAT2_to_R16G16_UNORM(brx_float2(metallic, roughness));
    brx_uint packed_emissive = FLOAT4_to_R10G10B10A2_UNORM(brx_float4(emissive.x, emissive.y, emissive.z, 1.0));

    out_gbuffer = brx_uint4(packed_shading_normal_world_space, packed_base_color, packed_metallic_roughness, packed_emissive);
}