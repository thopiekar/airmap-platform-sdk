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
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QThread>
#include <QCoreApplication>

#include <airmap/context.h>

namespace airmap {
namespace qt {

  class QtMainThreadScheduler : public QObject,
                                public airmap::Context::Scheduler,
                                public std::enable_shared_from_this<QtMainThreadScheduler>
  {
    struct Event : public QEvent {

        static Type registered_type() {
          static const Type rt = static_cast<Type>(registerEventType());
          return rt;
        }

        explicit Event(const std::function<void()>& task)
        : QEvent{registered_type()},
          task_(task)
        {}

        std::function<void()> task_;
    };

    public:
      void schedule(const std::function<void()>& task) override {
        QCoreApplication::postEvent(this, new Event{[task]() { task(); }});
      }

      bool event(QEvent* event) {
        assert(QCoreApplication::instance());
        assert(QThread::currentThread() == QCoreApplication::instance()->thread());

        if (event->type() == Event::registered_type()) {
          event->accept();

          if (auto e = dynamic_cast<Event*>(event)) {
            e->task_();
          }
          return true;
        }
        return false;
      }
  };


}}

#endif // SCHEDULER_H
