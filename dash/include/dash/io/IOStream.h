#ifndef DASH__IO__IOSTREM_H__INCLUDED
#define DASH__IO__IOSTREM_H__INCLUDED

#include <cstdint>
#include <type_traits>

namespace dash {
namespace io {

/**
 * Modes defined for all parallel IO devices.
 *
 * \concept{DashIOConcept}
 *
 */
enum class IOSBaseMode
#if !defined(SPEC)
: uint32_t
#endif
{
    no_flags        = 0,
    /// Append: set the stream position to the end of the stream before
    /// output operations.
    app             = 1 << 0,
    /// At End: set the stream position to the end of the stream on open.
    ate             = 1 << 1,
    /// Binary: Consider stream as raw data.
    binary          = 1 << 2,
    /// Allow input operations on the stream.
    in              = 1 << 3,
    /// Allow output operations on the stream.
    out             = 1 << 4,
    /// Truncate: discard content of the stream on open.
    trunk           = 1 << 5
}; // class IOStream

/**
 * Type facade wrapping \c dash::io::IOSBaseMode and its device-dependent
 * specializations.
 *
 * \concept{DashIOConcept}
 *
 * An IO stream implementation for a specific device defines its modes by
 * extending \c dash::io::IOSBaseMode and uses these to define type
 * specializations of \c dash::io::IOStreamMode and \c dash::io::IOSBase.
 *
 */
template<typename IOSModeType>
class IOStreamMode
{
private:

  typedef IOStreamMode<IOSModeType>              self_t;
  using   UT = std::underlying_type<IOSBaseMode>::type;

public:

  /// Constructors

  IOStreamMode(IOSModeType ios_base = IOSBaseMode::no_flags)
  : _ios_mode(ios_base)
  { }

  /// Type conversion

  inline operator IOSModeType() const {
    return _ios_mode;
  }
 
  /**
   * Returns false if and only if no flags are set.
   */
  inline operator bool() const {
    return _ios_mode != IOSBaseMode::no_flags;
  }

  /// Binary operators

  inline self_t & operator|=(const self_t & rhs) {
    _ios_mode |= rhs._ios_mode;
    return *this;
  }

  inline self_t & operator&=(const self_t & rhs) {
    _ios_mode &= rhs._ios_mode;
    return *this;
  }

  inline self_t & operator^=(const self_t & rhs) {
    _ios_mode ^= rhs._ios_mode;
    return *this;
  }

  inline self_t operator~() {
    return self_t(~_ios_mode);
  }

  inline self_t operator|(const self_t & rhs) {
    return self_t(static_cast<IOSModeType>(
                    static_cast<UT>(_ios_mode)
                    | static_cast<UT>(rhs._ios_mode))
                 );
  }

  inline self_t operator&(const self_t & rhs) {
    return self_t(static_cast<IOSModeType>(
                    static_cast<UT>(_ios_mode)
                    & static_cast<UT>(rhs._ios_mode))
                 );
  }

  inline self_t operator^(const self_t & rhs) {
    return self_t(static_cast<IOSModeType>(
                    static_cast<UT>(_ios_mode)
                    ^ static_cast<UT>(rhs._ios_mode))
                 );
  }

private:

  IOSModeType _ios_mode;

}; // class IOStream

/**
 * Base type for device-specific IO streams.
 *
 * \concept{DashIOConcept}
 *
 * An IO stream implementation for a specific device defines its modes by
 * extending \c dash::io::IOSBaseMode and uses these to define type
 * specializations of \c dash::io::IOStreamMode and \c dash::io::IOSBase.
 *
 * Example:
 *
 * \code
 * enum class MyDeviceModes : public dash::io::IOSBaseMode {
 *     // device-specific modes:
 *     mydevice_foo_mode_ = 1 << 8,
 *     mydevice_bar_mode_ = 1 << 9
 * }
 *
 * typedef dash::io::IOStreamMode<MyDeviceModes>
 *         MyDeviceStreamMode;
 *
 * class MyDeviceStream
 * : public dash::io::IOSBase<MyDeviceStreamMode> {
 * private:
 *   typedef MyDeviceStream                         self_t;
 *   typedef dash::io::IOSBase<MyDeviceStreamMode>  base_t;
 *   typedef MyDeviceStreamMode                     mode_t;
 *
 * public:
 *   // ...
 *
 *   // Device-specific stream mode modifiers:
 *
 *   inline mode_t setfoo(mode_t foo) {
 *     this->_ios_stream_mode |= foo;
 *   }
 *
 *   inline mode_t setbar(mode_t bar) {
 *     this->_ios_stream_mode |= bar;
 *   }
 *
 *   inline mode_t foo() {
 *     return this->_ios_stream_mode & mode_t::foo;
 *   }
 *
 *   inline mode_t bar() {
 *     return this->_ios_stream_mode & mode_t::bar;
 *   }
 * };
 *
 * \endcode
 *
 */
template<typename IOSModeType>
class IOSBase
{
public:

  typedef IOStreamMode<IOSModeType> ios_mode_type;

public:

  IOSBase()
  : _io_stream_mode(IOSBaseMode::no_flags)
  { }

protected:

  ios_mode_type _io_stream_mode;

}; // class IOStreamBase

} // namespace io
} // namespace dash

#endif // DASH__IO__IOSTREM_H__INCLUDED
