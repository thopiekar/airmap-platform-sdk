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
#ifndef AIRMAP_QT_LOGGER_H_
#define AIRMAP_QT_LOGGER_H_

#include <airmap/logger.h>
#include <airmap/visibility.h>
#include "scheduler.h"

#include <QLoggingCategory>

#include <memory>

namespace airmap {
namespace qt {

/// Logger is an airmap::Logger implementation that uses to
/// Qt's logging facilities.
class AIRMAP_EXPORT Logger : public airmap::Logger {
 public:
  /// logging_category returns a QLoggingCategory instance
  /// that enables calling code to fine-tune logging behavior of a Logger instance.
  QLoggingCategory& logging_category() {
    static QLoggingCategory lc{"airmap"};
    return lc;
  }

  /// Logger initializes a new instance.
  Logger() = default;
  /// ~Logger cleans up all resources held by a Logger instance.
  ~Logger() = default;

  // From airmap::Logger
  void log(Severity severity, const char* message, const char*) override {
    switch (severity) {
      case Severity::debug:
        qCDebug(logging_category(), "%s", message);
        break;
      case Severity::info:
        qCInfo(logging_category(), "%s", message);
        break;
      case Severity::error:
        qCWarning(logging_category(), "%s", message);
        break;
      default:
        break;
    }
  }

  bool should_log(Severity severity, const char*, const char*) override {
    switch (severity) {
      case Severity::debug:
        return logging_category().isDebugEnabled();
      case Severity::info:
        return logging_category().isInfoEnabled();
      case Severity::error:
        return logging_category().isWarningEnabled();
      default:
        break;
    }

    return true;
  }
};

/// DispatchingLogger is an airmap::Logger implementation that dispatches to Qt's main
/// event loop for logger invocation
class AIRMAP_EXPORT DispatchingLogger : public airmap::Logger {
 public:
  /// DispatchingLogger initializes a new instance with 'underlying'.
  DispatchingLogger(const std::shared_ptr<airmap::Logger>& underlying)
  :underlying_(underlying)
  {}

  /// ~DispatchingLogging cleans up all resources held a DispatchingLogger instance.
  ~DispatchingLogger() = default;

  // From airmap::Logger
  void log(Severity severity, const char* message, const char* component) override {
    auto underlying = underlying_;
    std::string cmessage{message};
    std::string ccomponent{component};
    scheduler_.schedule([severity, cmessage, ccomponent, underlying]() {
      underlying->log(severity, cmessage.c_str(), ccomponent.c_str());
    });
  }

  bool should_log(Severity, const char*, const char*) override {
    // We have to accept all incoming log messages and postpone
    // the actual evaluation of should_log in the context of next until we
    // run on the correct thread.
    return true;
  }

 private:
  QtMainThreadScheduler scheduler_;
  std::shared_ptr<airmap::Logger> underlying_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_LOGGER_H_
