
#include "verify_program.hpp"
#include <migraphx/program.hpp>
#include <migraphx/generate.hpp>
#include <migraphx/make_op.hpp>

struct test_atanh : verify_program<test_atanh>
{
    migraphx::program create_program() const
    {
        migraphx::program p;
        auto* mm = p.get_main_module();
        migraphx::shape s{migraphx::shape::float_type, {16}};
        auto x       = mm->add_parameter("x", s);
        auto min_val = mm->add_literal(-0.95f);
        auto max_val = mm->add_literal(0.95f);
        min_val =
            mm->add_instruction(migraphx::make_op("multibroadcast", {{"out_lens", {16}}}), min_val);
        max_val =
            mm->add_instruction(migraphx::make_op("multibroadcast", {{"out_lens", {16}}}), max_val);
        auto cx = mm->add_instruction(migraphx::make_op("clip"), x, min_val, max_val);
        mm->add_instruction(migraphx::make_op("atanh"), cx);
        return p;
    }
};
