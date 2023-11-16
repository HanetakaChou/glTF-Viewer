constexpr uint32_t const deferred_shading_fragment_shader_module_code[] = {
#ifndef NDEBUG
#include "debug/_internal_deferred_shading_fragment.inl"
#else
#include "release/_internal_deferred_shading_fragment.inl"
#endif
};