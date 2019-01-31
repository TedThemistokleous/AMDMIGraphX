#ifndef MIGRAPHX_GUARD_FIND_CONCUR_HPP
#define MIGRAPHX_GUARD_FIND_CONCUR_HPP

#include <cassert>
#include <string>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <unordered_map>
#include <vector>

namespace migraphx {

#ifdef DOXYGEN

/// An interface for target-dependent analysis to find concurrent instructions
/// executing in different streams.
struct find_concur
{
    void get_concur(program* p,
                    int num_of_streams,
                    std::unordered_map<const instruction*,
                                       std::vector<std::vector<const instruction*>>>& concur_instrs,
                    std::unordered_map<const instruction*, int>& instr2_points);
};

#else

<%
interface('find_concur',
          virtual('get_concur', returns='void', p = 'program*', num_of_stream = 'int', concur_instrs = 'std::unordered_map<const instruction*, std::vector<std::vector<const instruction*>>>&', input = 'std::unordered_map<const instruction*, int>&')
)
%>

#endif
} // namespace migraphx

#endif
