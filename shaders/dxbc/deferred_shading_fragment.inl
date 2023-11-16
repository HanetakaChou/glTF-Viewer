#define BYTE uint8_t
#ifndef NDEBUG
#include "debug/_internal_deferred_shading_fragment.inl"
#else
#include "release/_internal_deferred_shading_fragment.inl"
#endif
#undef BYTE