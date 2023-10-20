#pragma once

#define TERRAQTT_DETAIL_READ_PROPERTIES_BEGIN(ec, read, property_length)                                     \
	std::tie(ec, read) = co_await read_properties(                                                             \
	  reader, property_length,                                                                                 \
	  [&, _property_read = std::bitset<64>{}](PropertyIdentifier _identifier) mutable ->                       \
	  typename Context::template return_type<std::size_t> {                                                    \
		  switch (_identifier)
#define TERRAQTT_DETAIL_READ_PROPERTY_END()                                                                  \
	co_return reader.context().return_value(Error::bad_property_identifier, std::size_t{ 0 });                 \
	})

#define TERRAQTT_DETAIL_PROPERTY_CASE(identifier, value)                                                     \
	case PropertyIdentifier::identifier: {                                                                     \
		TERRAQTT_DETAIL_PROPERTY_CHECK_DUPLICATE()                                                               \
		_property_read[static_cast<int>(_identifier)] = true;                                                    \
		co_return co_await reader.read_fields(value);                                                            \
	}
#define TERRAQTT_DETAIL_PROPERTY_CASE_DUPLICATE(identifier, value)                                           \
	_property_read[static_cast<int>(_identifier)] = true;                                                      \
	co_return co_await reader.read_fields(value);

#define TERRAQTT_DETAIL_PROPERTY_CASE_BEGIN(identifier, value)                                               \
	case PropertyIdentifier::identifier: {                                                                     \
		TERRAQTT_DETAIL_PROPERTY_CHECK_DUPLICATE()                                                               \
		auto [_ec, _read] = co_await reader.read_fields(value);
#define TERRAQTT_DETAIL_PROPERTY_CASE_BEGIN_DUPLICATE(identifier, value)                                     \
	case PropertyIdentifier::identifier: {                                                                     \
		auto [_ec, _read] = co_await reader.read_fields(value);
#define TERRAQTT_DETAIL_PROPERTY_CASE_END()                                                                  \
	_property_read[static_cast<int>(_identifier)] = true;                                                      \
	co_return reader.context().return_value(_ec, _read);                                                       \
	}

#define TERRAQTT_DETAIL_PROPERTY_CHECK_DUPLICATE()                                                           \
	if (_property_read[static_cast<int>(_identifier)]) {                                                       \
		co_return reader.context().return_value(static_cast<Error>(4000 + static_cast<int>(_identifier)),        \
		                                        std::size_t{ 0 });                                               \
	}
#define TERRAQTT_DETAIL_PROPERTY_CHECK_AND_SET(condition, error_value)                                       \
	if (!_ec && (condition)) {                                                                                 \
		_ec = error_value;                                                                                       \
	}
