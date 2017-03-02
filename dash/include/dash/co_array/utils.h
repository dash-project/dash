#ifndef DASH__COARRAY_UTILS_H__
#define DASH__COARRAY_UTILS_H__

namespace dash {
namespace co_array {

  inline dash::global_unit_t this_image() {
    return dash::myid();
  }
  
  inline ssize_t num_images() {
    return dash::size();
  }
  
  

} // namespace co_array
} // namespace dash


#endif /* DASH__COARRAY_UTILS_H__ */

