/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_NFS_MAID_NODE_DISPATCHER_H_
#define MAIDSAFE_NFS_MAID_NODE_DISPATCHER_H_

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/timer.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

class OwnerDirectory;
class GroupDirectory;
class WorldDirectory;

namespace nfs {

class MaidNodeDispatcher {
 public:
  MaidNodeDispatcher(routing::Routing& routing, const passport::Maid& signing_fob);

  template<typename Data>
  void SendGetRequest(routing::TaskId task_id, const typename Data::Name& data_name);

  template<typename Data>
  void SendPutRequest(const Data& data, const passport::Pmid::Name& pmid_node_hint);

  template<typename Data>
  void SendDeleteRequest(const typename Data::Name& data_name);

  template<typename Data>
  void SendGetVersionsRequest(routing::TaskId task_id, const typename Data::Name& data_name);

  template<typename Data>
  void SendGetBranchRequest(routing::TaskId task_id,
                            const typename Data::Name& data_name,
                            const StructuredDataVersions::VersionName& branch_tip);

  template<typename Data>
  void SendPutVersionRequest(routing::TaskId task_id,
                             const typename Data::Name& data_name,
                             const StructuredDataVersions::VersionName& old_version_name,
                             const StructuredDataVersions::VersionName& new_version_name);

  template<typename Data>
  void SendDeleteBranchUntilForkRequest(const typename Data::Name& data_name,
                                        const StructuredDataVersions::VersionName& branch_tip);

  void SendCreateAccountRequest(routing::TaskId task_id);

  void SendRemoveAccountRequest(routing::TaskId task_id);

  void SendRegisterPmidRequest(routing::TaskId task_id, const PmidRegistration& pmid_registration);

  void SendUnregisterPmidRequest(routing::TaskId task_id,
                                 const PmidRegistration& pmid_registration);

  void SendGetPmidHealthRequest(const passport::Pmid& pmid);

 private:
  MaidNodeDispatcher();
  MaidNodeDispatcher(const MaidNodeDispatcher&);
  MaidNodeDispatcher(MaidNodeDispatcher&&);
  MaidNodeDispatcher& operator=(MaidNodeDispatcher);

  template<typename Message>
  void CheckSourcePersonaType() const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  const routing::SingleSource kThisNodeAsSender_;
  const routing::GroupId kMaidManagerReceiver_;
};



// ==================== Implementation =============================================================
template<typename Data>
void MaidNodeDispatcher::SendGetRequest(routing::TaskId task_id,
                                        const typename Data::Name& data_name) {
  typedef GetRequestFromMaidNodeToDataManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kGet :
                                                                         routing::Cacheable::kNone);
  NfsMessage nfs_message(MessageId(task_id), NfsMessage::Contents(data_name));
  NfsMessage::Receiver receiver(routing::GroupId(NodeId(data_name->string())));
  RoutingMessage routing_message(nfs_message.Serialise(), kThisNodeAsSender_, receiver, kCacheable);
  routing_.Send(routing_message);
}

template<typename Data>
void MaidNodeDispatcher::SendPutRequest(const Data& data,
                                        const passport::Pmid::Name& pmid_node_hint) {
  typedef PutRequestFromMaidNodeToMaidManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kPut :
                                                                         routing::Cacheable::kNone);
  NfsMessage::Contents contents;
  contents.data = data;
  contents.pmid_hint = pmid_node_hint.value;
  NfsMessage nfs_message(contents);
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, kMaidManagerReceiver_));
}

template<typename Data>
void MaidNodeDispatcher::SendDeleteRequest(const typename Data::Name& data_name) {
  typedef DeleteRequestFromMaidNodeToMaidManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage nfs_message(NfsMessage::Contents(data_name));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, kMaidManagerReceiver_));
}

template<typename Data>
void MaidNodeDispatcher::SendGetVersionsRequest(routing::TaskId task_id,
                                                const typename Data::Name& data_name) {
  typedef GetVersionsRequestFromMaidNodeToVersionManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage nfs_message(MessageId(task_id), NfsMessage::Contents(data_name));
  NfsMessage::Receiver receiver(routing::GroupId(NodeId(data_name->string())));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, receiver));
}

template<typename Data>
void MaidNodeDispatcher::SendGetBranchRequest(
    routing::TaskId task_id,
    const typename Data::Name& data_name,
    const StructuredDataVersions::VersionName& branch_tip) {
  typedef GetBranchRequestFromMaidNodeToVersionManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage::Contents contents;
  contents.data_name = DataName(data_name);
  contents.version_name = branch_tip;
  NfsMessage nfs_message(MessageId(task_id), contents);
  NfsMessage::Receiver receiver(routing::GroupId(NodeId(data_name->string())));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, receiver));
}

template<typename Data>
void MaidNodeDispatcher::SendPutVersionRequest(
    routing::TaskId task_id,
    const typename Data::Name& data_name,
    const StructuredDataVersions::VersionName& old_version_name,
    const StructuredDataVersions::VersionName& new_version_name) {
  typedef PutVersionRequestFromMaidNodeToMaidManager NfsMessage;
  static_assert(NfsMessage::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage::Contents contents;
  contents.data_name = DataName(data_name);
  contents.old_version_name = old_version_name;
  contents.new_version_name = new_version_name;
  NfsMessage nfs_message(MessageId(task_id), contents);
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, kMaidManagerReceiver_));
}

template<typename Data>
void MaidNodeDispatcher::SendDeleteBranchUntilForkRequest(
    const typename Data::Name& data_name,
    const StructuredDataVersions::VersionName& branch_tip) {
  typedef DeleteBranchUntilForkRequestFromMaidNodeToMaidManager NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;

  NfsMessage::Contents contents;
  contents.data_name = DataName(data_name);
  contents.version_name = branch_tip;
  NfsMessage nfs_message(contents);
  routing_.Send(RoutingMessage(nfs_message.Serialise(), kThisNodeAsSender_, kMaidManagerReceiver_));
}

template<typename Message>
void MaidNodeDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == Persona::kMaidNode,
                "The source Persona must be kMaidNode.");
}

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_MAID_NODE_DISPATCHER_H_
