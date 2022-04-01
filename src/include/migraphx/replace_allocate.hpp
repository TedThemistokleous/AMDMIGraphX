#ifndef MIGRAPHX_GUARD_RTGLIB_REPLACE_ALLOCATE_HPP
#define MIGRAPHX_GUARD_RTGLIB_REPLACE_ALLOCATE_HPP

#include <migraphx/config.hpp>
#include <migraphx/allocation_model.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

struct module;

struct replace_allocate
{
    allocation_model model;
    std::string name() const { return "replace_allocate"; }
    void apply(module& p) const;
};

} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
