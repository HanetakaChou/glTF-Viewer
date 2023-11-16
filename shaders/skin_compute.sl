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

#include "skin_pipeline_resource_binding.sli"
#include "../thirdparty/ImportAsset/shaders/packed_vector.sli"
#include "../thirdparty/ImportAsset/shaders/octahedron_mapping.sli"
#include "common_constant.sli"

#if defined(GL_SPIRV) || defined(VULKAN)
#define brx_dual_quaternion mat2x4
#include "../thirdparty/DLB/DLB.glsli"
#elif defined(HLSL_VERSION) || defined(__HLSL_VERSION)
#define brx_dual_quaternion float2x4
#include "../thirdparty/DLB/DLB.hlsli"
#else
#error Unknown Compiler
#endif

#define THREAD_GROUP_X 1
#define THREAD_GROUP_Y 1
#define THREAD_GROUP_Z 1

brx_root_signature(skin_root_signature_macro, skin_root_signature_name)
brx_num_threads(THREAD_GROUP_X, THREAD_GROUP_Y, THREAD_GROUP_Z)
brx_compute_shader_parameter_begin(main)
brx_compute_shader_parameter_in_group_id
brx_pixel_shader_parameter_end(main)
{
    brx_uint vertex_index = brx_group_id.x;

    brx_uint vertex_position_buffer_offset = g_vertex_position_buffer_stride * vertex_index;
    brx_uint3 packed_vector_vertex_position_buffer = brx_byte_address_buffer_load3(g_geometry_buffers[0], vertex_position_buffer_offset);
    brx_float3 vertex_position_model_space = brx_uint_as_float(packed_vector_vertex_position_buffer);
    
    brx_uint vertex_varying_buffer_offset = g_vertex_varying_buffer_stride * vertex_index;
    brx_uint3 packed_vector_vertex_varying_buffer = brx_byte_address_buffer_load3(g_geometry_buffers[1], vertex_varying_buffer_offset);
    brx_float3 vertex_normal_model_space = R10G10B10A2_SNORM_to_FLOAT4(packed_vector_vertex_varying_buffer.x).xyz;
    brx_float4 vertex_tangent_model_space = R10G10B10A2_SNORM_to_FLOAT4(packed_vector_vertex_varying_buffer.y);

    brx_uint vertex_skinned_buffer_offset = g_vertex_skinned_buffer_stride * vertex_index;
    brx_uint3 packed_vector_vertex_skinned_buffer = brx_byte_address_buffer_load3(g_geometry_buffers[2], vertex_skinned_buffer_offset);

    brx_uint4 joint_indices = R16G16B16A16_UINT_to_UINT4(packed_vector_vertex_skinned_buffer.xy);
    brx_float4 joint_weights = R8G8B8A8_UNORM_to_FLOAT4(packed_vector_vertex_skinned_buffer.z);

    brx_dual_quaternion dual_quaternion_indices_x = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.x], g_dual_quaternions[2u * joint_indices.x + 1u]);  
    brx_dual_quaternion dual_quaternion_indices_y = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.y], g_dual_quaternions[2u * joint_indices.y + 1u]); 
    brx_dual_quaternion dual_quaternion_indices_z = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.z], g_dual_quaternions[2u * joint_indices.z + 1u]);
    brx_dual_quaternion dual_quaternion_indices_w = brx_dual_quaternion(g_dual_quaternions[2u * joint_indices.w], g_dual_quaternions[2u * joint_indices.w + 1u]);

    brx_dual_quaternion blend_dual_quaternion = dual_quaternion_linear_blending(dual_quaternion_indices_x, dual_quaternion_indices_y, dual_quaternion_indices_z, dual_quaternion_indices_w, joint_weights);

    brx_float4 blend_quaternion;
    brx_float3 blend_translation;
    unit_dual_quaternion_to_rigid_transform(blend_dual_quaternion, blend_quaternion, blend_translation);

    brx_float3 skined_vertex_position_model_space = unit_quaternion_to_rotation_transform(blend_quaternion, vertex_position_model_space) + blend_translation;    
    brx_uint3 packed_vector_skined_vertex_position_buffer = brx_float_as_uint(skined_vertex_position_model_space);
    brx_byte_address_buffer_store3(g_skinned_geometry_buffers[0], vertex_position_buffer_offset, packed_vector_skined_vertex_position_buffer);

    brx_float3 skined_vertex_normal_model_space = unit_quaternion_to_rotation_transform(blend_quaternion, vertex_normal_model_space);     
    brx_float4 skined_vertex_tangent_model_space = brx_float4(unit_quaternion_to_rotation_transform(blend_quaternion, vertex_tangent_model_space.xyz), vertex_tangent_model_space.w);
    brx_uint3 packed_vector_skined_vertex_varying_buffer = brx_uint3(FLOAT4_to_R10G10B10A2_SNORM(brx_float4(skined_vertex_normal_model_space, 0.0)), FLOAT4_to_R10G10B10A2_SNORM(skined_vertex_tangent_model_space), packed_vector_vertex_varying_buffer.z);
    brx_byte_address_buffer_store3(g_skinned_geometry_buffers[1], vertex_varying_buffer_offset, packed_vector_skined_vertex_varying_buffer);
}