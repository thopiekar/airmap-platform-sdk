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
#include <airmap/codec.h>
#include <airmap/jsend.h>
#include <airmap/net/http/middleware.h>
#include <airmap/optional.h>
#include <airmap/paths.h>
#include <airmap/rest/authenticator.h>
#include <airmap/token.h>
#include <airmap/util/fmt.h>

#include <nlohmann/json.hpp>

namespace fmt = airmap::util::fmt;
using json    = nlohmann::json;

constexpr const char* component{"authenticator"};

std::string airmap::rest::Authenticator::default_route_for_version(Client::Version version) {
  static constexpr const char* pattern{"/auth/%s"};

  switch (version) {
    case airmap::Client::Version::production:
      return fmt::sprintf(pattern, "v1");
    case airmap::Client::Version::staging:
      return fmt::sprintf(pattern, "stage");
  }

  throw std::logic_error{"should not reach here"};
}

airmap::rest::Authenticator::Authenticator(const std::shared_ptr<net::http::Requester>& airmap_requester,
                                           const std::shared_ptr<net::http::Requester>& sso_requester)
    : airmap_requester_{airmap_requester}, sso_requester_{sso_requester} {
}

void airmap::rest::Authenticator::authenticate_with_password(const AuthenticateWithPassword::Params& params,
                                                             const AuthenticateWithPassword::Callback& cb) {
  std::unordered_map<std::string, std::string> headers;

  json j;
  j = params;

  sso_requester_->post("/oauth/ro", std::move(headers), j.dump(), [cb, this](const auto& result) {
    if (result) {
      const auto& response = result.value();

      switch (response.classify()) {
        case net::http::Response::Classification::informational:
        case net::http::Response::Classification::redirection:
          break;
        case net::http::Response::Classification::success:
        case net::http::Response::Classification::server_error: {
          auto outcome = jsend::parse_to_outcome<Token::OAuth>(result.value().body);
          jwt_string_  = outcome.value().id;
          cb(outcome);
          break;
        }
        case net::http::Response::Classification::client_error:
          try {
            auto j = nlohmann::json::parse(result.value().body);
            if (j.count("error_description") > 0) {
              cb(Outcome<Token::OAuth, Error>{Error{j["error_description"].template get<std::string>()}});
            } else {
              cb(Outcome<Token::OAuth, Error>{Error{"generic client error"}});
            }
          } catch (...) {
            cb(Outcome<Token::OAuth, Error>{Error{"generic client error"}});
          }
          break;
        default:
          cb(Outcome<Token::OAuth, Error>{
              Error{"networking error"}
                  .value(Error::Value{std::string{"status-code"}},
                         Error::Value{static_cast<std::int64_t>(result.value().status)})
                  .value(Error::Value{std::string{"body"}}, Error::Value{result.value().body})});
          break;
      }
    } else {
      cb(Outcome<Token::OAuth, Error>{result.error()});
    }
  });
}

void airmap::rest::Authenticator::authenticate_anonymously(const AuthenticateAnonymously::Params& params,
                                                           const AuthenticateAnonymously::Callback& cb) {
  std::unordered_map<std::string, std::string> headers;

  json j;
  j = params;

  airmap_requester_->post("/anonymous/token", std::move(headers), j.dump(),
                          net::http::jsend_parsing_request_callback<Token::Anonymous>(cb));
}

void airmap::rest::Authenticator::renew_authentication(const RenewAuthentication::Params& params,
                                                       const RenewAuthentication::Callback& cb) {
  std::unordered_map<std::string, std::string> headers;

  json j;
  j = params;

  sso_requester_->post("/delegation", std::move(headers), j.dump(),
                       net::http::jsend_parsing_request_callback<Token::Refreshed>(cb));
}

airmap::Optional<std::string> airmap::rest::Authenticator::jwt_string() {
  if (!jwt_string_) {
    auto token = read_token_from_file_();
    if (token) {
      if (token.type() == Token::Type::oauth) {
        if (token.oauth().refresh.empty()) {
          log_.errorf(component, "token file does not hold renewable token");
        } else {
          renew_authentication(config.credentials, token, std::ref(ctxt));
        }
      } else if (token.type() == Token::Type::refreshed) {
        if (!token.refreshed().original_token) {
          log_.errorf(component, "token file does not hold renewable token");
        } else {
          renew_authentication(config.credentials, token, std::ref(ctxt));
        }
      } else {
        log_.errorf(component, "token file does not hold renewable token");
      }
    } else {
      log_.errorf(component, "failed to open token file for reading");
    }
  } else {
    request_authentication(config.credentials, std::ref(ctxt));
  }

  return jwt_string_;
}

airmap::Optional<airmap::Token> airmap::rest::Authenticator::read_token_from_file_() {
  airmap::Optional<airmap::Token> maybe_token;
  std::ifstream in_token{airmap::paths::token_file(Client::Version::production).string()};
  if (!in_token) {
    log_.errorf(component, "failed to open cached token file for reading");
  } else {
    maybe_token = Token::load_from_json(in_token);
  }

  return maybe_token;
}

bool write_token_to_file_(Token token) {
  auto tfn = token_file_.get().string();
  if (std::ofstream token_file{tfn}) {
    nlohmann::json j = token;
    token_file << j.dump(2);
    log_.infof(component, "successfully persisted login token to %s", tfn);
  } else {
    log_.errorf(component, "failed to persist login token to %s", tfn);
  }
}
