
#include "verify_program.hpp"
#include <migraphx/program.hpp>
#include <migraphx/generate.hpp>
#include <migraphx/make_op.hpp>

struct test_step_broadcast_transpose : verify_program<test_step_broadcast_transpose>
{
    migraphx::program create_program() const
    {
        migraphx::program p;
        auto* mm = p.get_main_module();
        migraphx::shape s1{migraphx::shape::float_type, {1, 1, 1, 6}};
        auto l0 = mm->add_parameter("x", s1);
        auto ml = mm->add_instruction(
            migraphx::make_op("multibroadcast", {{"output_lens", {2, 1, 4, 6}}}), l0);
        auto tl = mm->add_instruction(migraphx::make_op("transpose", {{"dims", {0, 2, 3, 1}}}), ml);
        auto r  = mm->add_instruction(
            migraphx::make_op("step", {{"axes", {0, 1, 2}}, {"steps", {2, 2, 3}}}), tl);
        mm->add_return({r});

        return p;
    }
};
