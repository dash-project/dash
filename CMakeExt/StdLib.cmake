# Some compilers ship with stdlib versions without std::is_trivially_copyable.
# As a fallback check for the __has_trivial_copy intrinsic which also works for
# older GCC versions.
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CMakePushCheckState)

cmake_push_check_state()
set(CMAKE_REQUIRED_FLAGS "-std=c++11")
CHECK_CXX_SOURCE_COMPILES(
  "
  #include <type_traits>
  template <class T> struct is_container_compatible :
  public std::integral_constant<bool, std::is_trivially_copyable<T>::value> {};
  int main() { return 0; }
  " HAVE_STD_TRIVIALLY_COPYABLE)

if (NOT HAVE_STD_TRIVIALLY_COPYABLE)
  CHECK_CXX_SOURCE_COMPILES(
    "
    #include <type_traits>
    template <class T> struct is_container_compatible :
    public std::integral_constant<bool, __has_trivial_copy(T)> {};
    int main() { return 0; }
    " HAVE_TRIVIAL_COPY_INTRINSIC)

    if(NOT HAVE_TRIVIAL_COPY_INTRINSIC)
      message(WARNING "Neither std::is_trivially_copyable nor __has_trivial_copy intrinsic are present.")
    endif()
endif()
cmake_pop_check_state()
