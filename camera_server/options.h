#ifndef __AMY_UTILS_HPP__
#define __AMY_UTILS_HPP__

#define USE_BOOST_ASIO 1

#include <amy/asio.hpp>
#include <amy/auth_info.hpp>

/// Global options.
struct global_options {
    /// MySQL server host.
    std::string host = "notice.com.tw";

    /// MySQL server port.
    unsigned int port = 6033;

    /// Login user.
    std::string user = "facecam";

    /// Login password.
    std::string password = "qJlGgeg9uwAPPKlS";

    /// Default schema to use.
    std::string schema = "ndocar";

    AMY_ASIO_NS::ip::tcp::endpoint tcp_endpoint() const;

    amy::auth_info auth_info() const;

}; // struct global_options

///extern global_options opts;

#endif // __AMY_UTILS_HPP__

