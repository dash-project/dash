#ifndef BLOCKED_DIST_H_INCLUDED
#define BLOCKED_DIST_H_INCLUDED

namespace dash
{

class BlockedDist
{
private:
  int m_blocksz;
  int m_nunits;
  int m_maxidx;
  
public:
  BlockedDist(int nunits, int blocksz, int maxidx=-1) {
    m_blocksz = blocksz;
    m_nunits  = nunits;
    m_maxidx  = maxidx;
  }
  
  int index_to_unit(int idx) {
    idx/m_blocksz
  }
  int index_to_offset(int idx);

  int unit_and_offset_to_index(int unit, int offset);


};


} // namespace dash

#endif /* BLOCKED_DIST_H_INCLUDED */
