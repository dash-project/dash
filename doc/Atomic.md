DASH provides an easy to use interface for atomic operations similar to `std::atomic<T>` for elements stored in DASH containers. The STL atomic concept cannot be ported 1:1 as remote accesses require copying (amongst others).

**The interface is as follows:**

- Phantom type `dash::Atomic<T>` which can (only) be placed in DASH containers.
- Specialisation of `dash::GlobRef<Atomic<T>>` which provides interface of `std::Atomic` and proxies it to atomic DART calls. Methods which cannot be performed atomically are disabled / deleted.
- Function-based interface `dash::atomic::` (works only for atomic types)
- DART interface for atomic operations which internally uses the corresponding MPI operations.
- Static assertions to check weather atomic operations on the underlying type are supported

**Currently neither implemented nor specified:**

- memory model information
- memory model traits
- handling of local accesses e.g. `array.lbegin().load()`

**Mixing atomic and non-atomic accesses**

As the constructor of `dash::GlobRef<T>` takes a DART gptr which is not strongly-typed, atomic accesses to non-atomic elements (and vice versa) are possible. In some cases this might be beneficial, as atomic operations are expensive compared to non-atomic ones.
However between sections with non-atomic and atomic operations there has to be a **barrier and a memory flush**. Otherwise the behavior is undefined. Furthermore we do not recommend using this "hack" at all.

An example of this is `dash::Shared` which provides both non-atomic (`shared.get()`) and atomic (`shared.atomic.get()`) access.

**Container Example:**
```c++
dash::Array<dash::Atomic<int>> array(100);
// supported as Atomic<value_t>(value_t T) is available
dash::fill(array.begin(), array.end(), 0);

if(dash::myid() == 0){
  array[10].store(5);
  }
dash::barrier();
array[10].add(1);
// postcondition:
// array[10] == dash::size() + 5
```

The following does not work:
```c++
dash::Array<dash::Atomic<int>> array(100);
array.local[10].load();               // not allowed
dash::atomic::load(array.local[10]);  // not allowed
dash::atomic::load(array.lbegin());   // not allowed
```
