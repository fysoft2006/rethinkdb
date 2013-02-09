#ifndef RDB_PROTOCOL_FUNC_HPP_
#define RDB_PROTOCOL_FUNC_HPP_

#include <vector>

#include "utils.hpp"

#include "containers/ptr_bag.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/term.hpp"
#include "rpc/serialize_macros.hpp"
#include "protob/protob.hpp"

namespace ql {

class func_t : public ptr_baggable_t {
public:
    func_t(env_t *env, const Term2 *_source, backtrace_t::frame_t _frame);
    static func_t *new_shortcut_func(env_t *env, const datum_t *obj,
                                     backtrace_t::frame_t frame);
    val_t *_call(const std::vector<const datum_t *> &args);
    val_t *call(const datum_t *arg);
    val_t *call(const datum_t *arg1, const datum_t *arg2);

    void dump_scope(std::map<int, Datum> *out) const;
    bool is_deterministic() const;

    void maybe_set_frame(backtrace_t::frame_t _frame);
private:
    std::vector<const datum_t *> argptrs;
    term_t *body;

    friend class wire_func_t;
    const Term2 *source;
    backtrace_t::frame_t frame;
    bool implicit_bound;

    // TODO: make this smarter
    std::map<int, const datum_t **> scope;
};


RDB_MAKE_PROTOB_SERIALIZABLE(Term2);
RDB_MAKE_PROTOB_SERIALIZABLE(Datum);
class wire_func_t {
public:
    wire_func_t();
    virtual ~wire_func_t() { }
    wire_func_t(env_t *env, func_t *_func);
    wire_func_t(const Term2 &_source, std::map<int, Datum> *_scope,
                backtrace_t::frame_t _frame);
    func_t *compile(env_t *env);
protected:
    std::map<env_t *, func_t *> cached_funcs;

    Term2 source;
    backtrace_t::frame_t frame;
    std::map<int, Datum> scope;
public:
    //RDB_MAKE_ME_SERIALIZABLE_2(source, scope);
};

namespace derived {
// For whatever reason RDB_MAKE_ME_SERIALIZABLE_3 wasn't happy inside of the
// template, so, you know, yeah.
class serializable_wire_func_t : public wire_func_t {
public:
    serializable_wire_func_t() : wire_func_t() { }
    serializable_wire_func_t(env_t *env, func_t *func) : wire_func_t(env, func) { }
    serializable_wire_func_t(const Term2 &_source, std::map<int, Datum> *_scope,
                             backtrace_t::frame_t _frame)
        : wire_func_t(_source, _scope, _frame) { }
    RDB_MAKE_ME_SERIALIZABLE_3(source, frame, scope);
};
enum simple_funcs { MAP, FILTER, REDUCE, CONCATMAP };
template<int fconst>
class derived_wire_func_t : public serializable_wire_func_t {
public:
    derived_wire_func_t() : serializable_wire_func_t() { }
    derived_wire_func_t(env_t *env, func_t *func)
        : serializable_wire_func_t(env, func) { }
    derived_wire_func_t(const Term2 &_source, std::map<int, Datum> *_scope,
                        backtrace_t::frame_t _frame)
        : serializable_wire_func_t(_source, _scope, _frame) { }
};
} // namespace derived
typedef derived::derived_wire_func_t<derived::MAP> map_wire_func_t;
typedef derived::derived_wire_func_t<derived::FILTER> filter_wire_func_t;
typedef derived::derived_wire_func_t<derived::REDUCE> reduce_wire_func_t;
typedef derived::derived_wire_func_t<derived::CONCATMAP> concatmap_wire_func_t;

// Faux functions
struct count_wire_func_t { RDB_MAKE_ME_SERIALIZABLE_0() };

// Grouped Map Reduce
class gmr_wire_func_t {
public:
    gmr_wire_func_t() { }
    gmr_wire_func_t(env_t *env, func_t *_group, func_t *_map, func_t *_reduce)
        : group(env, _group), map(env, _map), reduce(env, _reduce) { }
    func_t *compile_group(env_t *env) { return group.compile(env); }
    func_t *compile_map(env_t *env) { return map.compile(env); }
    func_t *compile_reduce(env_t *env) { return reduce.compile(env); }
private:
    map_wire_func_t group;
    map_wire_func_t map;
    reduce_wire_func_t reduce;
public:
    RDB_MAKE_ME_SERIALIZABLE_3(group, map, reduce);
};

class func_term_t : public term_t {
public:
    func_term_t(env_t *env, const Term2 *term);
private:
    virtual bool is_deterministic_impl() const;
    virtual val_t *eval_impl();
    virtual const char *name() const { return "func"; }
    func_t *func;
};

} // namespace ql
#endif // RDB_PROTOCOL_FUNC_HPP_
