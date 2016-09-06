
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/logging.h>

#ifdef DART_ENABLE_NANOS

#include <nanos.h>

struct nanos_wd_definition
{
     nanos_const_wd_definition_t base;
     nanos_device_t devices[1];
};

static const nanos_const_wd_definition_t const_wd_def =
  {{
      .mandatory_creation = true,
      .tied = true
   }, // props
   0, // data_alignment
   0, // num_copies
   1, // num_devices
   0, // num_dimensions
   NULL // description
  };

/**
 * \brief Add a task the local task graph with dependencies. Tasks may define new tasks if necessary.
 */
dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, dart_task_dep_t *deps, size_t ndeps)
{
  size_t i;
  nanos_err_t ret;
  nanos_wd_t                wd1;
  nanos_wd_dyn_props_t      dyn_props = {0};
  dart_unit_t myid;
  dart_myid(&myid);
  // TODO: can we make use of multidimensional dependencies at some point?
  nanos_region_dimension_t  dimensions[1] = {{sizeof(char), 0, sizeof(char)}};

  nanos_data_access_t      *data_accesses = calloc(ndeps, sizeof(nanos_data_access_t));
  for (i = 0; i < ndeps; i++)
  {
    dart_task_dep_t *dep = &deps[i];
    nanos_data_access_t *da = &data_accesses[i];
    // TODO: distinguish between local and global addresses!
    if (dep->gptr.unitid == myid) {
      da->address = dep->gptr.addr_or_offs.addr;
    } else {
      DART_LOG_ERROR("dart_task_create ! Cannot handle global dependencies yet!");
    }

    // map dependency type information
    switch(dep->type) {
    case DART_DEP_IN:
      da->flags.input  = 1;
      break;
    case DART_DEP_OUT:
      da->flags.output = 1;
      break;
    case DART_DEP_INOUT:
      da->flags.input  = 1;
      da->flags.output = 1;
      break;
    default:
      DART_LOG_ERROR("dart_task_create ! Unknown dependency type: %i", dep->type);
      break;
    }
  }
  nanos_smp_args_t smp_args = { fn };
  struct nanos_wd_definition wd_data = {
      const_wd_def,
      {
         nanos_smp_factory,
         &smp_args
      }
  };

  // TODO: check whether the argument data handling is correct!
  ret = nanos_create_wd_compact( &wd1, &wd_data.base, &dyn_props, sizeof(void*), &data, nanos_current_wd(), NULL, NULL );
  if (ret != NANOS_OK) {
    DART_LOG_ERROR("dart_task_create ! Failed to create nanos work descriptor, ret=%i", ret);
    return DART_ERR_INVAL;
  }
  ret = nanos_submit( wd1, ndeps, data_accesses, 0 );
  if (ret != NANOS_OK) {
    DART_LOG_ERROR("dart_task_create ! Failed to submit nanos work descriptor, ret=%i", ret);
    return DART_ERR_INVAL;
  }
  return DART_OK;
}


/**
 * \brief Wait for all defined tasks to complete.
 */
dart_ret_t
dart__base__tasking__task_complete()
{
  nanos_err_t ret = nanos_wg_wait_completion( nanos_current_wd(), false );
  if (ret != NANOS_OK) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

#endif /* DART_ENABLE_NANOS */

