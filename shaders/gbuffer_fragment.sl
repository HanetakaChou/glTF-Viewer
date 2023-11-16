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
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float3, in_interpolated_normal, 0) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float4, in_interpolated_tangent, 1) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_in(brx_float2, in_interpolated_texcoord, 2) brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_uint4, out_gbuffer, 0)
brx_pixel_shader_parameter_end(main)
{
    brx_float3 geometry_normal_world_space = brx_normalize(in_interpolated_normal);
    brx_float4 tangent_world_space = brx_float4(brx_normalize(in_interpolated_tangent.xyz), in_interpolated_tangent.w);
    brx_float2 texcoord = in_interpolated_texcoord;
    
    brx_uint buffer_texture_flags;
    brx_float normal_texture_scale;
    brx_float3 emissive_factor;
    brx_float3 base_color_factor;
    brx_float metallic_factor;
    brx_float roughness_factor;
    {
        brx_uint4 packed_vectors_information[3];
        packed_vectors_information[0] = brx_byte_address_buffer_load4(g_mesh_subset_buffers[3], 0);
        packed_vectors_information[1] = brx_byte_address_buffer_load4(g_mesh_subset_buffers[3], 4 * 4);
        packed_vectors_information[2].xy = brx_byte_address_buffer_load2(g_mesh_subset_buffers[3], 4 * 4 + 4 * 4);
        buffer_texture_flags = packed_vectors_information[0].x;
        normal_texture_scale = brx_uint_as_float(packed_vectors_information[0].y);
        emissive_factor = brx_uint_as_float(brx_uint3(packed_vectors_information[0].zw, packed_vectors_information[1].x));
        base_color_factor = brx_uint_as_float(packed_vectors_information[1].yzw);
        metallic_factor = brx_uint_as_float(packed_vectors_information[2].x);
        roughness_factor = brx_uint_as_float(packed_vectors_information[2].y);
    }

    brx_float3 shading_normal_world_space;
    brx_branch
    if(0u != (buffer_texture_flags & Texture_Flag_Enable_Normal_Texture))
    {
        // ["5.20.3. material.normalTextureInfo.scale" of "glTF 2.0 Specification"](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_normaltextureinfo_scale)
        brx_float3 shading_normal_tangent_space = brx_normalize((brx_sample_2d(g_mesh_subset_textures[0], g_sampler[0], texcoord).xyz * 2.0 - brx_float3(1.0, 1.0, 1.0)) * brx_float3(normal_texture_scale, normal_texture_scale, 1.0));
        brx_float3 bitangent_world_space = brx_cross(geometry_normal_world_space, tangent_world_space.xyz) * tangent_world_space.w;
        shading_normal_world_space = brx_normalize(tangent_world_space.xyz * shading_normal_tangent_space.x + bitangent_world_space * shading_normal_tangent_space.y + geometry_normal_world_space * shading_normal_tangent_space.z);  
    }
    else
    {
        shading_normal_world_space = geometry_normal_world_space;
    }
  
    brx_float3 emissive = emissive_factor;
    brx_branch
    if(0u != (buffer_texture_flags & Texture_Flag_Enable_Emissive_Texture))
    {
        emissive *= brx_sample_2d(g_mesh_subset_textures[1], g_sampler[0], texcoord).xyz;
    }

    brx_float3 base_color = base_color_factor;
    brx_branch
    if(0u != (buffer_texture_flags & Texture_Flag_Enable_Base_Colorl_Texture))
    {
        base_color *= brx_sample_2d(g_mesh_subset_textures[2], g_sampler[0], texcoord).xyz;
    }

    brx_float metallic = metallic_factor;
    brx_float roughness = roughness_factor;
    brx_branch
    if(0u != (buffer_texture_flags & Texture_Flag_Enable_Metallic_Roughness_Texture))
    {
        brx_float2 metallic_roughness = brx_sample_2d(g_mesh_subset_textures[3], g_sampler[0], texcoord).bg;
	    metallic *= metallic_roughness.x;
        roughness *= metallic_roughness.y;
    }

    // Specular Anti-Aliasing
    // "7.8.1 Mipmapping BRDF and Normal Maps" of "Real-Time Rendering Third Edition"
    // "9.13.1 Filtering Normals and Normal Distributions" of "Real-Time Rendering Fourth Edition"
    // UE4: [NormalCurvatureToRoughness](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BasePassPixelShader.usf#L67)  
    // U3D: [TextureNormalVariance ](https://github.com/Unity-Technologies/Graphics/blob/v10.8.1/com.unity.render-pipelines.core/ShaderLibrary/CommonMaterial.hlsl#L214)  
    {
        const brx_float cvar_normal_curvature_to_roughness_scale = 1.0;
        const brx_float cvar_normal_curvature_to_roughness_bias = 0.0;
        const brx_float cvar_normal_curvature_to_roughness_exponent = 0.3333333;
        
        brx_float3 dn_dx = brx_ddx(geometry_normal_world_space);
        brx_float3 dn_dy = brx_ddy(geometry_normal_world_space);
        brx_float x = brx_dot(dn_dx, dn_dx);
        brx_float y = brx_dot(dn_dy, dn_dy);

        brx_float curvature_approx = brx_pow(brx_max(x, y), cvar_normal_curvature_to_roughness_exponent);
        brx_float geometric_aa_roughness = brx_clamp(curvature_approx * cvar_normal_curvature_to_roughness_scale + cvar_normal_curvature_to_roughness_bias, 0.0, 1.0);

        roughness = brx_max(roughness, geometric_aa_roughness);
    }

    brx_uint packed_shading_normal_world_space = FLOAT2_to_R16G16_SNORM(octahedron_map(shading_normal_world_space));
    brx_uint packed_base_color = FLOAT4_to_R10G10B10A2_UNORM(brx_float4(base_color.x, base_color.y, base_color.z, 1.0));
    brx_uint packed_metallic_roughness = FLOAT2_to_R16G16_UNORM(brx_float2(metallic, roughness));
    brx_uint packed_emissive = FLOAT4_to_R10G10B10A2_UNORM(brx_float4(emissive.x, emissive.y, emissive.z, 1.0));

    out_gbuffer = brx_uint4(packed_shading_normal_world_space, packed_base_color, packed_metallic_roughness, packed_emissive);
}
