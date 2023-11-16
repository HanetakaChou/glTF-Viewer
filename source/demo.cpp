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

#include "demo.h"
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#include <DirectXMath.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <assert.h>
#include "support/camera_controller.h"
#include "../thirdparty/ImportAsset/include/import_image_asset.h"
#include "../thirdparty/ImportAsset/include/import_asset_input_stream_file.h"
#include "../thirdparty/ImportAsset/shaders/octahedron_mapping.sli"
#include "../shaders/common_constant.sli"
#include "../shaders/skin_pipeline_resource_binding.sli"
#include "../shaders/common_resource_binding.sli"
#include "../shaders/gbuffer_pipeline_resource_binding.sli"
#include "../shaders/deferred_shading_pipeline_resource_binding.sli"

static inline uint32_t tbb_align_up(uint32_t value, uint32_t alignment);

static inline DirectX::XMMATRIX XM_CALLCONV DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(float FovAngleY, float AspectRatio, float NearZ, float FarZ);

static float g_camera_fov = -1.0F;

Demo::Demo()
{
}

void Demo::init(brx_device *device, std::vector<std::string> const &file_names)
{
	// Descriptor Layout
	brx_descriptor_set_layout *skin_pipeline_per_mesh_instance_update_descriptor_set_layout;
	brx_descriptor_set_layout *skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout;
	brx_descriptor_set_layout *gbuffer_pipeline_none_update_descriptor_set_layout;
	brx_descriptor_set_layout *gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout;
	brx_descriptor_set_layout *gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout;
	{
		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const skin_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
		skin_pipeline_per_mesh_instance_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(skin_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings) / sizeof(skin_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings[0]), skin_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings);

		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const skin_pipeline_per_mesh_skined_subset_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 3U},
			{1U, BRX_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2U}};
		skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(skin_pipeline_per_mesh_skined_subset_update_descriptor_set_layout_bindings) / sizeof(skin_pipeline_per_mesh_skined_subset_update_descriptor_set_layout_bindings[0]), skin_pipeline_per_mesh_skined_subset_update_descriptor_set_layout_bindings);

		brx_descriptor_set_layout *const skin_pipeline_descriptor_set_layouts[] = {
			skin_pipeline_per_mesh_instance_update_descriptor_set_layout, skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout};
		this->m_skin_pipeline_layout = device->create_pipeline_layout(sizeof(skin_pipeline_descriptor_set_layouts) / sizeof(skin_pipeline_descriptor_set_layouts[0]), skin_pipeline_descriptor_set_layouts);

		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const gbuffer_pipeline_none_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U},
			{1U, BRX_DESCRIPTOR_TYPE_SAMPLER, 1U}};
		gbuffer_pipeline_none_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(gbuffer_pipeline_none_update_descriptor_set_layout_bindings) / sizeof(gbuffer_pipeline_none_update_descriptor_set_layout_bindings[0]), gbuffer_pipeline_none_update_descriptor_set_layout_bindings);

		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, DEMO_MESH_SUBSET_GEOMETRY_METERIAL_BUFFER_COUNT},
			{4U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT}};
		gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout_bindings) / sizeof(gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout_bindings[0]), gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout_bindings);

		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U}};
		gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings) / sizeof(gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings[0]), gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout_bindings);

		brx_descriptor_set_layout *const gbuffer_pipeline_descriptor_set_layouts[] = {
			gbuffer_pipeline_none_update_descriptor_set_layout, gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout, gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout};
		this->m_gbuffer_pipeline_layout = device->create_pipeline_layout(sizeof(gbuffer_pipeline_descriptor_set_layouts) / sizeof(gbuffer_pipeline_descriptor_set_layouts[0]), gbuffer_pipeline_descriptor_set_layouts);

		BRX_DESCRIPTOR_SET_LAYOUT_BINDING const deferred_shading_pipeline_none_update_descriptor_set_layout_bindings[] = {
			{0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 1U},
			{1U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U},
			{2U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U}};
		this->m_deferred_shading_pipeline_none_update_descriptor_set_layout = device->create_descriptor_set_layout(sizeof(deferred_shading_pipeline_none_update_descriptor_set_layout_bindings) / sizeof(deferred_shading_pipeline_none_update_descriptor_set_layout_bindings[0]), deferred_shading_pipeline_none_update_descriptor_set_layout_bindings);

		brx_descriptor_set_layout *const deferred_shading_pipeline_descriptor_set_layouts[] = {
			this->m_deferred_shading_pipeline_none_update_descriptor_set_layout};
		this->m_deferred_shading_pipeline_layout = device->create_pipeline_layout(sizeof(deferred_shading_pipeline_descriptor_set_layouts) / sizeof(deferred_shading_pipeline_descriptor_set_layouts[0]), deferred_shading_pipeline_descriptor_set_layouts);
	}

	// Render Pass and Pipeline
	{
		// Skin Pipeline
		{
#include <skin_compute.inl>
			this->m_skin_pipeline = device->create_compute_pipeline(this->m_skin_pipeline_layout, sizeof(skin_compute_shader_module_code), skin_compute_shader_module_code);
		}

		// GBuffer Render Pass
		{
			this->m_gbuffer_color_attachement_format = BRX_COLOR_ATTACHMENT_FORMAT_R32G32B32A32_UINT;
			this->m_gbuffer_depth_attachement_format = device->get_depth_attachment_image_format();

			BRX_RENDER_PASS_COLOR_ATTACHMENT const color_attachment = {
				this->m_gbuffer_color_attachement_format,
				BRX_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
				BRX_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE};

			BRX_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const depth_stencil_attachment = {
				this->m_gbuffer_depth_attachement_format,
				BRX_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_CLEAR,
				BRX_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE};

			this->m_gbuffer_render_pass = device->create_render_pass(1U, &color_attachment, &depth_stencil_attachment);
		}

		// GBuffer Pipeline
		{
#include <gbuffer_vertex.inl>
#include <gbuffer_fragment.inl>
			this->m_gbuffer_pipeline = device->create_graphics_pipeline(this->m_gbuffer_render_pass, this->m_gbuffer_pipeline_layout, sizeof(gbuffer_vertex_shader_module_code), gbuffer_vertex_shader_module_code, sizeof(gbuffer_fragment_shader_module_code), gbuffer_fragment_shader_module_code, true, BRX_GRAPHICS_PIPELINE_COMPARE_OPERATION_GREATER);
		}

		// Deferred Shading Pass
		{
			this->m_deferred_shading_color_attachement_format = BRX_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32;

			BRX_RENDER_PASS_COLOR_ATTACHMENT const color_attachment = {
				this->m_deferred_shading_color_attachement_format,
				BRX_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR,
				BRX_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE};

			this->m_deferred_shading_render_pass = device->create_render_pass(1U, &color_attachment, NULL);
		}

		// Deferred Shading Pipeline
		{
#include <deferred_shading_vertex.inl>
#include <deferred_shading_fragment.inl>
			this->m_deferred_shading_pipeline = device->create_graphics_pipeline(this->m_deferred_shading_render_pass, this->m_deferred_shading_pipeline_layout, sizeof(deferred_shading_vertex_shader_module_code), deferred_shading_vertex_shader_module_code, sizeof(deferred_shading_fragment_shader_module_code), deferred_shading_fragment_shader_module_code, false, BRX_GRAPHICS_PIPELINE_COMPARE_OPERATION_ALWAYS);
		}
	}

	// Asset & Place Holder Texture
	{
		brx_upload_command_buffer *const upload_command_buffer = device->create_upload_command_buffer();

		brx_graphics_command_buffer *const graphics_command_buffer = device->create_graphics_command_buffer();

		brx_upload_queue *const upload_queue = device->create_upload_queue();

		brx_graphics_queue *const graphics_queue = device->create_graphics_queue();

		brx_fence *const fence = device->create_fence(true);

		{
			std::vector<brx_staging_upload_buffer *> staging_upload_buffers;

			device->reset_upload_command_buffer(upload_command_buffer);

			device->reset_graphics_command_buffer(graphics_command_buffer);

			upload_command_buffer->begin();

			graphics_command_buffer->begin();

			// 60 FPS
			constexpr float const frame_rate = 60.0F;

			// Asset
			for (size_t file_name_index = 0U; file_name_index < file_names.size(); ++file_name_index)
			{
				std::string const &file_name = file_names[file_name_index];

				std::vector<scene_mesh_data> total_mesh_data;
				if (import_scene_asset_gltf(total_mesh_data, frame_rate, file_name.c_str()))
				{
					size_t mesh_index_offset = this->m_scene_meshes.size();

					this->m_scene_meshes.resize(mesh_index_offset + total_mesh_data.size());

					for (size_t mesh_index = 0U; mesh_index < total_mesh_data.size(); ++mesh_index)
					{
						scene_mesh_data const &in_mesh_data = total_mesh_data[mesh_index];

						Demo_Mesh &out_mesh = this->m_scene_meshes[mesh_index_offset + mesh_index];

						out_mesh.m_skinned = in_mesh_data.m_skinned;

						out_mesh.m_subsets.resize(in_mesh_data.m_subsets.size());

						for (size_t subset_index = 0U; subset_index < in_mesh_data.m_subsets.size(); ++subset_index)
						{
							scene_mesh_subset_data const &in_subset_data = in_mesh_data.m_subsets[subset_index];

							Demo_Mesh_Subset &out_subset = out_mesh.m_subsets[subset_index];

							uint32_t const vertex_count = static_cast<uint32_t>(in_subset_data.m_vertex_position_binding.size());

							{
								uint32_t const vertex_position_buffer_size = sizeof(scene_mesh_vertex_position_binding) * vertex_count;

								out_subset.m_vertex_position_buffer = device->create_storage_asset_buffer(vertex_position_buffer_size);

								brx_staging_upload_buffer *const vertex_position_staging_upload_buffer = device->create_staging_upload_buffer(vertex_position_buffer_size);

								staging_upload_buffers.push_back(vertex_position_staging_upload_buffer);

								std::memcpy(vertex_position_staging_upload_buffer->get_host_memory_range_base(), in_subset_data.m_vertex_position_binding.data(), vertex_position_buffer_size);

								upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_vertex_position_buffer, 0U, vertex_position_staging_upload_buffer, 0U, vertex_position_buffer_size);
							}

							{
								assert(vertex_count == in_subset_data.m_vertex_varying_binding.size());

								uint32_t const vertex_varying_buffer_size = sizeof(scene_mesh_vertex_varying_binding) * vertex_count;

								out_subset.m_vertex_varying_buffer = device->create_storage_asset_buffer(vertex_varying_buffer_size);

								brx_staging_upload_buffer *const vertex_varying_staging_upload_buffer = device->create_staging_upload_buffer(vertex_varying_buffer_size);

								staging_upload_buffers.push_back(vertex_varying_staging_upload_buffer);

								std::memcpy(vertex_varying_staging_upload_buffer->get_host_memory_range_base(), in_subset_data.m_vertex_varying_binding.data(), vertex_varying_buffer_size);

								upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_vertex_varying_buffer, 0U, vertex_varying_staging_upload_buffer, 0U, vertex_varying_buffer_size);
							}

							if (!in_mesh_data.m_skinned)
							{
								out_subset.m_vertex_skinned_binding_buffer = NULL;
							}
							else
							{

								assert(vertex_count == in_subset_data.m_vertex_skinned_binding.size());

								uint32_t const vertex_skinned_buffer_size = sizeof(scene_mesh_vertex_skinned_binding) * vertex_count;

								out_subset.m_vertex_skinned_binding_buffer = device->create_storage_asset_buffer(vertex_skinned_buffer_size);

								brx_staging_upload_buffer *const vertex_skinned_staging_upload_buffer = device->create_staging_upload_buffer(vertex_skinned_buffer_size);

								staging_upload_buffers.push_back(vertex_skinned_staging_upload_buffer);

								std::memcpy(vertex_skinned_staging_upload_buffer->get_host_memory_range_base(), in_subset_data.m_vertex_skinned_binding.data(), vertex_skinned_buffer_size);

								upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_vertex_skinned_binding_buffer, 0U, vertex_skinned_staging_upload_buffer, 0U, vertex_skinned_buffer_size);
							}

							out_subset.m_index_count = static_cast<uint32_t>(in_subset_data.m_indices.size());
							{
								if (in_subset_data.m_max_index <= static_cast<uint32_t>(UINT16_MAX))
								{
									std::vector<uint16_t> converted_indices(static_cast<size_t>(out_subset.m_index_count));
									for (uint32_t index_index = 0; index_index < out_subset.m_index_count; ++index_index)
									{
										converted_indices[index_index] = static_cast<uint32_t>(in_subset_data.m_indices[index_index]);
									}

									uint32_t const index_buffer_size = sizeof(uint16_t) * out_subset.m_index_count;

									out_subset.m_index_buffer = device->create_storage_asset_buffer(index_buffer_size);

									brx_staging_upload_buffer *const index_staging_upload_buffer = device->create_staging_upload_buffer(index_buffer_size);

									staging_upload_buffers.push_back(index_staging_upload_buffer);

									std::memcpy(index_staging_upload_buffer->get_host_memory_range_base(), converted_indices.data(), index_buffer_size);

									upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_index_buffer, 0U, index_staging_upload_buffer, 0U, index_buffer_size);
								}
								else
								{
									uint32_t const index_buffer_size = sizeof(uint32_t) * out_subset.m_index_count;

									out_subset.m_index_buffer = device->create_storage_asset_buffer(index_buffer_size);

									brx_staging_upload_buffer *const index_staging_upload_buffer = device->create_staging_upload_buffer(index_buffer_size);

									staging_upload_buffers.push_back(index_staging_upload_buffer);

									std::memcpy(index_staging_upload_buffer->get_host_memory_range_base(), in_subset_data.m_indices.data(), index_buffer_size);

									upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_index_buffer, 0U, index_staging_upload_buffer, 0U, index_buffer_size);
								}
							}

							{
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags = 0U;
								if (in_subset_data.m_max_index <= static_cast<uint32_t>(UINT16_MAX))
								{
									gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags |= Geometry_Flag_Index_Type_UInt16;
								}
								if (!in_subset_data.m_normal_texture_image_uri.empty())
								{
									gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags |= Material_Flag_Enable_Normal_Texture;
								}
								if (!in_subset_data.m_emissive_texture_image_uri.empty())
								{
									gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags |= Material_Flag_Enable_Emissive_Texture;
								}
								if (!in_subset_data.m_base_color_texture_image_uri.empty())
								{
									gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags |= Material_Flag_Enable_Base_Colorl_Texture;
								}
								if (!in_subset_data.m_metallic_roughness_texture_image_uri.empty())
								{
									gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_geometry_material_flags |= Material_Flag_Enable_Metallic_Roughness_Texture;
								}
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_normal_texture_scale = in_subset_data.m_normal_texture_scale;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_emissive_factor_x = in_subset_data.m_emissive_factor.x;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_emissive_factor_y = in_subset_data.m_emissive_factor.y;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_emissive_factor_z = in_subset_data.m_emissive_factor.z;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_base_color_factor_x = in_subset_data.m_base_color_factor.x;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_base_color_factor_y = in_subset_data.m_base_color_factor.y;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_base_color_factor_z = in_subset_data.m_base_color_factor.z;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_metallic_factor = in_subset_data.m_metallic_factor;
								gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination.m_roughness_factor = in_subset_data.m_roughness_factor;

								uint32_t const material_buffer_size = sizeof(gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination);

								out_subset.m_material_buffer = device->create_storage_asset_buffer(material_buffer_size);

								brx_staging_upload_buffer *const material_staging_upload_buffer = device->create_staging_upload_buffer(material_buffer_size);

								staging_upload_buffers.push_back(material_staging_upload_buffer);

								std::memcpy(material_staging_upload_buffer->get_host_memory_range_base(), &gbuffer_pipeline_per_mesh_subset_update_set_geometry_material_storage_buffer_binding_destination, material_buffer_size);

								upload_command_buffer->upload_from_staging_upload_buffer_to_storage_asset_buffer(out_subset.m_material_buffer, 0U, material_staging_upload_buffer, 0U, material_buffer_size);
							}

							{
								uint32_t const staging_upload_buffer_offset_alignment = device->get_staging_upload_buffer_offset_alignment();
								uint32_t const staging_upload_buffer_row_pitch_alignment = device->get_staging_upload_buffer_row_pitch_alignment();

								std::string const *const meterial_texture_image_uris[] = {
									&in_subset_data.m_normal_texture_image_uri,
									&in_subset_data.m_emissive_texture_image_uri,
									&in_subset_data.m_base_color_texture_image_uri,
									&in_subset_data.m_metallic_roughness_texture_image_uri};

								for (uint32_t mesh_subset_meterial_texture_index = 0U; mesh_subset_meterial_texture_index < DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT; ++mesh_subset_meterial_texture_index)
								{
									std::string const &meterial_texture_image_uri = (*meterial_texture_image_uris[mesh_subset_meterial_texture_index]);

									if (!meterial_texture_image_uri.empty())
									{
										std::string image_asset_file_name_dds;
										std::string image_asset_file_name_pvr;
										{
											std::string image_asset_file_name;

											size_t dir_name_pos = file_name.find_last_of("/\\");
											if (std::string::npos != dir_name_pos)
											{
												image_asset_file_name = file_name.substr(0U, dir_name_pos + 1U);
											}
											else
											{
												image_asset_file_name += "./";
											}

											size_t ext_name_pos = meterial_texture_image_uri.find_last_of(".");
											if (std::string::npos != ext_name_pos)
											{
												image_asset_file_name += meterial_texture_image_uri.substr(0U, ext_name_pos + 1U);
											}
											else
											{
												image_asset_file_name += meterial_texture_image_uri;
											}

											image_asset_file_name_dds = (image_asset_file_name + "dds");
											image_asset_file_name_pvr = (image_asset_file_name + "pvr");
										}

										import_asset_input_stream_file import_image_asset_file_input_stream;
										if (device->is_sampled_asset_image_compression_bc_supported() && import_image_asset_file_input_stream.init(image_asset_file_name_dds.c_str()))
										{
											BRX_SAMPLED_ASSET_IMAGE_IMPORT_HEADER image_asset_header;
											std::vector<BRX_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST> subresource_memcpy_dests;

											size_t image_asset_data_offset;
											bool const res_import_image_asset_header = import_image_asset_header_from_input_stream_dds(&import_image_asset_file_input_stream, &image_asset_header, &image_asset_data_offset);
											assert(res_import_image_asset_header);

											uint32_t const subresource_count = image_asset_header.mip_levels;
											subresource_memcpy_dests.resize(subresource_count);

											// TODO: support more image paramters
											assert(!image_asset_header.is_cube_map);
											assert(BRX_SAMPLED_ASSET_IMAGE_TYPE_2D == image_asset_header.type);
											assert(1U == image_asset_header.depth);
											assert(1U == image_asset_header.array_layers);
											uint32_t const total_bytes = brx_sampled_asset_image_import_calculate_subresource_memcpy_dests(image_asset_header.format, image_asset_header.width, image_asset_header.height, 1U, image_asset_header.mip_levels, 1U, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, subresource_count, &subresource_memcpy_dests[0]);
											brx_staging_upload_buffer *const image_staging_upload_buffer = device->create_staging_upload_buffer(static_cast<uint32_t>(total_bytes));
											staging_upload_buffers.push_back(image_staging_upload_buffer);

											bool const res_import_image_asset_data = import_image_asset_data_from_input_stream_dds(&import_image_asset_file_input_stream, &image_asset_header, image_asset_data_offset, image_staging_upload_buffer->get_host_memory_range_base(), subresource_count, &subresource_memcpy_dests[0]);
											assert(res_import_image_asset_data);

											import_image_asset_file_input_stream.uninit();

											out_subset.m_material_textures[mesh_subset_meterial_texture_index] = device->create_sampled_asset_image(image_asset_header.format, image_asset_header.width, image_asset_header.height, image_asset_header.mip_levels);

											for (uint32_t mip_level = 0U; mip_level < image_asset_header.mip_levels; ++mip_level)
											{
												upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(out_subset.m_material_textures[mesh_subset_meterial_texture_index], image_asset_header.format, image_asset_header.width, image_asset_header.height, mip_level, image_staging_upload_buffer, subresource_memcpy_dests[mip_level].staging_upload_buffer_offset, subresource_memcpy_dests[mip_level].output_row_pitch, subresource_memcpy_dests[mip_level].output_row_count);
											}
										}
										else if (device->is_sampled_asset_image_compression_astc_supported() && import_image_asset_file_input_stream.init(image_asset_file_name_pvr.c_str()))
										{
											BRX_SAMPLED_ASSET_IMAGE_IMPORT_HEADER image_asset_header;
											std::vector<BRX_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST> subresource_memcpy_dests;

											size_t image_asset_data_offset;
											bool const res_import_image_asset_header = import_image_asset_header_from_input_stream_pvr(&import_image_asset_file_input_stream, &image_asset_header, &image_asset_data_offset);
											assert(res_import_image_asset_header);

											uint32_t const subresource_count = image_asset_header.mip_levels;
											subresource_memcpy_dests.resize(subresource_count);

											// TODO: support more image paramters
											assert(!image_asset_header.is_cube_map);
											assert(BRX_SAMPLED_ASSET_IMAGE_TYPE_2D == image_asset_header.type);
											assert(1U == image_asset_header.depth);
											assert(1U == image_asset_header.array_layers);
											uint32_t const total_bytes = brx_sampled_asset_image_import_calculate_subresource_memcpy_dests(image_asset_header.format, image_asset_header.width, image_asset_header.height, 1U, image_asset_header.mip_levels, 1U, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, subresource_count, &subresource_memcpy_dests[0]);
											brx_staging_upload_buffer *const image_staging_upload_buffer = device->create_staging_upload_buffer(static_cast<uint32_t>(total_bytes));
											staging_upload_buffers.push_back(image_staging_upload_buffer);

											bool const res_import_image_asset_data = import_image_asset_data_from_input_stream_pvr(&import_image_asset_file_input_stream, &image_asset_header, image_asset_data_offset, image_staging_upload_buffer->get_host_memory_range_base(), subresource_count, &subresource_memcpy_dests[0]);
											assert(res_import_image_asset_data);

											import_image_asset_file_input_stream.uninit();

											out_subset.m_material_textures[mesh_subset_meterial_texture_index] = device->create_sampled_asset_image(image_asset_header.format, image_asset_header.width, image_asset_header.height, image_asset_header.mip_levels);

											for (uint32_t mip_level = 0U; mip_level < image_asset_header.mip_levels; ++mip_level)
											{
												upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(out_subset.m_material_textures[mesh_subset_meterial_texture_index], image_asset_header.format, image_asset_header.width, image_asset_header.height, mip_level, image_staging_upload_buffer, subresource_memcpy_dests[mip_level].staging_upload_buffer_offset, subresource_memcpy_dests[mip_level].output_row_pitch, subresource_memcpy_dests[mip_level].output_row_count);
											}
										}
										else
										{
											// TODO: jpeg
											// TODO: png
											out_subset.m_material_textures[mesh_subset_meterial_texture_index] = NULL;
										}
									}
									else
									{
										out_subset.m_material_textures[mesh_subset_meterial_texture_index] = NULL;
									}
								}
							}
						}

						out_mesh.m_instances.resize(total_mesh_data[mesh_index].m_instances.size());

						for (size_t instance_index = 0U; instance_index < total_mesh_data[mesh_index].m_instances.size(); ++instance_index)
						{
							scene_mesh_instance_data &in_instance_data = total_mesh_data[mesh_index].m_instances[instance_index];

							Demo_Mesh_Instance &out_mesh_instance = out_mesh.m_instances[instance_index];

							out_mesh_instance.m_model_transform = std::move(in_instance_data.m_model_transform);
							out_mesh_instance.m_animation_skeleton = std::move(in_instance_data.m_animation_skeleton);

							if (!in_mesh_data.m_skinned)
							{
								assert(out_mesh_instance.m_subsets.empty());
							}
							else
							{
								out_mesh_instance.m_subsets.resize(in_mesh_data.m_subsets.size());

								for (size_t subset_index = 0U; subset_index < in_mesh_data.m_subsets.size(); ++subset_index)
								{
									scene_mesh_subset_data const &in_subset_data = in_mesh_data.m_subsets[subset_index];

									Demo_Mesh_Skinned_Subset &out_mesh_skinned_subset = out_mesh_instance.m_subsets[subset_index];

									uint32_t const vertex_count = static_cast<uint32_t>(in_subset_data.m_vertex_position_binding.size());

									uint32_t const vertex_position_buffer_size = sizeof(scene_mesh_vertex_position_binding) * vertex_count;

									out_mesh_skinned_subset.m_skinned_vertex_position_buffer = device->create_storage_intermediate_buffer(vertex_position_buffer_size);

									assert(vertex_count == in_subset_data.m_vertex_varying_binding.size());

									uint32_t const vertex_varying_buffer_size = sizeof(scene_mesh_vertex_varying_binding) * vertex_count;

									out_mesh_skinned_subset.m_skinned_vertex_varying_buffer = device->create_storage_intermediate_buffer(vertex_varying_buffer_size);
								}
							}
						}
					}
				}
			}

			// Place Holder Texture
			{
				uint32_t const staging_upload_buffer_offset_alignment = device->get_staging_upload_buffer_offset_alignment();
				uint32_t const staging_upload_buffer_row_pitch_alignment = device->get_staging_upload_buffer_row_pitch_alignment();

				BRX_SAMPLED_ASSET_IMAGE_FORMAT const format = BRX_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM;
				uint32_t const width = 1U;
				uint32_t const height = 1U;
				uint32_t const mip_levels = 1U;
				this->m_place_holder_texture = device->create_sampled_asset_image(format, width, height, mip_levels);

				std::vector<BRX_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST> subresource_memcpy_dests;
				uint32_t const subresource_count = mip_levels;
				subresource_memcpy_dests.resize(subresource_count);

				uint32_t total_bytes = brx_sampled_asset_image_import_calculate_subresource_memcpy_dests(format, width, height, 1U, mip_levels, 1U, 0U, staging_upload_buffer_offset_alignment, staging_upload_buffer_row_pitch_alignment, subresource_count, &subresource_memcpy_dests[0]);
				brx_staging_upload_buffer *place_holder_image_staging_upload_buffer = device->create_staging_upload_buffer(total_bytes);

				staging_upload_buffers.push_back(place_holder_image_staging_upload_buffer);

				uint32_t const mip_level = 0U;
				uint32_t const subresource_index = brx_sampled_asset_image_import_calculate_subresource_index(mip_level, 0U, 0U, mip_levels, 1U);
				for (uint32_t output_slice_index = 0U; output_slice_index < subresource_memcpy_dests[mip_level].output_slice_count; ++output_slice_index)
				{
					for (uint32_t output_row_index = 0U; output_row_index < subresource_memcpy_dests[mip_level].output_row_count; ++output_row_index)
					{
						void *destination = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(place_holder_image_staging_upload_buffer->get_host_memory_range_base()) + (subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset + subresource_memcpy_dests[subresource_index].output_slice_pitch * output_slice_index + subresource_memcpy_dests[subresource_index].output_row_pitch * output_row_index));

						std::memset(destination, 0, subresource_memcpy_dests[mip_level].output_row_size);
					}
				}

				upload_command_buffer->upload_from_staging_upload_buffer_to_sampled_asset_image(this->m_place_holder_texture, format, width, height, mip_level, place_holder_image_staging_upload_buffer, subresource_memcpy_dests[mip_level].staging_upload_buffer_offset, subresource_memcpy_dests[mip_level].output_row_pitch, subresource_memcpy_dests[mip_level].output_row_count);
			}

			// store
			// release
			// acquire
			{
				std::vector<brx_storage_asset_buffer const *> uploaded_storage_asset_buffers;
				std::vector<brx_sampled_asset_image const *> uploaded_sampled_asset_images;
				std::vector<uint32_t> uploaded_destination_mip_levels;

				// Asset
				for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
				{
					Demo_Mesh const &scene_mesh = this->m_scene_meshes[mesh_index];

					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Subset const &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

						uploaded_storage_asset_buffers.push_back(scene_mesh_subset.m_vertex_position_buffer);

						uploaded_storage_asset_buffers.push_back(scene_mesh_subset.m_vertex_varying_buffer);

						if (scene_mesh.m_skinned)
						{
							uploaded_storage_asset_buffers.push_back(scene_mesh_subset.m_vertex_skinned_binding_buffer);
						}

						uploaded_storage_asset_buffers.push_back(scene_mesh_subset.m_index_buffer);

						uploaded_storage_asset_buffers.push_back(scene_mesh_subset.m_material_buffer);

						for (uint32_t mesh_subset_meterial_texture_index = 0U; mesh_subset_meterial_texture_index < DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT; ++mesh_subset_meterial_texture_index)
						{
							brx_sampled_asset_image *const material_texture = scene_mesh_subset.m_material_textures[mesh_subset_meterial_texture_index];

							if (NULL != material_texture)
							{
								uint32_t const mip_level_count = material_texture->get_mip_levels();

								for (uint32_t mip_level = 0U; mip_level < mip_level_count; ++mip_level)
								{
									uploaded_sampled_asset_images.push_back(material_texture);

									uploaded_destination_mip_levels.push_back(mip_level);
								}
							}
						}
					}
				}

				// Place Holder Texture
				{
					uploaded_sampled_asset_images.push_back(this->m_place_holder_texture);

					uploaded_destination_mip_levels.push_back(0U);
				}

				assert(uploaded_sampled_asset_images.size() == uploaded_destination_mip_levels.size());

				upload_command_buffer->release(static_cast<uint32_t>(uploaded_storage_asset_buffers.size()), uploaded_storage_asset_buffers.data(), static_cast<uint32_t>(uploaded_sampled_asset_images.size()), &uploaded_sampled_asset_images[0], uploaded_destination_mip_levels.data(), 0U, NULL);

				graphics_command_buffer->acquire(static_cast<uint32_t>(uploaded_storage_asset_buffers.size()), uploaded_storage_asset_buffers.data(), static_cast<uint32_t>(uploaded_sampled_asset_images.size()), &uploaded_sampled_asset_images[0], uploaded_destination_mip_levels.data(), 0U, NULL);
			}

			upload_command_buffer->end();

			graphics_command_buffer->end();

			upload_queue->submit_and_signal(upload_command_buffer);

			device->reset_fence(fence);

			graphics_queue->wait_and_submit(upload_command_buffer, graphics_command_buffer, fence);

			device->wait_for_fence(fence);

			for (brx_staging_upload_buffer *const staging_upload_buffer : staging_upload_buffers)
			{
				device->destroy_staging_upload_buffer(staging_upload_buffer);
			}

			staging_upload_buffers.clear();
		}

		device->destroy_fence(fence);

		device->destroy_upload_command_buffer(upload_command_buffer);

		device->destroy_graphics_command_buffer(graphics_command_buffer);

		device->destroy_upload_queue(upload_queue);

		device->destroy_graphics_queue(graphics_queue);
	}

	// Sampler
	{
		this->m_sampler = device->create_sampler(BRX_SAMPLER_FILTER_LINEAR);
	}

	// Uniform Buffer
	{
		this->m_uniform_upload_buffer_offset_alignment = device->get_uniform_upload_buffer_offset_alignment();

		this->m_common_none_update_uniform_buffer = device->create_uniform_upload_buffer(tbb_align_up(static_cast<uint32_t>(sizeof(common_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * FRAME_THROTTLING_COUNT);

		for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
		{
			Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

			for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
			{
				Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

				if (!scene_mesh.m_skinned)
				{
					scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_uniform_buffer = NULL;
				}
				else
				{
					scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_uniform_buffer = device->create_uniform_upload_buffer(tbb_align_up(static_cast<uint32_t>(sizeof(skin_pipeline_per_mesh_instance_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * FRAME_THROTTLING_COUNT);
				}

				scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer = device->create_uniform_upload_buffer(tbb_align_up(static_cast<uint32_t>(sizeof(gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * FRAME_THROTTLING_COUNT);
			}
		}
	}

	// Descriptor
	{
		// GBuffer Pipeline
		{
			{
				this->m_gbuffer_pipeline_none_update_descriptor_set = device->create_descriptor_set(gbuffer_pipeline_none_update_descriptor_set_layout);

				constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(common_none_update_set_uniform_buffer_binding);
				device->write_descriptor_set(this->m_gbuffer_pipeline_none_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_common_none_update_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);

				device->write_descriptor_set(this->m_gbuffer_pipeline_none_update_descriptor_set, 1U, BRX_DESCRIPTOR_TYPE_SAMPLER, 0U, 1U, NULL, NULL, NULL, NULL, NULL, NULL, &this->m_sampler, NULL);
			}

			for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
			{
				Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

				if (!scene_mesh.m_skinned)
				{
					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Subset &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

						scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set = device->create_descriptor_set(gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout);

						brx_read_only_storage_buffer const *const read_only_storage_buffers[] = {
							scene_mesh_subset.m_vertex_position_buffer->get_read_only_storage_buffer(),
							scene_mesh_subset.m_vertex_varying_buffer->get_read_only_storage_buffer(),
							scene_mesh_subset.m_index_buffer->get_read_only_storage_buffer(),
							scene_mesh_subset.m_material_buffer->get_read_only_storage_buffer()};
						device->write_descriptor_set(scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(read_only_storage_buffers) / sizeof(read_only_storage_buffers[0]), NULL, NULL, read_only_storage_buffers, NULL, NULL, NULL, NULL, NULL);

						brx_sampled_image const *sample_images[DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT];
						for (uint32_t mesh_subset_meterial_texture_index = 0U; mesh_subset_meterial_texture_index < DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT; ++mesh_subset_meterial_texture_index)
						{
							brx_sampled_asset_image *material_texture = scene_mesh_subset.m_material_textures[mesh_subset_meterial_texture_index];

							sample_images[mesh_subset_meterial_texture_index] = (NULL != material_texture) ? material_texture->get_sampled_image() : this->m_place_holder_texture->get_sampled_image();
						}
						device->write_descriptor_set(scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set, 4U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sample_images) / sizeof(sample_images[0]), NULL, NULL, NULL, NULL, sample_images, NULL, NULL, NULL);
					}

					for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
					{
						Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

						assert(scene_mesh_instance.m_subsets.empty());

						scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_descriptor_set = NULL;

						scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set = device->create_descriptor_set(gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout);

						constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding);
						device->write_descriptor_set(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
					}
				}
				else
				{
					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Subset &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

						scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set = NULL;
					}

					for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
					{
						Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

						assert(scene_mesh.m_subsets.size() == scene_mesh_instance.m_subsets.size());

						for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
						{
							Demo_Mesh_Subset const &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

							Demo_Mesh_Skinned_Subset &scene_mesh_skinned_subset = scene_mesh_instance.m_subsets[subset_index];

							{
								scene_mesh_skinned_subset.m_skin_pipeline_per_mesh_skinned_subset_update_descriptor_set = device->create_descriptor_set(skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout);

								brx_read_only_storage_buffer const *const read_only_storage_buffers[] = {
									scene_mesh_subset.m_vertex_position_buffer->get_read_only_storage_buffer(),
									scene_mesh_subset.m_vertex_varying_buffer->get_read_only_storage_buffer(),
									scene_mesh_subset.m_vertex_skinned_binding_buffer->get_read_only_storage_buffer()};
								device->write_descriptor_set(scene_mesh_skinned_subset.m_skin_pipeline_per_mesh_skinned_subset_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(read_only_storage_buffers) / sizeof(read_only_storage_buffers[0]), NULL, NULL, read_only_storage_buffers, NULL, NULL, NULL, NULL, NULL);

								brx_storage_buffer const *const storage_buffers[] = {
									scene_mesh_skinned_subset.m_skinned_vertex_position_buffer->get_storage_buffer(),
									scene_mesh_skinned_subset.m_skinned_vertex_varying_buffer->get_storage_buffer()};
								device->write_descriptor_set(scene_mesh_skinned_subset.m_skin_pipeline_per_mesh_skinned_subset_update_descriptor_set, 1U, BRX_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0U, sizeof(storage_buffers) / sizeof(storage_buffers[0]), NULL, NULL, NULL, storage_buffers, NULL, NULL, NULL, NULL);
							}

							{
								scene_mesh_skinned_subset.m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set = device->create_descriptor_set(gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout);

								brx_read_only_storage_buffer const *const read_only_storage_buffers[] = {
									scene_mesh_skinned_subset.m_skinned_vertex_position_buffer->get_read_only_storage_buffer(),
									scene_mesh_skinned_subset.m_skinned_vertex_varying_buffer->get_read_only_storage_buffer(),
									scene_mesh_subset.m_index_buffer->get_read_only_storage_buffer(),
									scene_mesh_subset.m_material_buffer->get_read_only_storage_buffer()};
								device->write_descriptor_set(scene_mesh_skinned_subset.m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER, 0U, sizeof(read_only_storage_buffers) / sizeof(read_only_storage_buffers[0]), NULL, NULL, read_only_storage_buffers, NULL, NULL, NULL, NULL, NULL);

								brx_sampled_image const *sample_images[DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT];
								for (uint32_t mesh_subset_meterial_texture_index = 0U; mesh_subset_meterial_texture_index < DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT; ++mesh_subset_meterial_texture_index)
								{
									brx_sampled_asset_image *material_texture = scene_mesh_subset.m_material_textures[mesh_subset_meterial_texture_index];

									sample_images[mesh_subset_meterial_texture_index] = (NULL != material_texture) ? material_texture->get_sampled_image() : this->m_place_holder_texture->get_sampled_image();
								}
								device->write_descriptor_set(scene_mesh_skinned_subset.m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set, 4U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sample_images) / sizeof(sample_images[0]), NULL, NULL, NULL, NULL, sample_images, NULL, NULL, NULL);
							}
						}

						{
							scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_descriptor_set = device->create_descriptor_set(skin_pipeline_per_mesh_instance_update_descriptor_set_layout);

							constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(skin_pipeline_per_mesh_instance_update_set_uniform_buffer_binding);
							device->write_descriptor_set(scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
						}

						{
							scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set = device->create_descriptor_set(gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout);

							constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding);
							device->write_descriptor_set(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
						}
					}
				}
			}
		}

		// Deferred Shading Pipeline
		{
			{
				this->m_deferred_shading_pipeline_none_update_descriptor_set = device->create_descriptor_set(this->m_deferred_shading_pipeline_none_update_descriptor_set_layout);

				constexpr uint32_t const dynamic_uniform_buffers_range = sizeof(common_none_update_set_uniform_buffer_binding);
				device->write_descriptor_set(this->m_deferred_shading_pipeline_none_update_descriptor_set, 0U, BRX_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER, 0U, 1U, &this->m_common_none_update_uniform_buffer, &dynamic_uniform_buffers_range, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}
	}

	device->destroy_descriptor_set_layout(skin_pipeline_per_mesh_instance_update_descriptor_set_layout);
	skin_pipeline_per_mesh_instance_update_descriptor_set_layout = NULL;
	device->destroy_descriptor_set_layout(skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout);
	skin_pipeline_per_mesh_skinned_subset_update_descriptor_set_layout = NULL;
	device->destroy_descriptor_set_layout(gbuffer_pipeline_none_update_descriptor_set_layout);
	gbuffer_pipeline_none_update_descriptor_set_layout = NULL;
	device->destroy_descriptor_set_layout(gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout);
	gbuffer_pipeline_per_mesh_subset_update_descriptor_set_layout = NULL;
	device->destroy_descriptor_set_layout(gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout);
	gbuffer_pipeline_per_mesh_instance_update_descriptor_set_layout = NULL;

	// Init Intermediate Images
	this->m_intermediate_width = 0U;
	this->m_intermediate_height = 0U;
	this->m_gbuffer_color_attachment = NULL;
	this->m_gbuffer_depth_attachment = NULL;
	this->m_deferred_shading_color_attachment = NULL;
	this->m_gbuffer_frame_buffer = NULL;
	this->m_deferred_shading_frame_buffer = NULL;

	// Init Camera
	g_camera_controller.m_eye_position = DirectX::XMFLOAT3(0.0F, 1.3F, 1.0F);
	g_camera_controller.m_eye_direction = DirectX::XMFLOAT3(0.0F, -0.36F, -0.98F);
	g_camera_controller.m_up_direction = DirectX::XMFLOAT3(0.0F, 1.0F, 0.0F);
	g_camera_fov = DirectX::XM_PI * (1.0F / 3.0F);
}

void Demo::destroy(brx_device *device)
{
	// Descriptor
	{
		// GBuffer Pipeline
		{
			device->destroy_descriptor_set(this->m_gbuffer_pipeline_none_update_descriptor_set);

			for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
			{
				Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

				if (!scene_mesh.m_skinned)
				{
					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Subset &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

						device->destroy_descriptor_set(scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set);
					}

					for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
					{
						Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

						assert(scene_mesh_instance.m_subsets.empty());

						assert(NULL == scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_descriptor_set);

						device->destroy_descriptor_set(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set);
					}
				}
				else
				{
					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Subset &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

						assert(NULL == scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set);
					}

					for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
					{
						Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

						for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
						{
							Demo_Mesh_Skinned_Subset &scene_mesh_skinned_subset = scene_mesh_instance.m_subsets[subset_index];

							device->destroy_descriptor_set(scene_mesh_skinned_subset.m_skin_pipeline_per_mesh_skinned_subset_update_descriptor_set);

							device->destroy_descriptor_set(scene_mesh_skinned_subset.m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set);
						}

						device->destroy_descriptor_set(scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_descriptor_set);

						device->destroy_descriptor_set(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set);
					}
				}
			}
		}

		// Deferred Shading Pipeline
		{
			device->destroy_descriptor_set_layout(this->m_deferred_shading_pipeline_none_update_descriptor_set_layout);

			device->destroy_descriptor_set(this->m_deferred_shading_pipeline_none_update_descriptor_set);
		}
	}

	// Uniform Buffer
	{
		device->destroy_uniform_upload_buffer(this->m_common_none_update_uniform_buffer);

		for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
		{
			Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

			for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
			{
				Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

				if (!scene_mesh.m_skinned)
				{
					assert(NULL == scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_uniform_buffer);
				}
				else
				{
					device->destroy_uniform_upload_buffer(scene_mesh_instance.m_skin_pipeline_per_mesh_instance_update_uniform_buffer);
				}

				device->destroy_uniform_upload_buffer(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer);
			}
		}
	}

	// Sampler
	{
		device->destroy_sampler(this->m_sampler);
	}

	// Place Holder Texture
	{
		device->destroy_sampled_asset_image(this->m_place_holder_texture);
	}

	// Asset
	{
		for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
		{
			Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

			for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
			{
				Demo_Mesh_Subset &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

				device->destroy_storage_asset_buffer(scene_mesh_subset.m_vertex_position_buffer);

				device->destroy_storage_asset_buffer(scene_mesh_subset.m_vertex_varying_buffer);

				if (scene_mesh.m_skinned)
				{
					device->destroy_storage_asset_buffer(scene_mesh_subset.m_vertex_skinned_binding_buffer);
				}
				else
				{
					assert(NULL == scene_mesh_subset.m_vertex_skinned_binding_buffer);
				}

				device->destroy_storage_asset_buffer(scene_mesh_subset.m_index_buffer);

				device->destroy_storage_asset_buffer(scene_mesh_subset.m_material_buffer);

				for (uint32_t mesh_subset_meterial_texture_index = 0U; mesh_subset_meterial_texture_index < DEMO_MESH_SUBSET_METERIAL_TEXTURE_COUNT; ++mesh_subset_meterial_texture_index)
				{
					brx_sampled_asset_image *const material_texture = scene_mesh_subset.m_material_textures[mesh_subset_meterial_texture_index];

					if (NULL != material_texture)
					{
						device->destroy_sampled_asset_image(material_texture);
					}
				}
			}

			if (!scene_mesh.m_skinned)
			{
				for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
				{
					Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

					assert(scene_mesh_instance.m_subsets.empty());
				}
			}
			else
			{
				for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
				{
					Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

					for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
					{
						Demo_Mesh_Skinned_Subset &scene_mesh_skinned_subset = scene_mesh_instance.m_subsets[subset_index];

						device->destroy_storage_intermediate_buffer(scene_mesh_skinned_subset.m_skinned_vertex_position_buffer);

						device->destroy_storage_intermediate_buffer(scene_mesh_skinned_subset.m_skinned_vertex_varying_buffer);
					}
				}
			}
		}
	}

	// Render Pass and Pipeline
	{
		device->destroy_graphics_pipeline(this->m_gbuffer_pipeline);

		device->destroy_pipeline_layout(this->m_gbuffer_pipeline_layout);

		device->destroy_render_pass(this->m_gbuffer_render_pass);

		device->destroy_graphics_pipeline(this->m_deferred_shading_pipeline);

		device->destroy_pipeline_layout(this->m_deferred_shading_pipeline_layout);

		device->destroy_render_pass(this->m_deferred_shading_render_pass);

		device->destroy_compute_pipeline(this->m_skin_pipeline);

		device->destroy_pipeline_layout(this->m_skin_pipeline_layout);
	}
}

void Demo::on_swap_chain_attach(brx_device *device, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height)
{
	assert(0U == this->m_intermediate_width);
	assert(0U == this->m_intermediate_height);

	this->m_intermediate_width = swap_chain_image_width;
	this->m_intermediate_height = swap_chain_image_height;

	// Intermediate Images
	{
		assert(NULL == this->m_gbuffer_color_attachment);
		assert(NULL == this->m_gbuffer_depth_attachment);
		assert(NULL == this->m_deferred_shading_color_attachment);

		this->m_gbuffer_color_attachment = device->create_color_attachment_image(this->m_gbuffer_color_attachement_format, this->m_intermediate_width, this->m_intermediate_height, true);
		this->m_gbuffer_depth_attachment = device->create_depth_stencil_attachment_image(this->m_gbuffer_depth_attachement_format, this->m_intermediate_width, this->m_intermediate_height, true);
		this->m_deferred_shading_color_attachment = device->create_color_attachment_image(this->m_deferred_shading_color_attachement_format, this->m_intermediate_width, this->m_intermediate_height, true);
	}

	// Frame Buffer
	{
		assert(NULL == this->m_gbuffer_frame_buffer);
		assert(NULL == this->m_deferred_shading_frame_buffer);

		this->m_gbuffer_frame_buffer = device->create_frame_buffer(this->m_gbuffer_render_pass, this->m_intermediate_width, this->m_intermediate_height, 1U, &this->m_gbuffer_color_attachment, this->m_gbuffer_depth_attachment);
		this->m_deferred_shading_frame_buffer = device->create_frame_buffer(this->m_deferred_shading_render_pass, this->m_intermediate_width, this->m_intermediate_height, 1U, &this->m_deferred_shading_color_attachment, NULL);
	}

	// Descriptor
	{
		// Deferred Shading Pipeline
		{
			// The VkDescriptorSetLayout should still be valid when perform write update on VkDescriptorSet
			assert(NULL != this->m_deferred_shading_pipeline_none_update_descriptor_set_layout);
			{
				{
					brx_sampled_image const *const sampled_images[] = {
						this->m_gbuffer_color_attachment->get_sampled_image()};
					device->write_descriptor_set(this->m_deferred_shading_pipeline_none_update_descriptor_set, 1U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
				}

				{
					brx_sampled_image const *const sampled_images[] = {
						this->m_gbuffer_depth_attachment->get_sampled_image()};
					device->write_descriptor_set(this->m_deferred_shading_pipeline_none_update_descriptor_set, 2U, BRX_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0U, sizeof(sampled_images) / sizeof(sampled_images[0]), NULL, NULL, NULL, NULL, sampled_images, NULL, NULL, NULL);
				}
			}
		}
	}
}

brx_sampled_image const *Demo::get_sampled_image_for_present()
{
	return this->m_deferred_shading_color_attachment->get_sampled_image();
}

void Demo::on_swap_chain_dettach(brx_device *device)
{
	device->destroy_frame_buffer(this->m_gbuffer_frame_buffer);
	device->destroy_frame_buffer(this->m_deferred_shading_frame_buffer);

	this->m_gbuffer_frame_buffer = NULL;
	this->m_deferred_shading_frame_buffer = NULL;

	device->destroy_color_attachment_image(this->m_deferred_shading_color_attachment);
	device->destroy_depth_stencil_attachment_image(this->m_gbuffer_depth_attachment);
	device->destroy_color_attachment_image(this->m_gbuffer_color_attachment);

	this->m_deferred_shading_color_attachment = NULL;
	this->m_gbuffer_depth_attachment = NULL;
	this->m_gbuffer_color_attachment = NULL;

	this->m_intermediate_width = 0U;
	this->m_intermediate_height = 0U;
}

void Demo::draw(brx_graphics_command_buffer *command_buffer, float interval_time, uint32_t frame_throttling_index)
{
	// Update Uniform Buffer
	{
		// Common - None Update (update frequency denotes the updating of the resource bindings instead of the data)
		{
			common_none_update_set_uniform_buffer_binding *const common_none_update_set_uniform_buffer_binding_destination = reinterpret_cast<common_none_update_set_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(this->m_common_none_update_uniform_buffer->get_host_memory_range_base()) + tbb_align_up(static_cast<uint32_t>(sizeof(common_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index);

			DirectX::XMFLOAT3 eye_position = g_camera_controller.m_eye_position;
			DirectX::XMFLOAT3 eye_direction = g_camera_controller.m_eye_direction;
			DirectX::XMFLOAT3 up_direction = g_camera_controller.m_up_direction;
			DirectX::XMMATRIX view_transform = DirectX::XMMatrixLookToRH(DirectX::XMLoadFloat3(&eye_position), DirectX::XMLoadFloat3(&eye_direction), DirectX::XMLoadFloat3(&up_direction));
			DirectX::XMMATRIX projection_transform = DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(g_camera_fov, static_cast<float>(this->m_intermediate_width) / static_cast<float>(this->m_intermediate_height), 0.1F, 1000.0F);

			DirectX::XMStoreFloat4x4(&common_none_update_set_uniform_buffer_binding_destination->g_view_transform, view_transform);
			DirectX::XMStoreFloat4x4(&common_none_update_set_uniform_buffer_binding_destination->g_projection_transform, projection_transform);
			DirectX::XMStoreFloat4x4(&common_none_update_set_uniform_buffer_binding_destination->g_inverse_view_transform, DirectX::XMMatrixInverse(NULL, view_transform));
			DirectX::XMStoreFloat4x4(&common_none_update_set_uniform_buffer_binding_destination->g_inverse_projection_transform, DirectX::XMMatrixInverse(NULL, projection_transform));

			common_none_update_set_uniform_buffer_binding_destination->g_screen_width = static_cast<float>(this->m_intermediate_width);
			common_none_update_set_uniform_buffer_binding_destination->g_screen_height = static_cast<float>(this->m_intermediate_height);
		}

		// GBuffer Pipeline - Per Mesh Instance Update
		{
			for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
			{
				Demo_Mesh &scene_mesh = this->m_scene_meshes[mesh_index];

				for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
				{
					Demo_Mesh_Instance &scene_mesh_instance = scene_mesh.m_instances[instance_index];

					gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding *const gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding_destination = reinterpret_cast<gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding *>(reinterpret_cast<uintptr_t>(scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer->get_host_memory_range_base()) + tbb_align_up(static_cast<uint32_t>(sizeof(gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index);

					gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding_destination->g_model_transform = scene_mesh_instance.m_model_transform;
				}
			}
		}
	}

	// GBuffer Pass
	{
		command_buffer->begin_debug_utils_label("GBuffer Pass");

		float color_clear_values[4] = {0.0F, 0.0F, 0.0F, 0.0F};
		float depth_clear_value = 0.0F;
		command_buffer->begin_render_pass(this->m_gbuffer_render_pass, this->m_gbuffer_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, 1U, &color_clear_values, &depth_clear_value, NULL);

		command_buffer->bind_graphics_pipeline(this->m_gbuffer_pipeline);

		command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

		command_buffer->set_scissor(this->m_intermediate_width, this->m_intermediate_height);

		for (uint32_t mesh_index = 0; mesh_index < this->m_scene_meshes.size(); ++mesh_index)
		{
			Demo_Mesh const &scene_mesh = this->m_scene_meshes[mesh_index];

			for (size_t subset_index = 0U; subset_index < scene_mesh.m_subsets.size(); ++subset_index)
			{
				Demo_Mesh_Subset const &scene_mesh_subset = scene_mesh.m_subsets[subset_index];

				for (size_t instance_index = 0U; instance_index < scene_mesh.m_instances.size(); ++instance_index)
				{
					Demo_Mesh_Instance const &scene_mesh_instance = scene_mesh.m_instances[instance_index];

					brx_descriptor_set *const descritor_sets[] = {
						this->m_gbuffer_pipeline_none_update_descriptor_set,
						(!scene_mesh.m_skinned) ? scene_mesh_subset.m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set : scene_mesh_instance.m_subsets[subset_index].m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set,
						scene_mesh_instance.m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set};

					uint32_t const dynamic_offsets[] = {
						tbb_align_up(static_cast<uint32_t>(sizeof(common_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index,
						tbb_align_up(static_cast<uint32_t>(sizeof(gbuffer_pipeline_per_mesh_instance_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index};

					command_buffer->bind_graphics_descriptor_sets(this->m_gbuffer_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

					command_buffer->draw(scene_mesh_subset.m_index_count, 1U);
				}
			}
		}

		command_buffer->end_render_pass();

		command_buffer->end_debug_utils_label();
	}

	// Deferred Shading Pass
	{
		command_buffer->begin_debug_utils_label("Deferred Shading Pass");

		float color_clear_values[4] = {0.0F, 0.0F, 0.0F, 0.0F};
		command_buffer->begin_render_pass(this->m_deferred_shading_render_pass, this->m_deferred_shading_frame_buffer, this->m_intermediate_width, this->m_intermediate_height, 1U, &color_clear_values, NULL, NULL);

		command_buffer->bind_graphics_pipeline(this->m_deferred_shading_pipeline);

		command_buffer->set_view_port(this->m_intermediate_width, this->m_intermediate_height);

		command_buffer->set_scissor(this->m_intermediate_width, this->m_intermediate_height);

		brx_descriptor_set *const descritor_sets[] = {
			this->m_deferred_shading_pipeline_none_update_descriptor_set};

		uint32_t const dynamic_offsets[] = {
			tbb_align_up(static_cast<uint32_t>(sizeof(common_none_update_set_uniform_buffer_binding)), this->m_uniform_upload_buffer_offset_alignment) * frame_throttling_index};

		command_buffer->bind_graphics_descriptor_sets(this->m_deferred_shading_pipeline_layout, sizeof(descritor_sets) / sizeof(descritor_sets[0]), descritor_sets, sizeof(dynamic_offsets) / sizeof(dynamic_offsets[0]), dynamic_offsets);

		command_buffer->draw(3U, 1U);

		command_buffer->end_render_pass();

		command_buffer->end_debug_utils_label();
	}
}

static inline uint32_t tbb_align_up(uint32_t value, uint32_t alignment)
{
	//
	//  Copyright (c) 2005-2019 Intel Corporation
	//
	//  Licensed under the Apache License, Version 2.0 (the "License");
	//  you may not use this file except in compliance with the License.
	//  You may obtain a copy of the License at
	//
	//      http://www.apache.org/licenses/LICENSE-2.0
	//
	//  Unless required by applicable law or agreed to in writing, software
	//  distributed under the License is distributed on an "AS IS" BASIS,
	//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	//  See the License for the specific language governing permissions and
	//  limitations under the License.
	//

	// [alignUp](https://github.com/oneapi-src/oneTBB/blob/tbb_2019/src/tbbmalloc/shared_utils.h#L42)

	assert(alignment != static_cast<uint32_t>(0));

	// power-of-2 alignment
	assert((alignment & (alignment - static_cast<uint32_t>(1))) == static_cast<uint32_t>(0));

	return (((value - static_cast<uint32_t>(1)) | (alignment - static_cast<uint32_t>(1))) + static_cast<uint32_t>(1));
}

static inline DirectX::XMMATRIX XM_CALLCONV DirectX_Math_Matrix_PerspectiveFovRH_ReversedZ(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
{
	// [Reversed-Z](https://developer.nvidia.com/content/depth-precision-visualized)
	//
	// _  0  0  0
	// 0  _  0  0
	// 0  0  b -1
	// 0  0  a  0
	//
	// _  0  0  0
	// 0  _  0  0
	// 0  0 zb  -z
	// 0  0  a
	//
	// z' = -b - a/z
	//
	// Standard
	// 0 = -b + a/nearz // z=-nearz
	// 1 = -b + a/farz  // z=-farz
	// a = farz*nearz/(nearz - farz)
	// b = farz/(nearz - farz)
	//
	// Reversed-Z
	// 1 = -b + a/nearz // z=-nearz
	// 0 = -b + a/farz  // z=-farz
	// a = farz*nearz/(farz - nearz)
	// b = nearz/(farz - nearz)

	// __m128 _mm_shuffle_ps(__m128 lo,__m128 hi, _MM_SHUFFLE(hi3,hi2,lo1,lo0))
	// Interleave inputs into low 2 floats and high 2 floats of output. Basically
	// out[0]=lo[lo0];
	// out[1]=lo[lo1];
	// out[2]=hi[hi2];
	// out[3]=hi[hi3];

	// DirectX::XMMatrixPerspectiveFovRH

	float SinFov;
	float CosFov;
	DirectX::XMScalarSinCos(&SinFov, &CosFov, 0.5F * FovAngleY);

	float Height = CosFov / SinFov;
	float Width = Height / AspectRatio;
	float b = NearZ / (FarZ - NearZ);
	float a = (FarZ / (FarZ - NearZ)) * NearZ;
#if defined(_XM_SSE_INTRINSICS_)
	// Note: This is recorded on the stack
	DirectX::XMVECTOR rMem = {
		Width,
		Height,
		b,
		a};

	// Copy from memory to SSE register
	DirectX::XMVECTOR vValues = rMem;
	DirectX::XMVECTOR vTemp = _mm_setzero_ps();
	// Copy x only
	vTemp = _mm_move_ss(vTemp, vValues);
	// CosFov / SinFov,0,0,0
	DirectX::XMMATRIX M;
	M.r[0] = vTemp;
	// 0,Height / AspectRatio,0,0
	vTemp = vValues;
	vTemp = _mm_and_ps(vTemp, DirectX::g_XMMaskY);
	M.r[1] = vTemp;
	// x=b,y=a,0,-1.0f
	vTemp = _mm_setzero_ps();
	vValues = _mm_shuffle_ps(vValues, DirectX::g_XMNegIdentityR3, _MM_SHUFFLE(3, 2, 3, 2));
	// 0,0,b,-1.0f
	vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(3, 0, 0, 0));
	M.r[2] = vTemp;
	// 0,0,a,0.0f
	vTemp = _mm_shuffle_ps(vTemp, vValues, _MM_SHUFFLE(2, 1, 0, 0));
	M.r[3] = vTemp;
	return M;
#elif defined(_XM_ARM_NEON_INTRINSICS_)
	const float32x4_t Zero = vdupq_n_f32(0);

	DirectX::XMMATRIX M;
	M.r[0] = vsetq_lane_f32(Width, Zero, 0);
	M.r[1] = vsetq_lane_f32(Height, Zero, 1);
	M.r[2] = vsetq_lane_f32(b, DirectX::g_XMNegIdentityR3.v, 2);
	M.r[3] = vsetq_lane_f32(a, Zero, 2);
	return M;
#endif
}
