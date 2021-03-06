/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_NFS_UTILS_H_
#define MAIDSAFE_NFS_UTILS_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <system_error>
#include <vector>

#include "maidsafe/common/error.h"
#include "maidsafe/routing/parameters.h"


namespace maidsafe {

namespace nfs {

template<typename A, typename B>
bool CheckMutuallyExclusive(const A& a, const B& b) {
  return (!a != !b);
}

template<typename MessageContents>
bool IsSuccess(const MessageContents& response);

template<typename MessageContents>
std::error_code ErrorCode(const MessageContents& response);

template<typename MessageContents>
bool Equals(const MessageContents& lhs, const MessageContents& rhs) {
  return lhs == rhs;
}

// If 'responses' contains >= n successsful responses where n is 'successes_required', returns
// <iterator to nth successful reply, true> otherwise
// <iterator to most frequent failed reply, false>.  If there is more than one most frequent type,
// (e.g. 2 'no_such_element' and 2 'invalid_parameter') the iterator points to the first which
// reaches the frequency.
template<typename MessageContents>
std::pair<typename std::vector<MessageContents>::const_iterator, bool>
    GetSuccessOrMostFrequentResponse(const std::vector<MessageContents>& responses,
                                     int successes_required);

template<typename MessageContents>
class OpData {
 public:
  OpData(int successes_required, std::function<void(MessageContents)> callback);
  void HandleResponseContents(MessageContents&& response_contents);

 private:
  OpData(const OpData&);
  OpData(OpData&&);
  OpData& operator=(OpData);

  mutable std::mutex mutex_;
  int successes_required_;
  std::function<void(MessageContents)> callback_;
  std::vector<MessageContents> responses_;
  bool callback_executed_;
};



// ==================== Implementation =============================================================
template<typename MessageContents>
std::pair<typename std::vector<MessageContents>::const_iterator, bool>
    GetSuccessOrMostFrequentResponse(const std::vector<MessageContents>& responses,
                                     int successes_required) {
  auto most_frequent_itr(std::end(responses));
  int successes(0), most_frequent(0);
  typedef std::map<std::error_code, int> Count;
  Count count;
  for (auto itr(std::begin(responses)); itr != std::end(responses); ++itr) {
    int this_reply_count(++count[ErrorCode(*itr)]);
    if (IsSuccess(*itr)) {
      if (++successes >= successes_required)
        return std::make_pair(itr, true);
    } else if (this_reply_count > most_frequent) {
      most_frequent = this_reply_count;
      most_frequent_itr = itr;
    }
  }
  return std::make_pair(most_frequent_itr, false);
}

template<typename MessageContents>
OpData<MessageContents>::OpData(int successes_required,
                                std::function<void(MessageContents)> callback)
    : mutex_(),
      successes_required_(successes_required),
      callback_(callback),
      responses_(),
      callback_executed_(!callback) {
  if (!callback || successes_required <= 0)
    ThrowError(CommonErrors::invalid_parameter);
}

template<typename MessageContents>
void OpData<MessageContents>::HandleResponseContents(MessageContents&& response_contents) {
  std::function<void(MessageContents)> callback;
  std::unique_ptr<MessageContents> result_ptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_executed_)
      return;
    responses_.push_back(std::move(response_contents));
    auto result(GetSuccessOrMostFrequentResponse(responses_, successes_required_));
    // TODO(Fraser#5#): 2013-08-18 - Confirm expected count
    if (result.second || responses_.size() == routing::Parameters::node_group_size) {
      // Operation has succeeded or failed overall
      callback = callback_;
      callback_executed_ = true;
      result_ptr = std::unique_ptr<MessageContents>(new MessageContents(*result.first));
    } else {
      return;
    }
  }
  callback(*result_ptr);
}

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_UTILS_H_
