#ifndef PUD_MODULE_SERDE_FIXTURE_HPP
#define PUD_MODULE_SERDE_FIXTURE_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "functor_fixture.hpp"
#include "infrastructure/expr_pool.hpp"
#include "infrastructure/pud_module.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_inference.hpp"
#include "value_objects/pud_substitution.hpp"
#include "value_objects/rule.hpp"

#include "infrastructure/expr_ostream_inserter.hpp"
#include "infrastructure/expr_istream_extractor.hpp"
#include "infrastructure/istream_fixed_bytes_extractor.hpp"
#include "infrastructure/istream_uint8_extractor.hpp"
#include "infrastructure/istream_uint32_extractor.hpp"
#include "infrastructure/istream_uint64_extractor.hpp"
#include "infrastructure/ostream_fixed_bytes_inserter.hpp"
#include "infrastructure/ostream_uint8_inserter.hpp"
#include "infrastructure/ostream_uint32_inserter.hpp"
#include "infrastructure/ostream_uint64_inserter.hpp"
#include "infrastructure/pud_inference_istream_extractor.hpp"
#include "infrastructure/pud_inference_ostream_inserter.hpp"
#include "infrastructure/pud_module_istream_extractor.hpp"
#include "infrastructure/pud_module_ostream_inserter.hpp"
#include "infrastructure/rule_istream_extractor.hpp"
#include "infrastructure/rule_ostream_inserter.hpp"
#include "infrastructure/vector_uint64_istream_deserializer.hpp"
#include "infrastructure/vector_uint64_ostream_serializer.hpp"

struct pud_module_serde_bytes {
    static void append_u8(std::string& blob, uint8_t value) {
        blob.push_back(static_cast<char>(value));
    }

    static void append_u32(std::string& blob, uint32_t value) {
        append_u8(blob, static_cast<uint8_t>(value & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 8) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 16) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 24) & 0xffu));
    }

    static void append_u64(std::string& blob, uint64_t value) {
        append_u8(blob, static_cast<uint8_t>(value & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 8) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 16) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 24) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 32) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 40) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 48) & 0xffu));
        append_u8(blob, static_cast<uint8_t>((value >> 56) & 0xffu));
    }

    static void append_magic(std::string& blob) {
        blob.append(k_pud_module_blob_magic, 4);
    }

    static void append_var_expr(std::string& blob, uint32_t index) {
        append_u8(blob, 0);
        append_u32(blob, index);
    }

    static void append_nullary_functor(std::string& blob, uint32_t id) {
        append_u8(blob, 1);
        append_u32(blob, id);
        append_u32(blob, 0);
    }

    static uint8_t peek_u8(const std::string& blob, size_t offset) {
        if (offset >= blob.size())
            throw std::runtime_error("peek past end of blob");
        return static_cast<uint8_t>(blob[offset]);
    }

    static uint32_t peek_u32(const std::string& blob, size_t offset) {
        return static_cast<uint32_t>(peek_u8(blob, offset))
            | (static_cast<uint32_t>(peek_u8(blob, offset + 1)) << 8)
            | (static_cast<uint32_t>(peek_u8(blob, offset + 2)) << 16)
            | (static_cast<uint32_t>(peek_u8(blob, offset + 3)) << 24);
    }

    static uint64_t peek_u64(const std::string& blob, size_t offset) {
        return static_cast<uint64_t>(peek_u8(blob, offset))
            | (static_cast<uint64_t>(peek_u8(blob, offset + 1)) << 8)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 2)) << 16)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 3)) << 24)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 4)) << 32)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 5)) << 40)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 6)) << 48)
            | (static_cast<uint64_t>(peek_u8(blob, offset + 7)) << 56);
    }

    static std::string truncate(const std::string& blob, size_t keep) {
        return blob.substr(0, keep);
    }

    static std::string flip_byte(const std::string& blob, size_t offset) {
        std::string copy = blob;
        copy.at(offset) = static_cast<char>(static_cast<uint8_t>(copy.at(offset)) ^ 0xffu);
        return copy;
    }

    static std::string replace_magic(const std::string& blob, const char magic[4]) {
        std::string copy = blob;
        copy.replace(0, 4, magic, 4);
        return copy;
    }
};

struct pud_module_serde_cursor {
    const std::string& blob;
    size_t offset;

    pud_module_serde_cursor(const std::string& blob)
        : blob(blob)
        , offset(0) {}

    uint8_t read_u8() {
        const uint8_t value = pud_module_serde_bytes::peek_u8(blob, offset);
        ++offset;
        return value;
    }

    uint32_t read_u32() {
        const uint32_t value = pud_module_serde_bytes::peek_u32(blob, offset);
        offset += 4;
        return value;
    }

    uint64_t read_u64() {
        const uint64_t value = pud_module_serde_bytes::peek_u64(blob, offset);
        offset += 8;
        return value;
    }

    void expect_magic() {
        if (read_u8() != static_cast<uint8_t>(k_pud_module_blob_magic[0])
            || read_u8() != static_cast<uint8_t>(k_pud_module_blob_magic[1])
            || read_u8() != static_cast<uint8_t>(k_pud_module_blob_magic[2])
            || read_u8() != static_cast<uint8_t>(k_pud_module_blob_magic[3]))
            throw std::runtime_error("blob magic is not PUDM");
    }
};

inline pud_module make_empty_pud_module(pud_module_id id) {
    return pud_module{id, {}, {}};
}

inline pud_module make_facts_pud_module(pud_module_id id, expr_pool& pool, test_functors& functors) {
    return pud_module{
        id,
        {},
        {
            rule(pool.make_functor(functors.id("p"), {pool.make_functor(functors.id("a"), {})}), {}, 0),
            rule(pool.make_functor(functors.id("p"), {pool.make_functor(functors.id("b"), {})}), {}, 0),
            rule(pool.make_functor(functors.id("q"), {pool.make_functor(functors.id("a"), {})}), {}, 0),
        },
    };
}

inline pud_module make_chain_pud_module(pud_module_id id, expr_pool& pool, test_functors& functors, size_t depth) {
    const expr* goal = pool.make_functor(functors.id("g"), {});
    const expr* value = pool.make_functor(functors.id("v"), {});
    pud_module module{
        id,
        {},
        {rule(pool.make_functor(functors.id("h"), {}), {goal}, 0)},
    };
    pud_node_id parent = 0;
    for (size_t step = 0; step < depth; ++step) {
        parent = module.store_inference(pud_inference{
            .parent = parent,
            .call_site = 0,
            .substitutions = {pud_substitution{.var_idx = 0, .value = value}},
            .body_goals = {goal},
            .var_count = 1,
        });
    }
    return module;
}

inline pud_module make_branchy_pud_module(pud_module_id id, expr_pool& pool, test_functors& functors) {
    const expr* goal = pool.make_functor(functors.id("g"), {});
    const expr* value = pool.make_functor(functors.id("v"), {});
    pud_module module{
        id,
        {5, 9},
        {
            rule(pool.make_functor(functors.id("h"), {}), {goal}, 1),
            rule(pool.make_functor(functors.id("f"), {}), {}, 0),
        },
    };
    const pud_node_id first = module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {pud_substitution{.var_idx = 0, .value = value}},
        .body_goals = {goal},
        .var_count = 1,
    });
    module.store_inference(pud_inference{
        .parent = 0,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {goal},
        .var_count = 0,
    });
    module.store_inference(pud_inference{
        .parent = first,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 0,
    });
    module.store_inference(pud_inference{
        .parent = 1,
        .call_site = 0,
        .substitutions = {},
        .body_goals = {},
        .var_count = 2,
    });
    return module;
}

struct pud_module_serde_wiring {
    ostream_uint8_inserter insert_uint8_;
    ostream_uint32_inserter insert_uint32_;
    ostream_uint64_inserter insert_uint64_;
    ostream_fixed_bytes_inserter insert_fixed_bytes_;
    expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter> insert_expr_;
    rule_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_rule_;
    pud_inference_ostream_inserter<
        expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
        ostream_uint64_inserter,
        ostream_uint32_inserter> insert_inference_;
    vector_uint64_ostream_serializer<ostream_uint64_inserter> serialize_vector_uint64_;
    pud_module_ostream_inserter<
        ostream_fixed_bytes_inserter,
        ostream_uint64_inserter,
        vector_uint64_ostream_serializer<ostream_uint64_inserter>,
        rule_ostream_inserter<
            expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
            ostream_uint64_inserter,
            ostream_uint32_inserter>,
        pud_inference_ostream_inserter<
            expr_ostream_inserter<ostream_uint8_inserter, ostream_uint32_inserter>,
            ostream_uint64_inserter,
            ostream_uint32_inserter>,
        ostream_uint8_inserter> insert_module_;

    pud_module_serde_wiring()
        : insert_expr_(insert_uint8_, insert_uint32_)
        , insert_rule_(insert_expr_, insert_uint64_, insert_uint32_)
        , insert_inference_(insert_expr_, insert_uint64_, insert_uint32_)
        , serialize_vector_uint64_(insert_uint64_)
        , insert_module_(
            insert_fixed_bytes_,
            insert_uint64_,
            serialize_vector_uint64_,
            insert_rule_,
            insert_inference_,
            insert_uint8_) {}
};

template<typename IMakeVar, typename IMakeFunctor>
struct pud_module_serde_decode_wiring {
    using expr_extractor_t =
        expr_istream_extractor<IMakeVar, IMakeFunctor, istream_uint8_extractor, istream_uint32_extractor>;
    using rule_extractor_t =
        rule_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;
    using inference_extractor_t =
        pud_inference_istream_extractor<expr_extractor_t, istream_uint64_extractor, istream_uint32_extractor>;

    istream_uint8_extractor extract_uint8_;
    istream_uint32_extractor extract_uint32_;
    istream_uint64_extractor extract_uint64_;
    istream_fixed_bytes_extractor extract_fixed_bytes_;
    expr_extractor_t extract_expr_;
    rule_extractor_t extract_rule_;
    inference_extractor_t extract_inference_;
    vector_uint64_istream_deserializer<istream_uint64_extractor> deserialize_vector_uint64_;
    pud_module_istream_extractor<
        istream_fixed_bytes_extractor,
        istream_uint64_extractor,
        vector_uint64_istream_deserializer<istream_uint64_extractor>,
        rule_extractor_t,
        inference_extractor_t,
        istream_uint8_extractor> extract_module_;

    pud_module_serde_decode_wiring(IMakeVar& make_var, IMakeFunctor& make_functor)
        : extract_expr_(make_var, make_functor, extract_uint8_, extract_uint32_)
        , extract_rule_(extract_expr_, extract_uint64_, extract_uint32_)
        , extract_inference_(extract_expr_, extract_uint64_, extract_uint32_)
        , deserialize_vector_uint64_(extract_uint64_)
        , extract_module_(
            extract_fixed_bytes_,
            extract_uint64_,
            deserialize_vector_uint64_,
            extract_rule_,
            extract_inference_,
            extract_uint8_) {}
};

#endif
