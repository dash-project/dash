#include "dart_team_private.h"
#include "dart_gaspi.h"

dart_ret_t dart_barrier (dart_team_t teamid)
{
    gaspi_group_t gaspi_group_id;
    uint16_t index;
    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group_id = dart_teams[index].id;
    DART_CHECK_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));
    return DART_OK;
}
