#include "options.h"

AMY_ASIO_NS::ip::tcp::endpoint global_options::tcp_endpoint() const {
    using namespace AMY_ASIO_NS::ip;

    return host.empty() ?
        tcp::endpoint(address_v4::loopback(), port) :
        tcp::endpoint(address::from_string(host), port);
}

amy::auth_info global_options::auth_info() const {
    return amy::auth_info(user, password);
}
