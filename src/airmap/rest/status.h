// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef AIRMAP_REST_STATUS_H_
#define AIRMAP_REST_STATUS_H_

#include <airmap/client.h>
#include <airmap/net/http/requester.h>
#include <airmap/status.h>

namespace airmap {
namespace rest {

class Status : public airmap::Status {
 public:
  static std::string default_route_for_version(Client::Version version);

  explicit Status(const std::shared_ptr<net::http::Requester>& requester);

  void get_status_by_point(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;
  void get_status_by_path(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;
  void get_status_by_polygon(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) override;

  void set_auth_token(std::string token);

 private:
  std::shared_ptr<net::http::Requester> requester_;
};

}  // namespace rest
}  // namespace airmap

#endif  // AIRMAP_REST_STATUS_H_
