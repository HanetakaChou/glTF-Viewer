constexpr uint32_t const gbuffer_vertex_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_gbuffer_vertex.inl"
#else
#include "release/_internal_gbuffer_vertex.inl"
#endif
};