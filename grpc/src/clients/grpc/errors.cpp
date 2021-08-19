#include <userver/clients/grpc/errors.hpp>

#include <utility>

#include <fmt/format.h>

namespace clients::grpc {

BaseError::BaseError(std::string message) : message_(std::move(message)) {}

const char* BaseError::what() const noexcept { return message_.c_str(); }

RpcError::RpcError(std::string_view call_name, std::string_view additional_info)
    : BaseError(fmt::format("'{}' failed: {}", call_name, additional_info)) {}

ErrorWithStatus::ErrorWithStatus(std::string_view call_name,
                                 ::grpc::Status&& status)
    : RpcError(call_name,
               fmt::format("code={}, message='{}', details='{}'",
                           status.error_code(), status.error_message(),
                           status.error_details())),
      status_(std::move(status)) {}

UnknownRpcError::UnknownRpcError(std::string_view call_name,
                                 std::string_view stage)
    : RpcError(call_name, fmt::format("an error at {} call", stage)) {}

const ::grpc::Status& ErrorWithStatus::GetStatus() const noexcept {
  return status_;
}

UnexpectedEndOfInput::UnexpectedEndOfInput(std::string_view call_name)
    : RpcError(call_name, "end-of-input reached before 'WritesDone'") {}

namespace impl {

[[noreturn]] void ThrowErrorWithStatus(std::string_view call_name,
                                       ::grpc::Status&& status) {
  switch (status.error_code()) {
    case ::grpc::StatusCode::CANCELLED:
      throw CancelledError(call_name, std::move(status));
    case ::grpc::StatusCode::UNKNOWN:
      throw UnknownError(call_name, std::move(status));
    case ::grpc::StatusCode::INVALID_ARGUMENT:
      throw InvalidArgumentError(call_name, std::move(status));
    case ::grpc::StatusCode::DEADLINE_EXCEEDED:
      throw DeadlineExceededError(call_name, std::move(status));
    case ::grpc::StatusCode::NOT_FOUND:
      throw NotFoundError(call_name, std::move(status));
    case ::grpc::StatusCode::ALREADY_EXISTS:
      throw AlreadyExistsError(call_name, std::move(status));
    case ::grpc::StatusCode::PERMISSION_DENIED:
      throw PermissionDeniedError(call_name, std::move(status));
    case ::grpc::StatusCode::RESOURCE_EXHAUSTED:
      throw ResourceExhaustedError(call_name, std::move(status));
    case ::grpc::StatusCode::FAILED_PRECONDITION:
      throw FailedPreconditionError(call_name, std::move(status));
    case ::grpc::StatusCode::ABORTED:
      throw AbortedError(call_name, std::move(status));
    case ::grpc::StatusCode::OUT_OF_RANGE:
      throw OutOfRangeError(call_name, std::move(status));
    case ::grpc::StatusCode::UNIMPLEMENTED:
      throw UnimplementedError(call_name, std::move(status));
    case ::grpc::StatusCode::INTERNAL:
      throw InternalError(call_name, std::move(status));
    case ::grpc::StatusCode::UNAVAILABLE:
      throw UnavailableError(call_name, std::move(status));
    case ::grpc::StatusCode::DATA_LOSS:
      throw DataLossError(call_name, std::move(status));
    case ::grpc::StatusCode::UNAUTHENTICATED:
      throw UnauthenticatedError(call_name, std::move(status));
    default:
      throw UnknownError(call_name, std::move(status));
  }
}

}  // namespace impl

}  // namespace clients::grpc
