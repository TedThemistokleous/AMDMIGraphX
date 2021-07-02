#ifndef MIGRAPHX_GUARD_OPERATORS_GATHER_HPP
#define MIGRAPHX_GUARD_OPERATORS_GATHER_HPP

#include <algorithm>
#include <migraphx/check_shapes.hpp>
#include <migraphx/config.hpp>
#include <migraphx/value.hpp>
#include <migraphx/op/normalize_attribute.hpp>
#include <migraphx/ranges.hpp>
#include <migraphx/par_for.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace op {

struct topk
{
    int64_t k;
    int64_t axis = 0;
    bool largest = true;
    bool sorted  = true;

    template <class Self, class F>
    static auto reflect(Self& self, F f)
    {
        return pack(f(self.k, "topk"),
                    f(self.axis, "axis"),
                    f(self.largest, "largest"),
                    f(self.sorted, "sorted"));
    }

    value attributes() const
    {
        value normalize;
        normalize["axis"] = value::array{normalize_attribute::include_min};
        return {{"normalize_axes", normalize}};
    }

    std::string name() const { return "topk"; }

    shape normalize_compute_shape(std::vector<shape> inputs) const
    {
        check_shapes{inputs, *this}.has(1).standard();
        auto lens = inputs.at(0).lens();
        auto type = inputs.at(0).type();

        lens[axis] = k;

        shape s_val{type, lens};
        shape s_ind{shape::int64_type, lens};

        return shape({s_val, s_ind});
    }

    struct compare_op
    {
        bool larger;
        template <class T, class U>
        auto operator()(T x, U y) const
        {
            return (larger ? (x < y) : (x > y));
        }
    };

    template <class T>
    void swap(T& v1, T& v2) const
    {
        T v = v1;
        v1  = v2;
        v2  = v;
    }

    template <class T, class Op>
    void heapify(const T& data,
                 const shape& iss,
                 std::vector<std::size_t> sidx,
                 std::vector<int>& indices,
                 int n,
                 int i,
                 Op op) const
    {
        int index = i;
        auto idx  = sidx;
        auto idxl = sidx;
        auto idxr = sidx;
        auto idxp = sidx;
        while(index < n)
        {
            auto pre_index = index;
            int l          = 2 * index + 1;
            int r          = 2 * index + 2;
            idx[axis]      = indices[index];
            idxl[axis]     = indices[l];
            idxr[axis]     = indices[r];
            if(l < n && op(data[iss.index(idxl)], data[iss.index(idx)]))
            {
                index = l;
            }

            if(r < n && op(data[iss.index(idxr)], data[iss.index(idx)]))
            {
                index = r;
                if(op(data[iss.index(idxl)], data[iss.index(idxr)]))
                {
                    index = l;
                }
            }

            if(index == pre_index)
            {
                break;
            }
            swap(indices[index], indices[pre_index]);
        }
    }

    template <class T, class Op>
    void build_heap(const T& data,
                    const shape& iss,
                    std::vector<std::size_t> sidx,
                    std::vector<int>& indices,
                    int n,
                    Op op) const
    {
        for(int i = n / 2 - 1; i >= 0; i--)
        {
            heapify(data, iss, sidx, indices, n, i, op);
        }
    }

    template <class T, class Op>
    void heap_add(const T& data,
                  const shape& iss,
                  std::vector<std::size_t> sidx,
                  std::vector<int>& indices,
                  int n,
                  const int& val,
                  Op op) const
    {
        auto idx  = sidx;
        idx[axis] = val;
        if(op(data[iss.index(idx)], data[iss.index(sidx)]))
        {
            return;
        }

        indices[0] = val;
        heapify(data, iss, sidx, indices, n, 0, op);
    }

    template <class T, class Op>
    void heap_sort(const T& data,
                   const shape& iss,
                   std::vector<std::size_t> sidx,
                   std::vector<int>& indices,
                   int n,
                   Op op) const
    {
        build_heap(data, iss, sidx, indices, n, op);

        for(int i = n - 1; i > 0; i--)
        {
            swap(indices[0], indices[i]);
            heapify(data, iss, sidx, indices, i, 0, op);
        }
    }

    template <class T, class Op>
    void topk_value(const T& data,
                    const shape& iss,
                    std::vector<std::size_t> sidx,
                    std::vector<int>& indices,
                    int n,
                    int kk,
                    Op op) const
    {
        build_heap(data, iss, sidx, indices, kk, op);
        for(int i = kk; i < n; ++i)
        {
            heap_add(data, iss, sidx, indices, kk, indices[i], op);
        }
    }

    argument compute(const shape& output_shape, std::vector<argument> args) const
    {
        auto vec_ss = output_shape.sub_shapes();
        argument res_val{vec_ss.front()};
        argument res_ind{vec_ss.back()};
        auto in_s     = args.front().get_shape();
        auto out_s    = vec_ss.front();
        auto in_lens  = in_s.lens();
        auto axis_dim = in_lens[axis];

        // compute shape
        auto comp_lens = in_lens;
        comp_lens.erase(comp_lens.begin() + axis);
        shape comp_s{in_s.type(), comp_lens};
        auto op = compare_op{largest};

        visit_all(res_val, args.front())([&](auto out_val, auto input) {
            res_ind.visit([&](auto out_ind) {
                par_for(comp_s.elements(), [&](auto i) {
                    auto idx = comp_s.multi(i);
                    std::vector<int> indices(k);
                    std::iota(indices.begin(), indices.end(), 0);
                    topk_value(input, in_s, idx, indices, axis_dim, k, op);

                    if(sorted)
                    {
                        heap_sort(input, in_s, idx, indices, k, op);
                    }

                    auto out_idx = idx;
                    auto in_idx  = idx;
                    for(auto j : range(indices.size()))
                    {
                        out_idx[axis]                 = j;
                        in_idx[axis]                  = indices[j];
                        out_val[out_s.index(out_idx)] = input[in_s.index(in_idx)];
                        out_ind[out_s.index(out_idx)] = indices[j];
                    }
                });
            });
        });

        return argument({res_val, res_ind});
    }
};

} // namespace op
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
