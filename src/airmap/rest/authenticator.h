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
#ifndef AIRMAP_REST_AUTHENTICATOR_H_
#define AIRMAP_REST_AUTHENTICATOR_H_

#include <airmap/authenticator.h>
#include <airmap/client.h>
#include <airmap/credentials.h>
#include <airmap/util/formatting_logger.h>
#include <airmap/net/http/requester.h>
#include <airmap/net/http/jwt_provider.h>

namespace airmap {
namespace rest {

class Authenticator : public airmap::Authenticator, public airmap::net::http::JWTProvider {
 public:
  static std::string default_route_for_version(Client::Version version);

  explicit Authenticator(const std::shared_ptr<net::http::Requester>& airmap_requester,
                         const std::shared_ptr<net::http::Requester>& sso_requester);

  void authenticate_with_password(const AuthenticateWithPassword::Params& params,
                                  const AuthenticateWithPassword::Callback& cb) override;

  void authenticate_anonymously(const AuthenticateAnonymously::Params& params,
                                const AuthenticateAnonymously::Callback& cb) override;

  void renew_authentication(const RenewAuthentication::Params& params,
                            const RenewAuthentication::Callback& cb) override;
  
  void renew_authentication_token(const Credentials& credentials,
                                  const Token& token,
                                  std::function<void(Optional<std::string>)> cb = {});

  void request_authentication_token(const Credentials& credentials,
                                    std::function<void(Optional<std::string>)> cb = {});

  void perform_with_auth(std::function<void(Optional<std::string>)>) override;

 private:
  std::shared_ptr<net::http::Requester> airmap_requester_;
  std::shared_ptr<net::http::Requester> sso_requester_;
  util::FormattingLogger log_{create_null_logger()};

  /// JWT that may be utilized by other AirMap service interface classes
  Optional<std::string> jwt_string_;
  Optional<Token> read_token_from_file_();
  bool write_token_to_file_(Token token);

  void handle_result_for_authentication_with_password_(const Authenticator::AuthenticateWithPassword::Result& result,
                                                       std::function<void(Optional<std::string>)> cb);   
  void handle_result_for_anonymous_authentication_(const Authenticator::AuthenticateAnonymously::Result& result,
                                                   std::function<void(Optional<std::string>)> cb);
  void handle_result_for_renewed_authentication_(const airmap::Authenticator::RenewAuthentication::Result& result,
                                                 const airmap::Token& previous_token,
                                                 std::function<void(Optional<std::string>)> cb);
};

}  // namespace rest
}  // namespace airmap

#endif  // AIRMAP_REST_AUTHENTICATOR_H_
