#include "relation.h"

Relation compare(const Value &v1, const Value &v2) {
  Relation r = Relation::All();
  //+a < +b
  if (v1.add.preceeds(v2.add)) {
    r ^= (r & Relation((RelVal::After | RelVal::Above)));
  }
  //+b < +a
  if (v2.add.preceeds(v1.add)) {
    r ^= (r & Relation(RelVal::Before | RelVal::Below));
  }   

  //-a < -b
  if ( v1.rmv.has_value() && v1.rmv->preceeds(v2.rmv.value()) ) {
    r ^= (r & Relation(RelVal::After | RelVal::Below));
  }
  //-b < -a
  if (v2.rmv.has_value() && v1.rmv.has_value() && v2.rmv->preceeds(v1.rmv.value())) {
    r ^= (r & Relation(RelVal::Before | RelVal::Above));
  }
      
  
  //+a < -b
  if (!v2.rmv.has_value() || v1.add.preceeds(v2.rmv.value())) {
    r ^= (r & Relation(RelVal::After));
  }
  //+b < -a
  if (!v1.rmv.has_value() || v2.add.preceeds(v1.rmv.value())) {
    r ^= (r & Relation(RelVal::Before));
  }
  //-a < +b
  if (v1.rmv.has_value() && v1.rmv->preceeds(v2.add)) {
    r ^= (r & Relation(RelVal::After | RelVal::Above | RelVal::Below));
  }
  //-b < +a
  if (v2.rmv.has_value() && v2.rmv->preceeds(v1.add)) {
    r ^= (r & Relation(RelVal::Before | RelVal::Below | RelVal::Above));
  }
  return r;
}

