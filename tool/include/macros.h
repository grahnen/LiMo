#ifndef MACROS_H_
#define MACROS_H_

#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/seq.hpp>

#define EVT_TYPE (return,push,pop,enq,deq,add,rmv,ctn,crash,nil)
#define OP_TYPE (push,pop,enq,deq,add,rmv,ctn,nil)

#define EVT_TYPE_SEQ BOOST_PP_TUPLE_TO_SEQ(EVT_TYPE)
#define OP_TYPE_SEQ BOOST_PP_TUPLE_TO_SEQ(OP_TYPE)


#endif // MACROS_H_
