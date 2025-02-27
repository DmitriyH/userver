#include <consume.hpp>

#include <userver/formats/json/value_builder.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace kafka_sample {

void Consume(kafka::MessageBatchView messages) {
  for (const auto& message : messages) {
    if (!message.GetTimestamp().has_value()) {
      continue;
    }

    TESTPOINT("message_consumed", [&message] {
      formats::json::ValueBuilder builder;
      builder["key"] = message.GetKey();

      return builder.ExtractValue();
    }());
  }
}

}  // namespace kafka_sample
