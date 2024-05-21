#ifndef BUILDER_H_
#define BUILDER_H_
#include "event.h"
#include "util.h"
#include <vector>
#include <random>
#include <cassert>
#include <optional>

#define evec std::vector<event_t>
using int_dist = std::uniform_int_distribution<std::mt19937_64::result_type>;
using generator = std::optional<std::mt19937_64>;

class Component {
public:
    virtual int size() const = 0;
    virtual evec build(timestamp_t offset, generator gen = {}) = 0;
    Component() {}
    virtual ~Component() {}
};

class Empty : public Component {
    public:
        Empty() : Component() {}
        ~Empty() {}
        virtual int size() const { return 0; }
        virtual evec build(timestamp_t offset, generator gen = {}) {
            return evec();
        }
};

class Concat : public Component {
    public:
        Component *u, *v;
        Concat(Component *c, Component *d) : Component(), u(c), v(d) {}
        virtual evec build(timestamp_t offset, generator gen = {}) {
            evec uvec = u->build(offset, gen);
            evec vvec = v->build(u->size() + offset, gen);
            uvec.insert(uvec.end(), vvec.begin(), vvec.end());
            return uvec;
        }
        virtual int size() const { return u->size() + v->size(); }
};

class Enclose : public Component {
    public:
        Component *u;
        Enclose(Component *c) : Component(), u(c) {}
        virtual evec build(timestamp_t offset, generator gen = {}) {
            evec res;
            res.push_back(event_t(EType::Epush, offset, val_t(offset, 0), 0));
            res.push_back(event_t(EType::Ereturn, offset, {}, 0));

            evec uvec = u->build(offset + 1, gen);

            res.insert(res.end(), uvec.begin(), uvec.end());

            res.push_back(event_t(EType::Epop, offset, val_t(offset, 0), 0));
            res.push_back(event_t(EType::Ereturn, offset, {}, 0));

            return res;
        }
        virtual int size() const { return u->size() + 1; }
};

// struct Rule {
//     virtual evec build(timestamp_t offset,  generator = {}) = 0;
//     Rule() {}
//     virtual ~Rule() {}
// };

// struct Epsilon : public Rule {
//     virtual evec build(timestamp_t offset, std::optional<std::mt19937_64> gen = {}) { return evec(); }
//     Epsilon() : Rule() {}
//     ~Epsilon() {}
// };

// struct PushPop : public Rule {
//     Rule *u, *v;
//     virtual evec build(timestamp_t offset, std::optional<std::mt19937_64> gen = std::nullopt) {
//         evec res;

//         evec uvec = u->build(offset + 1, gen);
//         evec vvec = v->build(offset + 2, gen);

//         int pur = 0, poc = 0, por = 0;

//         res.push_back(event_t(EType::Epush, offset, val_t(offset, 0), 0));

//         res.insert(res.end(), uvec.begin(), uvec.end());
//         if(gen.has_value()) {
//             int_dist purdist(1, res.size());
//             pur = purdist(gen.value());
//         }
//         res.insert(res.begin() + pur, event_t(EType::Ereturn, offset, {}, 0));

//         if(gen.has_value()) {
//             int_dist pocdist(pur + 1, res.size());
//             poc = pocdist(gen.value());;
//             assert(poc > pur);
//         }
//         res.insert(res.begin() + poc, event_t(EType::Epop, offset, val_t(offset, 0), 0));
//         if(gen.has_value()) {
//             int_dist pordist(res.size(), res.size() + vvec.size());
//             por = pordist(gen.value());
//         }
//         res.insert(res.end(), vvec.begin(), vvec.end());

//         res.insert(res.begin() + por, event_t(EType::Ereturn, offset, {}, 0));



//         return res;
//     }

//     PushPop(Rule *u, Rule *v) : Rule(), u(u), v(v) {}
//     ~PushPop() { delete u; delete v; }
// };

// Rule *build_history(int size) {
//     if(size == 0)
//         return new struct Epsilon();

//     std::random_device r;
//     std::mt19937_64 gen(r());
//     std::uniform_int_distribution<std::mt19937_64::result_type> dist(0, size - 1);
//     int split = dist(gen);

//     return new struct PushPop(
//         build_history(split),
//         build_history(size - split - 1));
// }

Component *build_history(int size) {
    if(size == 0)
        return new Empty();
    if(size == 1)
        return new Enclose(new Empty());

    std::random_device r;
    std::mt19937_64 gen(r());
    int_dist ruletype(0,1);
    int type = ruletype(gen);
    if(type == 0)
        return new Enclose(build_history(size-1));

    int_dist dist(1, size-1);
    int split = dist(gen);
    assert(split > 0);
    assert(size-split > 0);
    return new Concat(build_history(split), build_history(size-split));
}


std::vector<event_t> generate_lin_hist(int size) {
    Component *constr = build_history(size);

    std::random_device r;
    std::mt19937_64 gen(r());

    std::vector<event_t> hist = constr->build(1, gen);

    delete constr;

    return hist;
}


#endif // BUILDER_H_
