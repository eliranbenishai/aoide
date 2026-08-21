#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace tramp {

/// Background threads their owner can wait for.
///
/// A detached thread cannot be waited on, so an owner being destroyed has no way
/// to know whether one is still running — and a worker that outlives its owner
/// marshals a queued call into freed memory. Every worker started here is joined
/// by [stopAndJoin], which the destructor also calls, so a worker body may hold a
/// raw pointer to its owner: the owner cannot finish being destroyed while the
/// worker runs, and a call posted during the join is discarded by ~QObject.
///
/// The join is only as short as the workers' own cancellation, which is why
/// [alive] is a shared atomic rather than a QPointer: any thread may read it, and
/// a body must consult it at every loop iteration.
class WorkerCrew {
 public:
  WorkerCrew() = default;
  WorkerCrew(const WorkerCrew&) = delete;
  WorkerCrew& operator=(const WorkerCrew&) = delete;
  ~WorkerCrew() { stopAndJoin(); }

  /// False from the moment teardown begins. Capture it by value into a body.
  std::shared_ptr<std::atomic_bool> alive() const { return alive_; }

  /// Run [body] on its own thread. Ignored once teardown has begun.
  void start(std::function<void()> body) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!alive_->load()) return;  // under the lock, or the join could miss it
    reapFinished();
    auto worker = std::make_unique<Worker>();
    Worker* slot = worker.get();
    slot->thread = std::thread([slot, run = std::move(body)]() {
      run();
      slot->done.store(true);
    });
    workers_.push_back(std::move(worker));
  }

  /// Signal cancellation, then wait for every worker to return.
  void stopAndJoin() {
    alive_->store(false);
    std::vector<std::unique_ptr<Worker>> mine;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      mine.swap(workers_);
    }
    for (const auto& worker : mine) {
      if (worker->thread.joinable()) worker->thread.join();
    }
  }

 private:
  struct Worker {
    std::thread thread;
    std::atomic_bool done{false};
  };

  /// Track playback starts a decode per track, so finished threads have to be
  /// cleared as we go rather than piling up until teardown.
  void reapFinished() {
    for (auto it = workers_.begin(); it != workers_.end();) {
      if (!(*it)->done.load()) {
        ++it;
        continue;
      }
      if ((*it)->thread.joinable()) (*it)->thread.join();
      it = workers_.erase(it);
    }
  }

  std::mutex mutex_;
  std::vector<std::unique_ptr<Worker>> workers_;
  std::shared_ptr<std::atomic_bool> alive_ = std::make_shared<std::atomic_bool>(true);
};

}  // namespace tramp
