#ifndef SEGMENTMONITOR_H_
#define SEGMENTMONITOR_H_

#include "monitor.hpp"
#include "pop_node.hpp"
#include "segment.hpp"
#include "monitor/val_node.hpp"
#include "../monitor/src/val_node.cpp" // Include the source file since it contains template functions
                                       //
class SegmentSystem : public ConstraintSystem<Segment> {
public:
        static Segment init();
        static Segment merge(Segment a, Segment b);
        static bool satisfiable(Segment a);
        static Segment from_intervals(AtomicInterval, AtomicInterval);
};

template class val_node<Segment, SegmentSystem>;

#endif // SEGMENTMONITOR_H_
