/*@

inductive objects = nil | cons(Object, objects);

predicate_family iterator(Class c)(Iterator i, objects xs);

@*/
interface Iterator {
  boolean hasNext();
      //@ requires iterator(this.getClass())(this, ?xs);
      //@ ensures iterator(this.getClass())(this, xs) &*& result==(xs!=nil);

  Object next();
      //@ requires iterator(this.getClass())(this, ?xs) &*& xs!=nil;
      //@ ensures switch (xs) { case nil: return false; case cons(x, xs0): return iterator(this.getClass())(this, xs0) &*& result == x &*& x!=null; };
}

/*@
fixpoint Object objects_last(objects a) {
  switch (a) {
      case nil: return null;
      case cons(x, xs): return x!=null && xs == nil ? x : objects_last(xs);
  }
}

@*/