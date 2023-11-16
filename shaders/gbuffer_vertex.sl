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
#include "../thirdparty/Import-Asset/shaders/packed_vector.sli"
#include "../thirdparty/Import-Asset/shaders/octahedron_mapping.sli"
#include "common_asset_constant.sli"

brx_root_signature(gbuffer_root_signature_macro, gbuffer_root_signature_name)
brx_vertex_shader_parameter_begin(main)
brx_vertex_shader_parameter_in_vertex_id brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out_position brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float3, out_vertex_normal, 0) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float4, out_vertex_tagent, 1) brx_vertex_shader_parameter_split
brx_vertex_shader_parameter_out(brx_float2, out_vertex_texcoord, 2)
brx_vertex_shader_parameter_end(main)
{
    brx_uint buffer_texture_flags = brx_byte_address_buffer_load(g_mesh_subset_buffers[3], 0);

    brx_uint vertex_index;
    brx_branch
    if(0u != (buffer_texture_flags & Buffer_Flag_Index_Type_UInt16))
    {
        brx_uint index_buffer_offset = (0u == ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_id)) & 3u)) ? (g_index_uint16_buffer_stride * brx_uint(brx_vertex_id)) : ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_id)) - 2u);
        brx_uint packed_vector_index_buffer = brx_byte_address_buffer_load(g_mesh_subset_buffers[2], index_buffer_offset);
        brx_uint2 unpacked_vector_index_buffer = R16G16_UINT_to_UINT2(packed_vector_index_buffer);
        vertex_index = (0u == ((g_index_uint16_buffer_stride * brx_uint(brx_vertex_id)) & 3u)) ? unpacked_vector_index_buffer.x : unpacked_vector_index_buffer.y;
    }
    else
    {
        brx_uint index_buffer_offset = g_index_uint32_buffer_stride * brx_uint(brx_vertex_id);
        vertex_index = brx_byte_address_buffer_load(g_mesh_subset_buffers[2], index_buffer_offset);
    }

    brx_float3 vertex_position_model_space;
    {
        brx_uint vertex_position_buffer_offset = g_vertex_position_buffer_stride * vertex_index;
        brx_uint3 packed_vector_vertex_position_binding = brx_byte_address_buffer_load3(g_mesh_subset_buffers[0], vertex_position_buffer_offset);
        vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_binding);
    }

    brx_float3 vertex_normal_model_space;
    brx_float4 vertex_tangent_model_space;
    brx_float2 vertex_texcoord;
    {
        brx_uint vertex_varying_buffer_offset = g_vertex_varying_buffer_stride * vertex_index;
        brx_uint3 packed_vector_vertex_varying_binding = brx_byte_address_buffer_load3(g_mesh_subset_buffers[1], vertex_varying_buffer_offset);
        vertex_normal_model_space = octahedron_unmap(R16G16_SNORM_to_FLOAT2(packed_vector_vertex_varying_binding.x));
        brx_float3 vertex_mapped_tangent_model_space = R15G15B2_SNORM_to_FLOAT3(packed_vector_vertex_varying_binding.y);
        vertex_tangent_model_space = brx_float4(octahedron_unmap(vertex_mapped_tangent_model_space.xy), vertex_mapped_tangent_model_space.z);
        vertex_texcoord = R16G16_UNORM_to_FLOAT2(packed_vector_vertex_varying_binding.z);
    }

    brx_float3 vertex_position_world_space = brx_mul(g_model_transform, brx_float4(vertex_position_model_space, 1.0)).xyz;
    brx_float3 vertex_position_view_space = brx_mul(g_view_transform, brx_float4(vertex_position_world_space, 1.0)).xyz;
    brx_float4 vertex_position_clip_space = brx_mul(g_projection_transform, brx_float4(vertex_position_view_space, 1.0));

    brx_float3 vertex_normal_world_space = brx_mul(g_model_transform, brx_float4(vertex_normal_model_space, 0.0)).xyz;
    
    brx_float4 vertex_tangent_world_space = brx_float4(brx_mul(g_model_transform, brx_float4(vertex_tangent_model_space.xyz, 0.0)).xyz, vertex_tangent_model_space.w);

    brx_position = vertex_position_clip_space;
    out_vertex_normal = vertex_normal_world_space;
    out_vertex_tagent = vertex_tangent_world_space;
    out_vertex_texcoord = vertex_texcoord;
}
