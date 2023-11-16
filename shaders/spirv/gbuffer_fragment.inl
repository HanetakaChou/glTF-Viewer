constexpr uint32_t const gbuffer_fragment_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_gbuffer_fragment.inl"
#else
#include "release/_internal_gbuffer_fragment.inl"
#endif
};