#ifndef SEGMENT_H_
#define SEGMENT_H_

#include "typedef.h"
#include "interval.h"
#include <memory>
#include <set>
#include <cstring>
#include <cassert>
#include <algorithm>

inline bool ordered_as(AtomicInterval a, AtomicInterval b, AtomicInterval c, AtomicInterval d) {
    return a.preceeds(b) && b.preceeds(c) && c.preceeds(d);
}

inline bool invalid(ADT adt, AtomicInterval aa, AtomicInterval ra, AtomicInterval ab, AtomicInterval rb) {
    bool abab = ordered_as(aa, ab, ra, rb);
    bool baba = ordered_as(ab, aa, rb, ra);
    bool abba = ordered_as(aa, ab, rb, ra);
    bool baab = ordered_as(ab, aa, ra, rb);
    switch(adt) {
        case stack:
            return abab || baba;
        case queue:
            return abba || baab;
    }
    throw std::logic_error("Unhandled ADT in \"invalid\" function");
}

inline bool valid(ADT adt, AtomicInterval add_a, AtomicInterval rmv_a, AtomicInterval add_b, AtomicInterval rmv_b) {
    return !invalid(adt, add_a, rmv_a, add_b, rmv_b);
}

struct Matrix {
    index_t rows, cols;
    bool *data;

    bool all_false() const {
        for(int i = 0; i < rows * cols; i++) {
            if(data[i])
                return false;
        }
        return true;
    }

    Matrix(int rows, int cols, bool init = true) : rows(rows), cols(cols), data(nullptr) {
        data = new bool[rows * cols];
        for(int i = 0; i < rows * cols; i++) {
            data[i] = init;
        }
    }
    Matrix() : rows(0), cols(0), data(nullptr) {}
    ~Matrix() {
        if(data != nullptr) {
            delete[] data;
        }
    }

    int getidx(int row, int col) const { return row * cols + col; }

    bool at(int row, int col) const { assert(col < cols && row < rows); return data[getidx(row, col)]; }

    void set(int row, int col, bool val) {
        data[getidx(row, col)] = val;
    }

    void set_or(int row, int col, bool val) {
        data[getidx(row, col)] = data[getidx(row, col)] || val;
    }

    Matrix operator+(const Matrix &o) {
        assert(rows == o.rows && cols == o.cols);
        Matrix m = Matrix(rows, cols);
        for(int c = 0; c < cols; c++) {
            for(int r = 0; r < rows; r++) {
                m.set(r, c, at(r, c) || o.at(r,c));
            }
        }
        return m;
    }

    Matrix operator*(const Matrix &o) {
        assert(rows == o.rows && cols == o.cols);
        Matrix m = Matrix(rows, cols);
        for(int c = 0; c < cols; c++) {
            for(int r = 0; r < rows; r++) {
                m.set(r, c, (at(r, c) && o.at(r,c)));
            }
        }
        return m;
    }

    Matrix(const Matrix &m) : rows(m.rows), cols(m.cols), data(new bool[m.rows * m.cols]) {
        for(int r = 0; r < m.rows; r++) {
            for(int c = 0; c < m.cols; c++) {
                set(r, c, m.at(r, c));
            }
        }
    }

    Matrix &operator=(const Matrix &m) {
        if(data != nullptr) {
            delete[] data;
        }

        rows = m.rows;
        cols = m.cols;
        data = new bool[rows * cols];
        for(int i = 0; i < rows * cols; i++) {
            data[i] = m.data[i];
        }
        return *this;
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix &m) {
        for(int r = 0; r < m.rows; r++) {
            if(r > 0)
                os << std::endl;
            os << "|";
            for(int c = 0; c < m.cols; c++) {
                os << (m.at(r, c) ? "1" : "0");
            }
            os << "|";
        }
        return os;
    }
};

struct Segmentation {
    std::vector<timestamp_t> add_ts;
    std::vector<timestamp_t> rmv_ts;
    Matrix valid;

    AtomicInterval add(int idx) const {
        assert(idx >= 0 && idx < (add_ts.size() - 1));
        return AtomicInterval::closedopen(add_ts[idx], add_ts[idx+1]);
    }

    AtomicInterval rmv(int idx) const {
        assert(idx >= 0 && idx < (rmv_ts.size() - 1));
        return AtomicInterval::closedopen(rmv_ts[idx], rmv_ts[idx+1]);
    }



    friend std::ostream &operator<<(std::ostream &out, const Segmentation &s) {
        for(int i = 0; i < s.add_ts.size() - 1; i++) {
            out << s.add(i) << ":\t";
            for(int j = 0; j < s.rmv_ts.size() - 1; j++){
                if(s.valid.at(i, j)) {
                    out << s.rmv(j);
                }
            }
            out << std::endl;
        }
        return out;
    }

    Segmentation(std::vector<timestamp_t> add_ts, std::vector<timestamp_t> rmv_ts)
        : add_ts(add_ts.begin(), add_ts.end())
        , rmv_ts(rmv_ts.begin(), rmv_ts.end())
        , valid(add_ts.size() - 1, rmv_ts.size() - 1) {}
    Segmentation() : add_ts({}), rmv_ts({}), valid(0,0) {}
    Segmentation(const Segmentation &o) : Segmentation(o.add_ts, o.rmv_ts) {
        valid = o.valid;
    }

    Segmentation &operator=(const Segmentation &o) {
        add_ts = std::vector(o.add_ts.begin(), o.add_ts.end());
        rmv_ts = std::vector(o.rmv_ts.begin(), o.rmv_ts.end());
        valid = o.valid;
        return *this;
    }
    ~Segmentation() = default;

};


struct SegmentBuilder {
    timestamp_t add_call, add_ret, rmv_call, rmv_ret;

    SegmentBuilder() : SegmentBuilder(NEGINF, POSINF, NEGINF, POSINF) {}
    SegmentBuilder(timestamp_t add_call, timestamp_t add_ret, timestamp_t rmv_call, timestamp_t rmv_ret)
        : add_call(add_call)
        , add_ret(add_ret)
        , rmv_call(rmv_call)
        , rmv_ret(rmv_ret) {}

    SegmentBuilder(AtomicInterval add, AtomicInterval rmv)
        : SegmentBuilder(add.lbound, add.ubound, rmv.lbound, rmv.ubound) {}

    Segmentation *Segmentize(std::vector<timestamp_t> &conc) {
        assert(add_call < POSINF && add_ret < POSINF && rmv_call < POSINF && rmv_ret < POSINF);
        assert(add_call > NEGINF && add_ret > NEGINF && rmv_call > NEGINF && rmv_ret > NEGINF);
        std::vector<timestamp_t> add;
        std::vector<timestamp_t> rmv;

        std::copy_if(conc.begin(), conc.end(), std::inserter(add, add.end()), [&](timestamp_t ts) {
            assert(ts < POSINF && ts > NEGINF);
            return add_call < ts && ts < add_ret;
        });

        add.insert(add.begin(), add_call);
        add.insert(add.end(), add_ret);


        std::copy_if(conc.begin(), conc.end(), std::inserter(rmv, rmv.end()), [&](timestamp_t ts) {
            return rmv_call < ts && ts < rmv_ret;
        });
        rmv.insert(rmv.begin(), rmv_call);
        rmv.insert(rmv.end(), rmv_ret);

        // std::vector<timestamp_t> n_add;
        // std::vector<timestamp_t> n_rmv;
        // for(int i = 0; i < add.size(); i++) {
        //     n_add.add_back(add[i] * 3);
        //     if(i != add.size() - 1)
        //         n_add.add_back((add[i] * 3) + 1); // Add offset-by-one to fix error
        //     if(i != 0)
        //         n_add.add_back((add[i] * 3) - 1);
        // }
        // for(int i = 0; i < rmv.size(); i++) {
        //     n_rmv.add_back(rmv[i] * 3);
        //     if(i != rmv.size() - 1)
        //         n_rmv.add_back((rmv[i] * 3) + 1); // Add offset-by-one to fix error
        //     if(i != 0)
        //         n_rmv.add_back((rmv[i] * 3) - 1); // add offset-by-one to fix error
        // }

        return new Segmentation(add, rmv);
    }

    std::vector<timestamp_t> timestamps() {
        return std::vector({add_call, add_ret, rmv_call, rmv_ret});
    }
};


std::ostream &operator<<(std::ostream &os, SegmentBuilder &bld);

#endif // SEGMENT_H_
