/**
 * \file dart/base/domain_map.c
 *
 */
#include <dash/dart/if/dart_types.h>

#include <dash/dart/base/domain_map.h>

#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/string.h>
#include <dash/dart/base/locality.h>

#include <string.h>
#include <stdlib.h>


dart__base__domain_tree_node_t dart__base__domain_tree;

/* ======================================================================== *
 * Private functions                                                        *
 * ======================================================================== */

dart_ret_t dart__base__domain_tree__find_node(
  const char                      * domain_tag,
  dart__base__domain_tree_node_t ** domain_tree_node)
{
  DART_LOG_TRACE("dart__base__domain_tree__find_node() domain(%s)",
                 domain_tag);

  dart__base__domain_tree_node_t * node = &dart__base__domain_tree;

  *domain_tree_node = NULL;

  /* pointer to dot separator in front of tag part:  */
  const char * dot_begin = domain_tag;
  /* pointer to dot separator in after tag part:     */
  char *       dot_end   = (char *)(domain_tag) + 1;
  /* Iterate tag (.1.2.3) by parts (1, 2, 3):        */
  while (*dot_end != '\0') {
    /* domain tag part converted to int is relative index of child: */
    long tag_part  = strtol(dot_begin, &dot_end, 10);
    int  child_idx = (int)(tag_part);
    if (node == NULL) {
      /* tree leaf node reached before last tag part: */
      return DART_ERR_INVAL;
    }
    if (node->num_child_nodes <= child_idx) {
      /* child index out of range: */
      return DART_ERR_INVAL;
    }
    /* descend to child at relative index: */
    node      = node->child_nodes + child_idx;
    /* continue scanning of domain tag at next part: */
    dot_begin = dot_end;
  }
  *domain_tree_node = node;

  DART_LOG_TRACE("dart__base__domain_tree__find_node > domain(%s)",
                 domain_tag);
  return DART_OK;
}

dart_ret_t dart__base__domain_tree__free_node(
  dart__base__domain_tree_node_t * domain_tree_node)
{
  if (domain_tree_node == NULL) {
    return DART_OK;
  }
  /* deallocate child nodes in depth-first recursion: */
  int num_child_nodes = domain_tree_node->num_child_nodes;
  for (int child_idx = 0; child_idx < num_child_nodes; ++child_idx) {
    if (domain_tree_node->num_child_nodes <= child_idx) {
      /* child index out of range: */
      return DART_ERR_INVAL;
    }
    if (domain_tree_node->child_nodes == NULL) {
      /* child nodes field is unspecified: */
      return DART_ERR_INVAL;
    }
    dart__base__domain_tree__free_node(
      domain_tree_node->child_nodes + child_idx);
  }
  /* deallocate node itself: */
  free(domain_tree_node->child_nodes);
  domain_tree_node->child_nodes     = NULL;
  domain_tree_node->num_child_nodes = 0;

  return DART_OK;
}

/* ======================================================================== *
 * Public functions                                                         *
 * ======================================================================== */

dart_ret_t dart__base__domain_map__init()
{
  DART_LOG_TRACE("dart__base__domain_map__init()");

  /* default-construct locality domain of top level node: */
  dart_domain_locality_t * blank_global_domain_loc =
    (dart_domain_locality_t *)(malloc(sizeof(dart_domain_locality_t)));
  dart__base__locality__domain_locality_init(blank_global_domain_loc);

  dart__base__domain_tree.num_child_nodes = 0;
  dart__base__domain_tree.child_nodes     = NULL;
  dart__base__domain_tree.level           = 0;
  dart__base__domain_tree.relative_id     = 0;
  dart__base__domain_tree.domain          = blank_global_domain_loc;

  DART_LOG_TRACE("dart__base__domain_map__init >");
  return DART_OK;
}

dart_ret_t dart__base__domain_map__finalize()
{
  DART_LOG_TRACE("dart__base__domain_map__finalize()");
  dart_ret_t ret = dart__base__domain_tree__free_node(
                     &dart__base__domain_tree);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__domain_map__finalize ! "
                   "dart__base__domain_tree__free_node failed (%d)", ret);
    return ret;
  }
  DART_LOG_TRACE("dart__base__domain_map__finalize >");
  return DART_OK;
}

dart_ret_t dart__base__domain_map__add_subdomains(
  const char             * domain_tag,
  dart_domain_locality_t * child_domains,
  int                      num_child_domains)
{
  dart__unused(domain_tag);
  dart__unused(child_domains);
  dart__unused(num_child_domains);

  dart__base__domain_tree_node_t * node;
  /* find node in domain tree for specified domain tag: */
  if (dart__base__domain_tree__find_node(domain_tag, &node) != DART_OK) {
    return DART_ERR_INVAL;
  }

  /* allocate child nodes: */
  node->num_child_nodes = num_child_domains;
  node->child_nodes     = (dart__base__domain_tree_node_t *)(
                            malloc(
                              num_child_domains *
                              sizeof(dart__base__domain_tree_node_t)));
  /* initialize child nodes: */
  int child_level = node->level + 1;
  for (int child_idx = 0; child_idx < num_child_domains; ++child_idx) {
    node->child_nodes[child_idx].level           = child_level;
    node->child_nodes[child_idx].relative_id     = child_idx;
    node->child_nodes[child_idx].domain          = child_domains + child_idx;
    node->child_nodes[child_idx].num_child_nodes = 0;
    node->child_nodes[child_idx].child_nodes     = NULL;
  }

  return DART_OK;
}

dart_ret_t dart__base__domain_map__find(
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_TRACE("dart__base__domain_map__find() domain(%s)", domain_tag);
  dart__base__domain_tree_node_t * node;
  /* find node in domain tree for specified domain tag: */
  if (dart__base__domain_tree__find_node(domain_tag, &node) != DART_OK) {
    *locality = NULL;
    return DART_ERR_INVAL;
  }
  *locality = node->domain;

  DART_LOG_TRACE("dart__base__domain_map__find > domain(%s) loc(%p)",
                 domain_tag, *locality);
  return DART_OK;
}
