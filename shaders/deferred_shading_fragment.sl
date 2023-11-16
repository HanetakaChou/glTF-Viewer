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

#include "deferred_shading_pipeline_resource_binding.sli"
#include "../thirdparty/Import-Asset/shaders/packed_vector.sli"
#include "../thirdparty/Import-Asset/shaders/octahedron_mapping.sli"
#include "math_constant.sli"

#define INVALID_GBUFFER_DEPTH 0.0

brx_root_signature(deferred_shading_root_signature_macro, deferred_shading_root_signature_name)
brx_early_depth_stencil
brx_pixel_shader_parameter_begin(main)
brx_pixel_shader_parameter_in_frag_coord brx_pixel_shader_parameter_split
brx_pixel_shader_parameter_out(brx_float4, out_color, 0)
brx_pixel_shader_parameter_end(main)
{
	// TODO: single pass deferred shading 
	
    brx_float depth = brx_load_2d(g_depth_texture[0], brx_int3(brx_frag_coord.xy, 0)).x;
   
    brx_branch
    if (INVALID_GBUFFER_DEPTH == depth)
    {
        out_color = brx_float4(0.0, 1.0, 0.0, 1.0);
        return;
    }

    brx_uint4 gbuffer = brx_load_2d(g_gbuffer_texture[0], brx_int3(brx_frag_coord.xy, 0));

    brx_float3 shading_normal_world_space = octahedron_unmap(R16G16_SNORM_to_FLOAT2(gbuffer.x));
    brx_float3 base_color = R10G10B10A2_UNORM_to_FLOAT4(gbuffer.y).xyz;
    brx_float2 metallic_roughness = R16G16_UNORM_to_FLOAT2(gbuffer.z);
    brx_float metallic = metallic_roughness.x;
    brx_float roughness = metallic_roughness.y;
    brx_float3 emissive = R10G10B10A2_UNORM_to_FLOAT4(gbuffer.w).xyz;
    
	const brx_float dielectric_specular = 0.04;
	// UE4: https://github.com/EpicGames/UnrealEngine/blob/4.21/Engine/Shaders/Private/MobileBasePassPixelShader.usf#L376
	brx_float3 f0 = brx_clamp((dielectric_specular - dielectric_specular * metallic) + base_color * metallic, 0.0, 1.0);
	brx_float3 albedo = brx_clamp(base_color - base_color * metallic, 0.0, 1.0);

    brx_float3 surface_position_world_space;
    {
        brx_float2 uv = brx_frag_coord.xy / brx_float2(g_screen_width, g_screen_height);

        brx_float surface_position_depth = depth;

        brx_float3 surface_position_ndc_space = brx_float3(uv * brx_float2(2.0, -2.0) + brx_float2(-1.0, 1.0), surface_position_depth);

        brx_float4 surface_position_view_space_with_w = brx_mul(g_inverse_projection_transform, brx_float4(surface_position_ndc_space, 1.0));

        brx_float3 surface_position_view_space = surface_position_view_space_with_w.xyz / surface_position_view_space_with_w.w;

        surface_position_world_space = brx_mul(g_inverse_view_transform, brx_float4(surface_position_view_space, 1.0)).xyz;
    }

    brx_float3 camera_ray_origin = brx_mul(g_inverse_view_transform, brx_float4(0.0, 0.0, 0.0, 1.0)).xyz;

	// TODO: shadow
	// TODO: environment lighting
	brx_float3 outgoing_radiance = brx_float3(0.0, 0.0, 0.0);

	const brx_int incident_light_count = 8;

	const brx_float3 incident_illuminances[incident_light_count] = brx_array_constructor_begin(brx_float3, incident_light_count) 
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.7, 0.7, 0.7) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) brx_array_constructor_split
		brx_float3(0.3, 0.3, 0.3) 
        brx_array_constructor_end;

	const brx_float3 Ls[incident_light_count] = brx_array_constructor_begin(brx_float3, incident_light_count) 
		brx_float3( 1.0,  1.0,  1.0) brx_array_constructor_split
		brx_float3( 1.0,  1.0, -1.0) brx_array_constructor_split
		brx_float3(-1.0,  1.0,  1.0) brx_array_constructor_split
		brx_float3(-1.0,  1.0, -1.0) brx_array_constructor_split
		brx_float3( 1.0, -1.0,  1.0) brx_array_constructor_split
		brx_float3( 1.0, -1.0, -1.0) brx_array_constructor_split
		brx_float3(-1.0, -1.0,  1.0) brx_array_constructor_split
		brx_float3(-1.0, -1.0, -1.0)
        brx_array_constructor_end;

	brx_unroll
	for(brx_int incident_light_index = 0; incident_light_index < incident_light_count; ++incident_light_index)
	{
		brx_float3 incident_illuminance = incident_illuminances[incident_light_index];
		brx_float3 L = brx_normalize(Ls[incident_light_index]);

		brx_float3 V = brx_normalize(camera_ray_origin - surface_position_world_space);
		brx_float3 N = shading_normal_world_space;
		brx_float3 H = normalize(L + V);
		brx_float NdotL = brx_clamp(dot(N, L), 0.0, 1.0);
		brx_float NdotH = brx_clamp(dot(N, H), 0.0, 1.0);
		brx_float NdotV = brx_clamp(dot(N, V), 0.0, 1.0);
		brx_float VdotH = brx_clamp(dot(V, H), 0.0, 1.0);

		brx_float3 brdf_diffuse;
		{
			// Lambert

			brdf_diffuse = (1.0 / M_PI) * albedo;
		}

		brx_float3 brdf_specular;
		{
			// Trowbridge Reitz		
		
			// Prevent the roughness to be zero
			// https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/CapsuleLightIntegrate.ush#L94
			const brx_float cvar_global_min_roughness_override = 0.02;
			roughness = brx_max(roughness, cvar_global_min_roughness_override);

			// Prevent the NdotV to be zero
			// https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L34
			NdotV = brx_max(NdotV, 1e-5);

			// Real-Time Rendering Fourth Edition / 9.8.1 Normal Distribution Functions: "In the Disney principled shading model, Burley[214] exposes the roughness control to users as g = r2, where r is the user-interface roughness parameter value between 0 and 1."
			brx_float alpha = roughness * roughness;
	
			// Equation 9.41 of Real-Time Rendering Fourth Edition: "Although **Trowbridge-Reitz distribution** is technically the correct name"
			// Equation 8.11 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MicrofacetDistributionFunctions
			brx_float alpha2 = alpha * alpha;
			brx_float denominator = 1.0 + NdotH * (NdotH * alpha2 - NdotH);
			brx_float D = (1.0 / M_PI) * (alpha2 / (denominator * denominator));

			// Lambda:
			// Equation 8.13 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
			// Equation 9.42 of Real-Time Rendering Fourth Edition
			// Figure 8.18 of PBR Book: https://pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models#MaskingandShadowing
			// Lambda(V) = 0.5*(-1.0 + (1.0/NoV)*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2))
			// Lambda(L) = 0.5*(-1.0 + (1.0/NoL)*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// G2
			// Equation 9.31 of Real-Time Rendering Fourth Edition
			// PBR Book / 8.4.3 Masking and Shadowing: "A more accurate model can be derived assuming that microfacet visibility is more likely the higher up a given point on a microface"
			// G2 = 1.0/(1.0 + Lambda(V) + Lambda(L)) = (2.0*NoV*NoL)/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// V = G2/(4.0*NoV*NoL) = 0.5/(NoL*sqrt(alpha^2 + (1.0 - alpha^2)*NoV^2) + NoV*sqrt(alpha^2 + (1.0 - alpha^2)*NoL^2))

			// float alpha2 = alpha * alpha;
			// float term_v = NdotL * sqrt(alpha2 + (1.0 - alpha2) * NdotV * NdotV);
			// float term_l = NdotV * sqrt(alpha2 + (1.0 - alpha2) * NdotL * NdotL);
			// UE: [Vis_SmithJointApprox](https://github.com/EpicGames/UnrealEngine/blob/4.27/Engine/Shaders/Private/BRDF.ush#L380)
			brx_float term_v = NdotL * (alpha + (1.0 - alpha) * NdotV);
			brx_float term_l = NdotV * (alpha + (1.0 - alpha) * NdotL);
			brx_float V = (0.5 / (term_v + term_l));

			// glTF Sample Renderer: [F_Schlick](https://github.com/KhronosGroup/glTF-Sample-Renderer/blob/e5646a2bf87b0871ba3f826fc2335fe117a11411/source/Renderer/shaders/brdf.glsl#L24)
			const brx_float3 f90 = brx_float3(1.0, 1.0, 1.0);

			brx_float x = brx_clamp(1.0 - VdotH, 0.0, 1.0);
			brx_float x2 = x * x;
			brx_float x5 = x * x2 * x2;
			brx_float3 F = f0 + (f90 - f0) * x5;

			brdf_specular = D * V * F;
		}
	
		 outgoing_radiance += (brdf_diffuse + brdf_specular) * (NdotL * incident_illuminance);
	}

    outgoing_radiance += emissive;

    out_color = brx_float4(outgoing_radiance, 1.0);
}
