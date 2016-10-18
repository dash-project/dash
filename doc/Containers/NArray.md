# The DASH Multidimensional Array (NArray)

## Instantiating a DASH NArray

`dash::NArray` is an N-dimensional array container class template. Its constructor requires
at least two template arguments, one for the element type `(int, double, ...)` and one for
the dimension `(N)`.

The following example creates a two-dimensional integer matrix with
40 rows and 30 columns:

~~~c++
dash::NArray<int,2> matrix(40,30); // 1200 elements
~~~

The number of required runtime constructor arguments depends on the dimension of
the `dash::NArray`. Except for a default constructed `dash::NArray` object, which requires
no arguments and is used for delayed allocation (see Sect. 1.6.1), we need to at least
specify the extents of the multidimensional array in each dimension. This can be achieved
by either passing an object of type `dash::SizeSpec<N>` or by giving the extent in each
dimension individually. For example:

~~~c++
// use SizeSpec 20 x30x40 elements:
dash::SizeSpec<3> sspec(20,30, 40);
dash::NArray<int, 3> mat1(sspec );
dash::NArray<int, 3> mat2(dash::SizeSpec<3>(5,5,5));
// specify extents directly, 10000 elem in a 4D cube:
dash::NArray<int, 4> mat3(10,10,10,10);
// unallocated matrices, .allocate() call needed:
dash::NArray<int, 3> mat4, mat5;
// .allocate() accepts similar inputs as the constructor:
mat4.allocate(20,40,10);
mat5.allocate(sspec);
~~~

Further optional template arguments to specify the index type and storage order (also
called memory arrangement) can be provided when instantiating the `dash::NArray` object.
An example multidimensional array with column-major layout and long as the index type
is shown below:

~~~c++
dash::NArray<int, 5, dash::COL_MAJOR, long>
matrix(100, 100, 100, 100, 100) ; // 10^10 = 2^33.2 elements 
~~~

In this example, the number of elements exceeds the range of a 32 bit index type (the
default is `ssize_t` which can be overridden using a build option in DASH) and a 64 bit
index type is thus required.
Additionally, DASH offers both column-major and row-major storag for the elements
(row-major is the default). These options determine the way in which the multi-dimensional
index space is linearized and mapped on to the one-dimensional memory that computers work
with.

For two-dimensional arrays this linearization can either happen row by row (aka. rowmajor
storage) or column by column (aka. column-major storage). For arbitrary dimensions these
definitions can be suitably extended and row-major then means that elements
$(i, j,..., n)$ and $(i, j,..., n + 1)$ are stored next to each other, while in the case of
columnmajor storage $(i, j,..., n)$ and $(i + 1, j, ...)$ are stored next to each other
(not taking into account any possible data distribution among multiple units).

Fig. 1 visualizes the layout of elements in a two-dimensional `dash::NArray`, both with
row-major storage.
A final optional template argument specifies the type of the data distribution pattern
used by the `dash::NArray` to determine the mapping of elements to units and finding their
storage location. The following example shows the most general form, where all template
parameters are explicitly specified.

~~~c++
using T        = int;                 // element value type
const int NDim = 2;                   // number of dimensions
const auto Arr = dash::COL_MAJOR;     // memory arrangement
using PatT     = dash::Pattern<NDim>; // pattern type
using IndexT   = PatT::index_type;    // index type

dash::NArray<T, NDim, Arr, IndexT, PatT> mat;
~~~

A `dash::NArray` is always allocated over a `dash::Team`, i.e., a group of units that contribute
storage to hold the data for the container. A team can be specified as an optional last
constructor argument. If no team is explicitly specified, it defaults to `dash::Team::All()`,
the group of all units that exits when the program starts.

~~~c++
dash::Team & t = dash::Team::All().split (2);
// 100x100 elements allocated over t:
dash::NArray<int,2> mat(100,100,t);
~~~

