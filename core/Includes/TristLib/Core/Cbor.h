/**
 * @file
 *
 * @brief CBOR (de)serialization helpers
 */
#ifndef TRISTLIB_CORE_CBOR_H
#define TRISTLIB_CORE_CBOR_H

#include <cbor.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>

namespace TristLib::Core {
/**
 * @brief Encode a timestamp
 *
 * This writes the timestamp as a floating point value since the UNIX epoch, and tags it as such
 * in the CBOR output.
 *
 * See RFC8949 section 3.4.2 for the details of this encoding.
 *
 * @param time Time value to encode
 */
template<class Clock>
constexpr inline static cbor_item_t *CborEncodeTimestamp(const std::chrono::time_point<Clock> time) {
    using namespace std::chrono;

    auto timestamp = cbor_new_tag(1);
    if(!timestamp) {
        return nullptr;
    }
    cbor_tag_set_item(timestamp,
            cbor_build_float8(duration_cast<duration<double>>(time.time_since_epoch()).count()));
    return timestamp;
}

/**
 * @brief Read a CBOR integer value
 *
 * @param item CBOR item to read
 *
 * @remark The item must be an unsigned integer or the results are undefined.
 */
constexpr inline static uint64_t CborReadUint(const cbor_item_t *item) {
    if(!cbor_isa_uint(item)) {
        throw std::runtime_error("invalid type (expected uint)");
    }

    switch(cbor_int_get_width(item)) {
        case CBOR_INT_8:
            return cbor_get_uint8(item);
        case CBOR_INT_16:
            return cbor_get_uint16(item);
        case CBOR_INT_32:
            return cbor_get_uint32(item);
        case CBOR_INT_64:
            return cbor_get_uint64(item);
    }
}

/**
 * @brief Read a CBOR floating point value
 *
 * @param item CBOR item to read
 *
 * @remark The item must be a floating point value
 */
constexpr inline static double CborReadFloat(const cbor_item_t *item) {
    if(!cbor_isa_float_ctrl(item) || !cbor_is_float(item)) {
        throw std::runtime_error("invalid type (expected float)");
    }

    switch(cbor_float_get_width(item)) {
        case CBOR_FLOAT_16:
            return cbor_float_get_float2(item);
        case CBOR_FLOAT_32:
            return cbor_float_get_float4(item);
        case CBOR_FLOAT_64:
            return cbor_float_get_float8(item);
        default:
            throw std::runtime_error("invalid float width");
    }
}

/**
 * @brief Read a CBOR string
 *
 * @param item CBOR item to read
 */
inline static std::string CborReadString(const cbor_item_t *item) {
    if(!cbor_isa_string(item)) {
        throw std::runtime_error("invalid type (expected string)");
    }

    return std::string(reinterpret_cast<char *>(cbor_string_handle(item)),
            cbor_string_length(item));
}

/**
 * @brief Get the value given a key from a map
 *
 * @param key String key to look up
 *
 * @return A CBOR item if found, nullptr if not
 */
constexpr inline const cbor_item_t *CborMapGet(const cbor_item_t *map,
        const std::string_view &inKey) {
    auto rootKeys = cbor_map_handle(map);

    for(size_t i = 0; i < cbor_map_size(map); i++) {
        auto &pair = rootKeys[i];
        const std::string_view key{reinterpret_cast<const char *>(cbor_string_handle(pair.key)),
            cbor_string_length(pair.key)};

        if(key == inKey) {
            return pair.value;
        }
    }

    return nullptr;
}

/**
 * @brief Decode a timestamp
 *
 * Read a timestamp (previously encoded using the "epoch based date/time" mechanism) from the
 * provided CBOR item. It must be tagged (with tag 1) to be considered a valid timestamp.
 *
 * @return The read timestamp value
 */
template<class Clock = std::chrono::system_clock>
inline static std::chrono::time_point<Clock> CborReadTimestamp(const cbor_item_t *item) {
    using namespace std::chrono;
    double secs;

    // read seconds
    if(!cbor_isa_tag(item)) {
        throw std::invalid_argument("invalid argument (expected tagged item)");
    }
    auto payload = cbor_tag_item(item);
    if(cbor_isa_float_ctrl(payload)) {
        secs = CborReadFloat(payload);
    } else if(cbor_isa_uint(payload)) {
        secs = CborReadUint(payload);
    }
    cbor_decref(&payload);

    // then convert it to a chrono time point
    const auto usec = duration_cast<microseconds>(duration<double>(secs));
    return time_point<Clock>(usec);
}
}

#endif
