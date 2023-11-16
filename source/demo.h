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

#ifndef _DEMO_H_
#define _DEMO_H_ 1

#include "support/frame_throttling.h"
#include "../thirdparty/Brioche/include/brx_device.h"
#include "../thirdparty/Import-Asset/include/import_scene_asset.h"

struct Demo_Mesh_Skinned_Subset
{
    brx_storage_intermediate_buffer *m_skinned_vertex_position_buffer;
    brx_storage_intermediate_buffer *m_skinned_vertex_varying_buffer;
    brx_descriptor_set *m_skin_pipeline_per_mesh_skinned_subset_update_descriptor_set;
    brx_descriptor_set *m_gbuffer_pipeline_per_mesh_skinned_subset_update_descriptor_set;
};

struct Demo_Mesh_Instance
{
    DirectX::XMFLOAT4X4 m_model_transform;
    scene_animation_skeleton m_animation_skeleton;
    mcrt_vector<Demo_Mesh_Skinned_Subset> m_skinned_subsets;
    brx_uniform_upload_buffer *m_skin_pipeline_per_mesh_instance_update_uniform_buffer;
    brx_descriptor_set *m_skin_pipeline_per_mesh_instance_update_descriptor_set;
    brx_uniform_upload_buffer *m_gbuffer_pipeline_per_mesh_instance_update_uniform_buffer;
    brx_descriptor_set *m_gbuffer_pipeline_per_mesh_instance_update_descriptor_set;
};

struct Demo_Mesh_Subset
{
    uint32_t m_vertex_count;
    uint32_t m_index_count;
    brx_storage_asset_buffer *m_vertex_position_buffer;
    brx_storage_asset_buffer *m_vertex_varying_buffer;
    brx_storage_asset_buffer *m_vertex_joint_buffer;
    brx_storage_asset_buffer *m_index_buffer;
    brx_storage_asset_buffer *m_information_buffer;
    brx_sampled_asset_image *m_normal_texture;
    brx_sampled_asset_image *m_emissive_texture;
    brx_sampled_asset_image *m_base_color_texture;
    brx_sampled_asset_image *m_metallic_roughness_texture;
    brx_descriptor_set *m_gbuffer_pipeline_per_mesh_subset_update_descriptor_set;
};

struct Demo_Mesh
{
    bool m_skinned;
    mcrt_vector<Demo_Mesh_Subset> m_subsets;
    mcrt_vector<Demo_Mesh_Instance> m_instances;
};

class Demo
{
    brx_pipeline_layout *m_skin_pipeline_layout;
    brx_compute_pipeline *m_skin_pipeline;

    BRX_COLOR_ATTACHMENT_IMAGE_FORMAT m_gbuffer_color_attachement_format;
    BRX_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT m_gbuffer_depth_attachement_format;
    brx_render_pass *m_gbuffer_render_pass;

    brx_pipeline_layout *m_gbuffer_pipeline_layout;
    brx_graphics_pipeline *m_gbuffer_pipeline;

    // TODO: self dependency
    // TODO: VK_EXT_rasterization_order_attachment_access
    BRX_COLOR_ATTACHMENT_IMAGE_FORMAT m_deferred_shading_color_attachement_format;
    brx_render_pass *m_deferred_shading_render_pass;

    brx_pipeline_layout *m_deferred_shading_pipeline_layout;
    brx_graphics_pipeline *m_deferred_shading_pipeline;

    brx_sampled_asset_image *m_place_holder_texture;

    brx_sampler *m_sampler;

    mcrt_vector<Demo_Mesh> m_scene_meshes;
    mcrt_vector<brx_sampled_asset_image *> m_scene_textures;

    uint32_t m_uniform_upload_buffer_offset_alignment;
    brx_uniform_upload_buffer *m_common_gbuffer_pipeline_deferred_shading_pipeline_none_update_uniform_buffer;

    brx_descriptor_set *m_gbuffer_pipeline_none_update_descriptor_set;
    brx_descriptor_set_layout *m_deferred_shading_pipeline_none_update_descriptor_set_layout;
    brx_descriptor_set *m_deferred_shading_pipeline_none_update_descriptor_set;

    uint32_t m_intermediate_width;
    uint32_t m_intermediate_height;
    brx_color_attachment_image *m_gbuffer_color_attachment;
    brx_depth_stencil_attachment_image *m_gbuffer_depth_attachment;
    brx_frame_buffer *m_gbuffer_frame_buffer;
    brx_color_attachment_image *m_deferred_shading_color_attachment;
    brx_frame_buffer *m_deferred_shading_frame_buffer;

    float m_animation_time;

public:
    Demo();

    void init(brx_device *device, mcrt_vector<mcrt_string> const &file_names);

    void destroy(brx_device *device);

    void on_swap_chain_attach(brx_device *device, uint32_t swap_chain_image_width, uint32_t swap_chain_image_height);

    brx_sampled_image const *get_sampled_image_for_present();

    void on_swap_chain_dettach(brx_device *device);

    void draw(brx_graphics_command_buffer *command_buffer, float interval_time, uint32_t frame_throttling_index);
};

#endif