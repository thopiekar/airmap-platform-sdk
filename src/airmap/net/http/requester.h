#ifndef AIRMAP_NET_HTTP_REQUESTER_H_
#define AIRMAP_NET_HTTP_REQUESTER_H_

#include <airmap/date_time.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/outcome.h>

#include <airmap/net/http/response.h>
#include <airmap/util/formatting_logger.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

namespace airmap {
namespace net {
namespace http {

class Requester : DoNotCopyOrMove {
 public:
  using Result   = Outcome<Response, std::exception_ptr>;
  using Callback = std::function<void(const Result&)>;

  virtual void delete_(const std::string& path, std::unordered_map<std::string, std::string>&& query,
                       std::unordered_map<std::string, std::string>&& headers, Callback cb) = 0;
  virtual void get(const std::string& path, std::unordered_map<std::string, std::string>&& query,
                   std::unordered_map<std::string, std::string>&& headers, Callback cb)     = 0;
  virtual void patch(const std::string& path, std::unordered_map<std::string, std::string>&& headers,
                     const std::string& body, Callback cb)                                  = 0;
  virtual void post(const std::string& path, std::unordered_map<std::string, std::string>&& headers,
                    const std::string& body, Callback cb)                                   = 0;

 protected:
  Requester() = default;
};

// RoutingRequester prefixes incoming paths and hands them off to the next Requester implementation.
class RoutingRequester : public Requester {
 public:
  explicit RoutingRequester(const std::string& route, const std::shared_ptr<Requester>& next);

  void delete_(const std::string& path, std::unordered_map<std::string, std::string>&& query,
               std::unordered_map<std::string, std::string>&& headers, Callback cb) override;
  void get(const std::string& path, std::unordered_map<std::string, std::string>&& query,
           std::unordered_map<std::string, std::string>&& headers, Callback cb) override;
  void patch(const std::string& path, std::unordered_map<std::string, std::string>&& headers, const std::string& body,
             Callback cb) override;
  void post(const std::string& path, std::unordered_map<std::string, std::string>&& headers, const std::string& body,
            Callback cb) override;

 private:
  std::string route_;
  std::shared_ptr<Requester> next_;
};

// LoggingRequester logs requests and their completion
class LoggingRequester : public Requester {
 public:
  explicit LoggingRequester(const std::shared_ptr<Logger>& logger, const std::shared_ptr<Requester>& next);

  void delete_(const std::string& path, std::unordered_map<std::string, std::string>&& query,
               std::unordered_map<std::string, std::string>&& headers, Callback cb) override;
  void get(const std::string& path, std::unordered_map<std::string, std::string>&& query,
           std::unordered_map<std::string, std::string>&& headers, Callback cb) override;
  void patch(const std::string& path, std::unordered_map<std::string, std::string>&& headers, const std::string& body,
             Callback cb) override;
  void post(const std::string& path, std::unordered_map<std::string, std::string>&& headers, const std::string& body,
            Callback cb) override;

 private:
  util::FormattingLogger log_;
  std::shared_ptr<Requester> next_;
  std::map<std::string, DateTime> request_start_timestamps_;
};

}  // namespace http
}  // namespace net
}  // namespace airmap

#endif  // AIRMAP_NET_HTTP_REQUESTER_H_
