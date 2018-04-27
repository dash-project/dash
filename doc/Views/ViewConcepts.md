
# Range- and View Expressions

## Cartesian Set Operations

### `index`

Maps elements in a view to their index in the view's origin domain.


### `sub<D>(ob,oe)`

Subsection in dimension `D` in offset range `(ob,oe]` with default
dimension `D = 0`.

In a two-dimensional matrix, for example, `sub(1,3)` selects the second
and third row.


### `section(ib,ie)`

Subsection delimited by n-dimensional indices `(ib,ie]`.


### `intersect(r)`





## Operations based on Memory Space

### `local`


### `global`


### `blocks`

Group range into domain decomposition blocks.
Elements in single block are not necessarily contiguous.


### `chunks`

Group range into chunks of sequential elements.
Elements in a chunk are contiguous in memory.



