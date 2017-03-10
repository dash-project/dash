#ifndef DASH__PATTERN_PROPERTIES_H__INCLUDED
#define DASH__PATTERN_PROPERTIES_H__INCLUDED

#include <dash/Types.h>
#include <sstream>
#include <iostream>

namespace dash {

#ifdef __TODO__

struct pattern_partitioning_tag
{
  typedef enum {
    any,

    /// Block extents are constant for every dimension.
    rectangular,

    /// Minimal number of blocks in every dimension, typically at most one
    /// block per unit.
    minimal,

    /// All blocks have identical extents.
    regular,

    /// All blocks have identical size.
    balanced,

    /// Size of blocks may differ.
    unbalanced,

    /// Data range is partitioned in at least two dimensions.
    ndimensional,

    /// Data range is partitioned dynamically.
    dynamic

  } type;
};
 
struct pattern_mapping_tag
{
  typedef enum {
    /// Unspecified mapping property.
    any,

    /// The number of assigned blocks is identical for every unit.
    balanced,

    /// The number of blocks assigned to units may differ.
    unbalanced,

    /// Adjacent blocks in any dimension are located at a remote unit.
    neighbor,

    /// Units are mapped to blocks in diagonal chains in at least one
    /// hyperplane
    shifted,

    /// Units are mapped to blocks in diagonal chains in all hyperplanes.
    diagonal,

    /// Units are mapped to more than one block. For minimal partitioning,
    /// every unit is mapped to two blocks.
    multiple,

    /// Blocks are assigned to processes like dealt from a deck of
    /// cards in every hyperplane, starting from first unit.
    cyclic

  } type;
};

struct pattern_layout_tag
{
  typedef enum {
    /// Unspecified layout property.
    any,

    /// Row major storage order, used by default.
    row_major,

    /// Column major storage order.
    col_major,

    /// Elements are contiguous in local memory within a single block
    /// and thus indexed blockwise.
    blocked,

    /// All local indices are mapped to a single logical index domain
    /// and thus not indexed blockwise.
    canonical,

    /// Local element order corresponds to a logical linearization
    /// within single blocks (if blocked) or within entire local memory
    /// (if canonical).
    linear

  } type;
};


// NOTES:
//
// Pattern traits result from the combination of:
//
// - Pattern type traits (partitioning, mapping, layout tags)
// - Partitioning spec (blocked, blockcyclic<Bd>, tiled<Td>)
// - Container type
// 
// Example:
//
// - Pattern type:
//     BlockPattern<2>
//
//     -> Guaranteed:
//
//        - partitioning { rectangular }
//        - mapping      {  }
//        - layout       { canonical, linear }
//
//     -> Additionally satisfiable depending on partitioning spec
//        but for any data domain shape:
//
//        - partitioning { minimal }
//                            `--- but not regular or balanced as these
//                                 traits depend on data domain size
//        - mapping      { balanced, multiple, cyclic }
//        - layout       { }
//
// - Partitioning spec:
//     Partitioning<blocked, none>
//
//     -> Result:
//        - partitioning { *minimal, rectangular }
//                            `--- 1-dim blocked is one block
//                                 per unit -> minimal
//        - mapping      { *balanced }
//        - layout       { *blocked, canonical, linear }
//                            `--- layout is canonical but identical to
//                                 blocked layout in this configuration
//                                            
//    

/**
 * Usage:
 *
 * \code
 * dash::pattern_partitioning_spec<
 *   dash::blocked,
 *   dash::blockcyclic<8>
 * > 
 * \endcode
 */
template <>
struct pattern_partitioning_spec<void>
{
  typedef void partitioning;
};

template <class... PartitioningType>
struct pattern_partitioning_spec<void, PartitioningType... >
{
  typedef 
};

#endif

} // namespace dash

#endif // DASH__PATTERN_PROPERTIES_H__INCLUDED
