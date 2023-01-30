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

#include <fstream>
#include <nlohmann/json.hpp>

namespace fmt = airmap::util::fmt;
namespace ph  = std::placeholders;
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

void airmap::rest::Authenticator::handle_result_for_authentication_with_password_(
    const Authenticator::AuthenticateWithPassword::Result& result) {
  if (result) {
    log_.infof(component, "successfully authenticated with the AirMap services");
    auto token = Token{result.value()};
    write_token_to_file_(token);
  } else {
    log_.errorf(component, "failed to authenticate with the Airmap services: %s", result.error());
  }
}

void airmap::rest::Authenticator::handle_result_for_anonymous_authentication_(
    const airmap::Authenticator::AuthenticateAnonymously::Result& result) {
  if (result) {
    log_.infof(component, "successfully authenticated with the AirMap services");
    auto token = Token{result.value()};
    write_token_to_file_(token);
  } else {
    log_.errorf(component, "failed to authenticate with the Airmap services: %s", result.error());
  }
}

void airmap::rest::Authenticator::handle_result_for_renewed_authentication_(
    const airmap::Authenticator::RenewAuthentication::Result& result, const airmap::Token& previous_token) {
  if (result) {
    log_.infof(component, "successfully authenticated with the AirMap services");
    auto t = result.value();

    if (previous_token.type() == airmap::Token::Type::oauth) {
      t.original_token = previous_token.oauth();
    } else if (previous_token.type() == airmap::Token::Type::refreshed) {
      t.original_token = previous_token.refreshed().original_token.get();
    }

    auto token = airmap::Token{t};
    write_token_to_file_(token);
  } else {
    log_.errorf(component, "failed to authenticate with the Airmap services: %s", result.error());
  }
}

void airmap::rest::Authenticator::renew_authentication_token(const Credentials& credentials, const Token& token) {
  airmap::Authenticator::RenewAuthentication::Params params;
  params.client_id = credentials.oauth.get().client_id;

  if (token.type() == Token::Type::oauth) {
    params.refresh_token = token.oauth().refresh;
  } else if (token.type() == Token::Type::refreshed) {
    params.refresh_token = token.refreshed().original_token.get().refresh;
  }

  this->renew_authentication(
      params, std::bind(&airmap::rest::Authenticator::handle_result_for_renewed_authentication_, this, ph::_1, token));
}

void airmap::rest::Authenticator::request_authentication_token(const Credentials& credentials) {
  if (credentials.oauth) {
    airmap::Authenticator::AuthenticateWithPassword::Params params;
    params.oauth = credentials.oauth.get();
    this->authenticate_with_password(
        params, std::bind(&airmap::rest::Authenticator::handle_result_for_authentication_with_password_, this, ph::_1));
  } else if (credentials.anonymous) {
    this->authenticate_anonymously(
        {credentials.anonymous.get().id},
        std::bind(&airmap::rest::Authenticator::handle_result_for_anonymous_authentication_, this, ph::_1));
  }
}

airmap::Optional<std::string> airmap::rest::Authenticator::jwt_string() {
  auto config_path = airmap::paths::config_file(Client::Version::production).string();
  std::ifstream config_file{config_path};
  if (!config_file) {
    std::string error_string = "failed to open config file " + config_path;
    throw std::runtime_error{error_string};
  }
  auto config = Client::load_configuration_from_json(config_file);

  if (!jwt_string_) {
    auto token = read_token_from_file_();
    if (token) {
      if (token.get().type() == Token::Type::oauth) {
        if (token.get().oauth().refresh.empty()) {
          log_.errorf(component, "token file does not hold renewable token");
          // we'll presume the token file is corrupt...so remove it
          remove(airmap::paths::token_file(Client::Version::production).string().c_str());
        } else {
          renew_authentication_token(config.credentials, token.get());
        }
      } else if (token.get().type() == Token::Type::refreshed) {
        if (!token.get().refreshed().original_token) {
          log_.errorf(component, "token file does not hold renewable token");
          // we'll presume the token file is corrupt...so remove it
          remove(airmap::paths::token_file(Client::Version::production).string().c_str());
        } else {
          renew_authentication_token(config.credentials, token.get());
        }
      } else {
        log_.errorf(component, "token file does not hold renewable token");
        // we'll presume the token file is corrupt...so remove it
        remove(airmap::paths::token_file(Client::Version::production).string().c_str());
      }
    } else {
      request_authentication_token(config.credentials);
    }
  }

  return jwt_string_;
}

airmap::Optional<airmap::Token> airmap::rest::Authenticator::read_token_from_file_() {
  airmap::Optional<airmap::Token> maybe_token;
  auto token_path = airmap::paths::token_file(Client::Version::production).string();
  std::ifstream in_token{token_path};
  if (!in_token) {
    log_.errorf(component, "failed to open cached token file %s for reading", token_path);
  } else {
    maybe_token = Token::load_from_json(in_token);
  }

  return maybe_token;
}

bool airmap::rest::Authenticator::write_token_to_file_(airmap::Token token) {
  auto token_path = airmap::paths::token_file(Client::Version::production).string();
  std::ofstream out_token{token_path};
  if (out_token) {
    nlohmann::json j = token;
    out_token << j.dump(2);
    log_.infof(component, "successfully persisted login token to %s", token_path);
  } else {
    log_.errorf(component, "failed to persist login token to %s", token_path);
  }
}
