#include <assert.h>
#include <dash/dart/gaspi/dart_gaspi.h>
#include <dash/dart/gaspi/dart_translation.h>

/* Global array: the header for the global translation table. */
node_t dart_transtable_globalalloc;

int dart_adapt_transtable_create ()
{
    dart_transtable_globalalloc = NULL;
    return 0;
}

int dart_adapt_transtable_add (info_t item)
{
    node_t pre, q;
    node_t p = (node_t) malloc (sizeof(node_info_t));
    assert(p);

    p->trans.seg_id            = item.seg_id;
    p->trans.size              = item.size;
    p->trans.gaspi_seg_ids     = item.gaspi_seg_ids;
    p->trans.own_gaspi_seg_id  = item.own_gaspi_seg_id;
    p->trans.unit_count        = item.unit_count;
    p->next                    = NULL;

    /* The translation table is empty. */
    if (dart_transtable_globalalloc == NULL)
    {
        dart_transtable_globalalloc = p;

        return 0;
    }
    /* Otherwise, insert the added item into the translation table. */

    q = dart_transtable_globalalloc;
    while (q != NULL)
    {
        pre = q;
        q = q -> next;
    }
    pre->next = p;

    return 0;
}

int dart_adapt_transtable_remove (int16_t seg_id)
{
    node_t pre, p;
    p = dart_transtable_globalalloc;
    if (seg_id == (p->trans.seg_id))
    {
        dart_transtable_globalalloc = p->next;
    }
    else
    {
        while ((p != NULL ) && (seg_id != (p->trans.seg_id)))
        {
            pre = p;
            p = p->next;
        }

        if (p == NULL)
        {
            fprintf(stderr,"Invalid seg_id: %d,can't remove the record from translation table\n", seg_id);
            return -1;
        }

        pre->next = p->next;
    }
    free(p->trans.gaspi_seg_ids);
    free(p);
    return 0;
}

int dart_adapt_transtable_get_local_gaspi_seg_id(int16_t seg_id, gaspi_segment_id_t * own_segid)
{
    node_t p;
    p = dart_transtable_globalalloc;

    while ((p != NULL) && (seg_id != ((p->trans).seg_id)))
    {
        p = p->next;
    }
    if (p == NULL)
    {

        fprintf(stderr,"Invalid seg_id: %d, can not get the related window object", seg_id);
        return -1;
    }

    *own_segid = (p->trans).own_gaspi_seg_id;
    return 0;
}

int dart_adapt_transtable_get_gaspi_seg_id(int16_t seg_id, dart_unit_t rel_unitid, gaspi_segment_id_t * segid)
{
    node_t p;
    p = dart_transtable_globalalloc;

    while ((p != NULL) && (seg_id > ((p->trans).seg_id)))
    {
        p = p->next;
    }

    if ((!p) || (seg_id != (p->trans).seg_id))
    {
        fprintf(stderr,"Invalid seg_id: %d, can not get the related displacement", seg_id);
        return -1;
    }

    *segid = (p->trans).gaspi_seg_ids[rel_unitid];
    return 0;
}

int dart_adapt_transtable_add_handle(int16_t seg_id, dart_unit_t rel_unit, struct dart_handle_struct * handle)
{
    node_t p;
    p = dart_transtable_globalalloc;

    while ((p != NULL) && (seg_id > ((p->trans).seg_id)))
    {
        p = p->next;
    }

    if ((!p) || (seg_id != (p->trans).seg_id))
    {
        fprintf(stderr,"Invalid seg_id: %d, can not get the related displacement", seg_id);
        return -1;
    }

    return 0;
}

int dart_adapt_transtable_get_entry(int16_t seg_id, node_t * entry)
{
    node_t p;
    p = dart_transtable_globalalloc;

    while ((p != NULL) && (seg_id > ((p->trans).seg_id)))
    {
        p = p->next;
    }

    if ((!p) || (seg_id) != (p->trans).seg_id)
    {
        fprintf(stderr,"Invalid seg_id: %d, can not get the related memory size", seg_id);
        return -1;
    }
    *entry = p;
    return 0;
}

int dart_adapt_transtable_get_size (int16_t seg_id, size_t *size)
{
    node_t p;
    p = dart_transtable_globalalloc;

    while ((p != NULL) && (seg_id > ((p->trans).seg_id)))
    {
        p = p->next;
    }

    if ((!p) || (seg_id) != (p->trans).seg_id)
    {
        fprintf(stderr,"Invalid seg_id: %d, can not get the related memory size", seg_id);
        return -1;
    }
    *size = (p->trans).size;
    return 0;
}

int dart_adapt_transtable_destroy ()
{
    node_t p, pre;
    p = dart_transtable_globalalloc;

    while (p != NULL)
    {
        pre = p;
        p = p->next;

        free(pre->trans.gaspi_seg_ids);
        free(pre);
    }

    dart_transtable_globalalloc = NULL;
    return 0;
}
