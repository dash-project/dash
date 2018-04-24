
#include <gtest.h>

#include <dash/View.h>
#include <dash/PatternExpr.h>

/*
 * General findings and assumptions:
 *
 * 0)  Patterns are domain-specific views and may differ from view
 *     expressions in:
 *        - design criteria
 *        - semantics
 *        - valid expressions (= algebra)
 *
 *     Therefore, for example:
 *        - nviews do not depend on anything but the existence of a
 *          canonical index space (d'uh)
 *        - patterns are restricted to specific mapping
 *          signatures (smarter but less generic than nviews) and depend
 *          on concrete concepts such as Unit, Team, Locality, ...
 *
 *     !! nviews are a pure-mathematical concoction, patterns are
 *        abstract algorithmic building blocks.
 *
 *
 * I)  Conceptional differences between nviews and pattern views:
 *     -- nview operations first and foremost must provide a zero-cost
 *        operation abstraction
 *        ( -> efficient to pipe, pass, invoke, confabulate, mogrify)
 *
 *          some_origin | foo() | bar() | index()
 *                ... should not copy a darn thing.
 *
 *     !! this is not a priority for patterns:
 *        - pattern instantiation may be "expensive" compared to views
 *        - pattern dereferentiation aka index access like
 *           
 *            pattern | global(34) | local()
 *          or
 *            pattern | global(34) | unit()
 *
 *        - pattern expression modifiers (global, local, unit_at, ...)
 *          possibly have different semantics than view modifiers of
 *          the same name (but please should not).
 *
 *        
 *
 *
 * C)  Advantages from switching to Pattern Views:
 *
 * --  With status quo pattern (class template) definitions, domain
 *     decomposition specified by a pattern object is immutable after
 *     its instantiation
 * >>  With pattern views, data space mappings can be modified just like
 *     nviews
 *
 *
 * N)  Some Wisenheimer words:
 *
 *     - index:  some numeric reference (scalar or n-dim point) to an
 *               element position
 *     - offset: a scalar index
 *     - point:  an n-dim index
 *
 */



/*-------------------------------------------------------------------------
 * pattern/PatternExpressionDefs.h
 *
 *
 * Want:
 *
 *   template<
 *     dim_t      NumDimensions,
 *     MemArrange Arrangement   = ROW_MAJOR,
 *     typename   IndexType     = dash::default_index_t
 *   >
 *   using BlockPattern = decltype(view_expr);
 *
 *   template <...>
 *   struct pattern_mapping_properties<BlockPattern> {
 *     typedef pattern_mapping_properties<
 *                 // Number of blocks assigned to a unit may differ.
 *                 pattern_mapping_tag::unbalanced
 *             > mapping_properties;
 *   }
 *
 */

TEST_T(PatternExprTest, BlockCyclicPatternExpression)
{
    BlockPattern<1, dash::ROW_MAJOR, ssize_t> block_pat(
        nlocal * nunits,
        dash::DistributionSpec<1>(dash::BLOCKED));
  
    dash::Array<int, pattern_t> array(nlocal * nunits);

    // 

}



/*-- snip ---------------------------------------------------------------- */


// Need adapter from current pattern constructor interface to view
// expressions
BlockPattern(size_spec, dist_spec, team)
{
//> size_spec: 100 * nunits
//> dist_spec: dash::BLOCKED

    // View object configured without any modifiers, comparable to
    // dash::origin or dash::identity:
    //
    auto pat_id = make_pattern_id_view(size_spec);

    // mapping signatures:
    //
    //    (1,2)
    //
    //    gidx <--> (uidx, lidx)
    //    gidx <--> goffs
    //         ...
    //    Xidx <--> Xoffs
    //
    //    ... with X as model of one of:
    //
    //    - block index
    //    - element index
    //    - unit index
    //    ...
    //    whatev
    //
    // -  Chaining mappings of X* index spaces allow for hierarchical
    //    patterns:
    //
    //    global space    block space     sub-block space ...
    //    (Aidx, uidx) -> (Bidx, uidx) -> (Pidx, Bidx)    ... 
    //

    // Provides: 
    //   - (coord-like) <-> (offset-like)
    pat_blk = pat_id | chunk(<calculate block extents from size, team>)
    // Now provides:
    //   + (index-like) <-> (chunk range)
    //     ...
    //     that is, the following expressions are valid:
    //
    //     pat_blk[2]  ->  chunk
    //                     with chunk ~> nview
    //     Therefore:
    //
    //     pat_blk[2].extents()
    //     pat_blk[2].offsets()
    //     pat_blk[4] | index()   -> block index (THIS is the block index)
    //
    //     pat_unit_offs = pat_blk | cycle(nunits) | index()
    //     // = offset of unit in team spec, so
    //     //   team_spec[unit_offs] -> unit_id
    //


    
}

/*-- snip ---------------------------------------------------------------- */








