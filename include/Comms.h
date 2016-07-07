#ifndef SULLY_COMMS_H
#define SULLY_COMMS_H

#include "Sul.h"
#include <map>
#include <string>
#include <vector>
#include <exception>
#include <windows.h>
#include <functional>
#include <ctime>
#include <cmath>

namespace Sul {
    namespace Comms {
        class MessageBase;
        class NodeBase;
        class ServerBase;
        class LocalServer;
        class RemoteServer;

        void SetDLLPath(std::string);
        std::string GetDLLPath();
        void SetMessageUIDLength(unsigned int);
        unsigned int GetMessageUIDLength();

        class Base {
            friend void SetDLLPath(std::string);
            friend std::string GetDLLPath();

        protected:
            static DynamicLibrary DLL;
            class CallDLL {
            public:
                //NodeBase
                static HANDLE (*createNode)(const char*); //Name
                static void (*send)(const char*, const char*); //msg, dest
                static bool (*mailslotExists)(const char*); //Path
                static unsigned int (*countNewMessages)(HANDLE); //Mailslot handle

                //MessageBase
                static const char* (*generateUID)(unsigned int); //Length
                static const char* (*messageEncode)(const char*); //Message
                static const char* (*messageDecode)(const char*); //Message

                //LocalNode
                static const char* (*getNextMessage)(HANDLE); //MailSlot Handle
            };

            template <class Type>
            void LoadProc(Type& ptr, std::string name) {
                ptr = (Type)DLL.getProcAddress(name);
            }

        public:
            Base() {
                if (!DLL.isDLLLoaded()) {
                    DLL.loadDLL();

                    LoadProc(CallDLL::createNode, "SUL_createNode");
                    LoadProc(CallDLL::send, "SUL_send");
                    LoadProc(CallDLL::mailslotExists, "SUL_mailslotExists");
                    LoadProc(CallDLL::countNewMessages, "SUL_countNewMessages");
                    LoadProc(CallDLL::generateUID, "SUL_generateUID");
                    LoadProc(CallDLL::getNextMessage, "SUL_getNextMessage");
                    LoadProc(CallDLL::messageEncode, "SUL_messageEncode");
                    LoadProc(CallDLL::messageDecode, "SUL_messageDecode");
                }
            }
        };
        DynamicLibrary Base::DLL = DynamicLibrary("SComms.dll");

        //NodeBase
        HANDLE (*Base::CallDLL::createNode)(const char*) = nullptr; //Name
        void (*Base::CallDLL::send)(const char*, const char*) = nullptr; //msg, dest
        bool (*Base::CallDLL::mailslotExists)(const char*) = nullptr; //Path
        unsigned int (*Base::CallDLL::countNewMessages)(HANDLE) = nullptr; //Mailslot handle
        const char* (*generateUID)(unsigned int) = nullptr;
        const char* (*getNextMessage)(HANDLE) = nullptr;

        //MessageBase
        const char* (*Base::CallDLL::generateUID)(unsigned int) = nullptr; //Length
        const char* (*Base::CallDLL::messageEncode)(const char*) = nullptr; //Message
        const char* (*Base::CallDLL::messageDecode)(const char*) = nullptr; //Message

        //LocalNode
        const char* (*Base::CallDLL::getNextMessage)(HANDLE) = nullptr; //MailSlot Handle

        void SetDLLPath(std::string path) {
            Base::DLL.setDLLPath(path);
        }
        std::string GetDLLPath() {
            return Base::DLL.getDLLPath();
        }

        class NodeBase: Base {
            friend class MessageBase;
            friend class ServerBase;
            friend class LocalServer;
            friend class RemoteServer;

            std::string _client_id;
            HANDLE _slot_handle = nullptr;
            std::vector<MessageBase*> _msg_links;

        protected:
            virtual void send(MessageBase& msg, std::string mailslot_prefix);
            virtual void send(MessageBase& msg) = 0;
            void onDeletedMessage(MessageBase* msg) {
                for (int i = 0; i < _msg_links.size(); ++i) {
                    if (_msg_links[i] == msg) {
                        _msg_links.erase(_msg_links.begin() + i);
                        return;
                    }
                }

                throw std::runtime_error("Sul::Comms::NodeBase::onDeletedMessage - the given pointer was not part of the message list");
            }
            void addLink(MessageBase* link) {
                _msg_links.push_back(link);
            }
            void changeLink(MessageBase* prev_msg, MessageBase* new_msg) {
                for (int i = 0; i < _msg_links.size(); ++i) {
                    if (_msg_links[i] == prev_msg) {
                        _msg_links[i] = new_msg;
                        return;
                    }
                }

                throw std::runtime_error("Sul::Comms::NodeBase::changeLink - the given pointer to be replaced was not part of the message list");
            }
            const char* baseGetNextMessage() {
                return CallDLL::getNextMessage(this->_slot_handle);
            }

            NodeBase(std::string& clientID) {
                if (NodeBase::Exists(clientID)) {
                    throw std::runtime_error("NodeBase - Cannot create node with the ID \"" + clientID + "\" as it already exists");
                }

                _slot_handle = CallDLL::createNode(("\\\\.\\mailslot\\" + Prefix + clientID).c_str());
                _client_id = clientID;
            }

            NodeBase(std::string&& clientID) {
                if (NodeBase::Exists(clientID)) {
                    throw std::runtime_error("NodeBase - Cannot create node with the ID \"" + clientID + "\" as it already exists");
                }

                _slot_handle = CallDLL::createNode((Prefix + clientID).c_str());
                _client_id = clientID;
            }

            //We don't want an accessible copy constructor, as there should
            //only be one node with a given clientID
            NodeBase(NodeBase&) {
                throw std::runtime_error("NodeBase(NodeBase&) is not implemented");
            }
            //Nor an assignment
            NodeBase& operator=(NodeBase&) {
                throw std::runtime_error("NodeBase& operator=(NodeBase&) is not implemented");
            }

        public:
            NodeBase(NodeBase&& node) {
                _client_id = node._client_id;
                _msg_links = node._msg_links;

                node._msg_links.erase(node._msg_links.begin(), node._msg_links.end() - 1);

                _slot_handle = node._slot_handle;
                node._slot_handle = nullptr;
            }
            virtual ~NodeBase();
            NodeBase& operator=(NodeBase&& node) {
                _client_id = node._client_id;
                _msg_links = node._msg_links;

                node._msg_links.erase(node._msg_links.begin(), node._msg_links.end() - 1);

                _slot_handle = node._slot_handle;
                node._slot_handle = nullptr;

                return *this;
            }

            std::string getCliendID() {
                return _client_id;
            }
            unsigned int numNewMessages() {
                return CallDLL::countNewMessages(_slot_handle);
            }
            bool hasNewMessages() {
                return numNewMessages() > 0;
            }

            static bool Exists(std::string path) {
                return CallDLL::mailslotExists(("\\\\.\\mailslot\\" + Prefix + path).c_str());
            }

            static std::string Prefix;
        };
        class MessageBase: Base {
            friend class NodeBase;
            friend void SetMessageUIDLength(unsigned int);
            friend unsigned int GetMessageUIDLength();
            std::map<std::string, std::string> _message_map;

        protected:
            MessageBase() {
                _message_map["uid"] = CallDLL::generateUID(UIDLength);
            }
            MessageBase(std::string& message): MessageBase() {
                std::vector<std::string> lines;

                //Ensure the first and last character is not an ampersand or whitespace, otherwise the below algorithm will error
                while (message[0] == '&' || message[0] == ' ' || message[0] == '\t' || message[0] == '\n') {
                    message.erase(message.begin());
                }
                while (message[message.length() - 1] == '&' || message[message.length() - 1] == ' ' || message[message.length() - 1] == '\t' || message[message.length() - 1] == '\n') {
                    message.erase(message.end() - 1);
                }

                //Separate the string into lines based on the '&' character
                std::string line;
                unsigned i = 0, j = 0;
                bool found = false;
                for (; j < message.length(); ++j) { //Iterate over the string
                    if (message[j] == '&') { //Finding all ampersnads
                        found = true; //Used for lines.push_back condition
                        if (message[j - 1] == '|') { //Check if it has been escaped
                            continue;
                        }

                        if (i > 0) { //If it's not the first result
                            line = message.substr(i + 1, j - i - 1);
                        } else {
                            line = message.substr(i, j - i);
                        }

                        i = j;

                        lines.push_back(line);
                    }
                }
                if (found) {
                    lines.push_back(message.substr(i + 1)); //Collect the segment after the last &
                } else {
                    lines.push_back(message); //No ampersand. Message is a singular variable
                }

                //Assign to the std::map
                std::string key, value;
                for (int k = 0; k < lines.size(); ++k) {
                    //split name and value
                    key = lines[k].substr(0, lines[k].find('='));
                    value = lines[k].substr(lines[k].find('=') + 1);

                    //assign to the std::map
                    _message_map[CallDLL::messageDecode(key.c_str())] = CallDLL::messageDecode(value.c_str());
                }
            }
            MessageBase(std::map<std::string, std::string>& map) {
                _message_map = map;
            }
            MessageBase(std::map<std::string, std::string>&& map) {
                _message_map = map;
            }
            MessageBase(std::string& message, NodeBase* link): MessageBase(message) {
                this->linkWithNode(link);
                link->addLink(this);
            }
            virtual void onDeletedNode() {
                _node_link = nullptr;
            }
            virtual void linkWithNode(NodeBase* link) {
                _node_link = link;
            }

            NodeBase* _node_link = nullptr;

        public:
            MessageBase(MessageBase const& msg) {
                _node_link = msg._node_link;
                _node_link->addLink(this);

                _message_map = msg._message_map;
            }
            MessageBase(MessageBase&& msg) {
                _node_link = msg._node_link;
                msg._node_link = nullptr;
                _node_link->changeLink(&msg, this);

                _message_map = msg._message_map;
            }
            virtual ~MessageBase() {
                if (_node_link) {
                    _node_link->onDeletedMessage(this);
                    _node_link = nullptr;
                }
            }

            MessageBase& operator=(MessageBase& msg) {
                //Stay on the same node but adopt 'msg._message_map'
                _message_map = msg._message_map;
                return *this;
            }
            MessageBase& operator=(MessageBase&& msg) {
                //Stay on the same node but adopt 'msg._message_map'
                _message_map = msg._message_map;
                return *this;
            }
            std::string& operator[](std::string& key) {
                return _message_map[key];
            }
            std::string& operator[](std::string&& key) {
                return _message_map[key];
            }

            virtual void send() {
                if (get("target") == "") {
                    throw std::runtime_error("Message::send - must specify a target to send a message");
                }

                send(get("target"));
            }
            virtual void send(std::string dest) {
                if (!_node_link) {
                    throw std::runtime_error("Sul::Comms::MessageBase::sendTo - The node link no longer exists.");
                }

                (*this)["target"] = dest;

                //Send to the destination mailslot by calling send() on _node_link;
                _node_link->send(*this);
            }
            virtual void reply(MessageBase msg) {
                msg["reply-to"] = _message_map["uid"];
                msg["type"] = "reply";
                msg["target"] = _message_map["sender"];
                _node_link->send(msg);
            }
            virtual void reply(std::string msg) {
                reply(MessageBase(msg));
            }
            std::string get(std::string& key) const {
                if (_message_map.find(key) == _message_map.end()) {
                    return "";
                }

                return _message_map.at(key);
            }
            std::string get(std::string&& key) const {
                if (_message_map.find(key) == _message_map.end()) {
                    return "";
                }

                return _message_map.at(key);
            }
            std::map<std::string, std::string>& getMessageMap() {
                return _message_map;
            };
            std::string getMessage() {
                //Explode the map into a string
                std::string message = "";
                for (auto i = _message_map.begin(), e = _message_map.end(); i != e; i++) {
                    std::string first = CallDLL::messageEncode(i->first.c_str());
                    std::string second = CallDLL::messageEncode(i->second.c_str());
                    message += first + "=" + second;

                    if (++i != e) {
                        message += '&';
                    }
                    i--;
                }

                return message;
            }
            void setMessageMap(std::map<std::string, std::string> map) {
                _message_map = map;
            }

            static unsigned int UIDLength;
        };

        void NodeBase::send(MessageBase& msg, std::string mailslot_prefix) {
            if (msg.getMessageMap().find("target") == msg.getMessageMap().end()) {
                throw std::runtime_error("NodeBase::send - The message cannot be sent without a target");
            }

            msg["sender"] = _client_id;
            auto dest = mailslot_prefix + msg["target"];
            CallDLL::send(msg.getMessage().c_str(), dest.c_str());
        }
        NodeBase::~NodeBase() {
            for (int i = 0; i < _msg_links.size(); ++i) {
                _msg_links[i]->onDeletedNode();
            }
        }

        void SetMessageUIDLength(unsigned int length) {
            MessageBase::UIDLength = length;
        }
        unsigned int GetMessageUIDLength() {
            return MessageBase::UIDLength;
        }

        class LocalNode: public NodeBase {
        public:
            LocalNode(std::string& clientID): NodeBase(clientID) { }
            LocalNode(std::string&& clientID): NodeBase(clientID) { }

            class Message: public MessageBase {
                friend class LocalNode;

            protected:
                Message(): MessageBase() {}
                Message(std::string& str): MessageBase(str) {}
                Message(std::string&& str): MessageBase(str) {}
                Message(std::map<std::string, std::string>& map): MessageBase(map) {}
                Message(std::map<std::string, std::string>&& map): MessageBase(map) {}

                virtual void linkWithNode(NodeBase* node) {
                    MessageBase::linkWithNode(node);
                }
            };

            Message getNextMessage() {
                auto ret = Message(baseGetNextMessage());
                ret.linkWithNode(this);
                this->addLink(&ret);
                return ret;
            }
            Message waitForMessage() {
                return waitForMessage(0); //No timeout
            }
            Message waitForMessage(std::size_t timeout) {
                std::size_t ms_count = 0;
                while (!hasNewMessages()) {
                    Sleep(20);
                    ms_count += 20;

                    if (timeout > 0 && ms_count > timeout) {
                        throw std::runtime_error("waitForMessage - Operation timed out.");
                    }
                }

                return getNextMessage();
            }

            virtual void send(MessageBase& msg) {
                NodeBase::send(msg, "\\\\.\\mailslot\\" + Prefix);
            }

            //Initializer can be l or r value refs to string or map
            template <class Init>
            Message createMessage(Init initializer) {
                auto msg = Message(initializer);
                msg.linkWithNode(this);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "local";
                return msg;
            }
            Message createMessage() {
                auto msg = Message();
                msg.linkWithNode(this);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "local";
                return msg;
            }
        };
        class RemoteNode: public NodeBase {
            std::string _remote_target = "*";

        public:
            RemoteNode(std::string& clientID): NodeBase(clientID) { }
            RemoteNode(std::string&& clientID): NodeBase(clientID) { }

            class Message: public MessageBase {
                friend class RemoteNode;

            protected:
                Message(): MessageBase() {}
                Message(std::string& str): MessageBase(str) {}
                Message(std::string&& str): MessageBase(str) {}
                Message(std::map<std::string, std::string>& map): MessageBase(map) {}
                Message(std::map<std::string, std::string>&& map): MessageBase(map) {}

                virtual void linkWithNode(NodeBase* node) {
                    MessageBase::linkWithNode(node);
                }
            };

            Message getNextMessage() {
                auto ret = Message(baseGetNextMessage());
                ret.linkWithNode(this);
                this->addLink(&ret);
                return ret;
            }
            Message waitForMessage() {
                return waitForMessage(0); //No timeout
            }
            Message waitForMessage(std::size_t timeout) {
                std::size_t ms_count = 0;
                while (!hasNewMessages()) {
                    Sleep(20);
                    ms_count += 20;

                    if (timeout > 0 && ms_count > timeout) {
                        throw std::runtime_error("waitForMessage - Operation timed out.");
                    }
                }

                return getNextMessage();
            }

            //Initializer can be l or r value refs to string or map
            template <class Init>
            Message createMessage(Init initializer) {
                auto msg = Message(initializer);
                msg.linkWithNode(this);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "remote";
                return msg;
            }
            Message createMessage() {
                auto msg = Message();
                msg.linkWithNode(this);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "remote";
                return msg;
            }

            //_remote_target defaults to wildcard, but can be set to specific
            //PC names or IPs
            void setRemoteTarget(std::string target) {
                _remote_target = target;
            }
            std::string getRemoteTarget() {
                return _remote_target;
            }

            virtual void send(MessageBase& msg) {
                NodeBase::send(msg, "\\\\*\\mailslot\\" + Prefix);
            }
        };

        std::string NodeBase::Prefix = "";
        unsigned int MessageBase::UIDLength = 3;

        class ServerBase {
        public:
            class Message: public MessageBase {
                friend class ServerBase;
                friend class LocalServer;
                friend class RemoteServer;

            protected:
                Message(): MessageBase() {}
                Message(std::string& str): MessageBase(str) {}
                Message(std::string&& str): MessageBase(str) {}
                Message(std::map<std::string, std::string>& map): MessageBase(map) {}
                Message(std::map<std::string, std::string>&& map): MessageBase(map) {}
                Message(std::string& str, NodeBase* nlink, ServerBase* slink): MessageBase(str, nlink) {
                    linkWithServer(slink);
                    slink->addLink(this);
                }

                virtual void linkWithNode(NodeBase* node) {
                    MessageBase::linkWithNode(node);
                }

                virtual void linkWithServer(ServerBase* server) {
                    _server_link = server;
                }

                virtual void onDeletedNode() {
                    _server_link = nullptr;
                    MessageBase::onDeletedNode();
                }

                ServerBase* _server_link = nullptr;

            public:
                Message(Message const& msg): MessageBase(msg) {
                    _server_link = msg._server_link;
                    _server_link->addLink(this);
                }
                Message(Message&& msg): MessageBase(msg) {
                    _server_link = msg._server_link;
                    msg._server_link = nullptr;
                    _server_link->changeLink(&msg, this);
                }
                virtual ~Message() {
                    if (_server_link) {
                        _server_link->removeLink(this);
                        _server_link = nullptr;
                    }
                }

                Message& operator=(Message const& msg) {
                    this->setMessageMap((*const_cast<Message*>(&msg)).getMessageMap());
                    return *this;
                }

                void onReply(std::function<void(MessageBase&)> fn) {
                    _server_link->setReceiveEvent(Event([this](MessageBase const& msg) -> bool {
                        //Capture any replies directed at this message, that are NOT errors
                        return msg.get("type") == "reply" && msg.get("reply-to") == (*this)["uid"] && msg.get("status") != "error";
                    }), fn);
                }

                void onError(std::function<void(MessageBase&)> fn) {
                    _server_link->setReceiveEvent(Event([this](MessageBase const& msg) -> bool {
                        //Capture any replies directed at this message, that ARE errors
                        return msg.get("type") == "reply" && msg.get("reply-to") == (*this)["uid"] && msg.get("status") == "error";
                    }), fn);
                }

                virtual void send() override {
                    if (get("target") == "") {
                        throw std::runtime_error("Message::send - must specify a target to send a message");
                    }

                    send(get("target"));
                }
                virtual void send(std::string dest) override {
                    (*this)["target"] = dest;

                    _server_link->processOutgoingMessage(*this);

                    MessageBase::send(dest);
                }
                virtual void reply(std::string msg) {
                    reply(Message(msg, _node_link, _server_link));
                }
                virtual void reply(Message msg) {
                    msg["reply-to"] = get("uid");
                    msg["type"] = "reply";
                    msg["target"] = get("sender");

                    msg.send();
                }
                virtual void replyError(Message msg) {
                    msg["status"] = "error";
                    reply(msg);
                }
                virtual void replyError(std::string msg) {
                    Message resp;
                    resp.linkWithServer(this->_server_link);
                    resp.linkWithNode(this->_node_link);
                    this->_server_link->addLink(&resp);
                    this->_node_link->addLink(&resp);

                    resp["status"] = "error";
                    resp["msg"] = msg;

                    reply(resp);
                }
            };
            friend class Message;

            class Event {
                std::function<bool(MessageBase const&)> _condition;
                std::function<void(MessageBase&)> _handler;

            public:
                Event() {
                    _condition = [](MessageBase const&) -> bool {
                        return true;
                    };
                }
                //Event(bool(*cd)(MessageBase const&)): _condition(cd) {}
                Event(std::function<bool(MessageBase const&)> cd): _condition(cd) {}
                Event(std::function<bool(MessageBase const&)> cd, std::function<bool(MessageBase&)> hn): _condition(cd), _handler(hn) {}

                void setCondition(std::function<bool(MessageBase const&)> fn) {
                    _condition = fn;
                }
                void setHandler(std::function<void(MessageBase&)> fn) {
                    _handler = fn;
                }

                bool operator()(MessageBase& msg) {
                    if (!_handler) {
                        return _condition(msg);
                    }

                    if (_condition(msg)) {
                        _handler(msg);
                        return true;
                    }

                    return false;
                }
            };

        private:
            std::function<void(std::function<void()>)> _workerAllocProc;
            std::function<int()> _workerCheckProc;
            bool _workerAllocImplemented = false;
            bool _workerCheckImplemented = false;
            std::map<std::size_t, Event> _receive_event_handlers;
            std::map<std::size_t, Event> _send_event_handlers;
            std::size_t _receive_evt_count = 0;
            std::size_t _send_evt_count = 0;
            std::size_t _default_ping_event;
            std::size_t _default_forward_event;
            std::vector<Message> _message_queue;
            bool _ping_overwritten = false;
            bool _forward_overwritten = false;
            bool _listening = false;

        protected:
            NodeBase* _node;

            ServerBase() {
                _workerAllocProc = [&](std::function<void()> fn) {
                    throw std::runtime_error("Worker allocation procedure is not implemented");
                };

                _workerCheckProc = []() -> int {
                    throw std::runtime_error("Worker check procedure is not implemented");
                };

                //Define default event handlers
                _default_ping_event = setReceiveEvent(Event([](MessageBase const& msg) -> bool {
                    return msg.get("type") == "ping";
                }), [](MessageBase& msg) {
                    msg.reply("response=ok");
                });

                _default_forward_event = setReceiveEvent(Event([](MessageBase const& msg) -> bool {
                    return msg.get("action") == "forward";
                }), [](MessageBase& msg) {
                    auto fw_target = msg["forward-to"];
                    msg.getMessageMap().erase("action");
                    msg.getMessageMap().erase("forward-to");

                    msg["forwarded-for"] = msg["sender"];
                    msg.send(fw_target);
                });
            }

            ServerBase(ServerBase&) {
                throw std::runtime_error("ServerBase(ServerBase&) is not implemented");
            }
            ServerBase& operator=(ServerBase&) {
                throw std::runtime_error("ServerBase& operator=(ServerBase&) is not implemented");
            }

            void processIncomingMessage(MessageBase& msg) {
                int process_count = 0;
                for (auto it = _receive_event_handlers.begin(); it != _receive_event_handlers.end(); it++) {
                    process_count++;
                    auto fn = [it, &msg, &process_count](){
                        it->second(msg);
                        process_count--;
                    };

                    //If there are no threads available, or no defined way to allocate work, do the work on this thread
                    if (_workerAllocImplemented && _workerCheckProc() > 1) {
                        _workerAllocProc(fn);
                    } else {
                        fn();
                    }
                }

                //Block the thread until all processes have finished
                while (process_count > 0) {
                    Sleep(10);
                }
            }
            void processOutgoingMessage(MessageBase& msg) {
                int process_count = 0;
                for (auto it = _send_event_handlers.begin(); it != _send_event_handlers.end(); it++) {
                    process_count++;
                    auto fn = [it, &msg, &process_count](){
                        it->second(msg);
                        process_count--;
                    };

                    if (_workerAllocImplemented && _workerCheckProc() > 1) {
                        _workerAllocProc(fn);
                    } else {
                        fn();
                    }
                }

                //Block the thread until all processes have finished
                while (process_count > 0) {
                    Sleep(10);
                }
            }

        public:
            ServerBase(ServerBase&& server) {
                _node = server._node;
                server._node = nullptr;
                _msg_links = server._msg_links;

                //To avoid the messages being deleted
                server._msg_links.erase(server._msg_links.begin(), server._msg_links.end() - 1);

                //Change the message links to this node
                for (int i = 0; i < _msg_links.size(); ++i) {
                    _msg_links[i]->linkWithServer(this);
                    //Don't need to link with node. Node is being transferred
                    //so it will be the same
                }
            }
            virtual ~ServerBase() {
                for (int i = 0; i < _msg_links.size(); ++i) {
                    _msg_links[i]->onDeletedNode();
                }
            }

            void setWorkerAllocationProc(std::function<void(std::function<void()>)> fn) {
                _workerAllocProc = fn;
                _workerAllocImplemented = true;
            }
            void setWorkerCheckProc(std::function<int()> fn) {
                _workerCheckProc = fn; //Should return the number of free worker threads
                _workerCheckImplemented = true;
            }
            std::size_t onPing(std::function<void(MessageBase&)> fn) {
                if (!_ping_overwritten) {
                    _ping_overwritten = true;
                    removeReceiveEvent(_default_ping_event);
                }

                return setReceiveEvent(Event([](MessageBase const& msg) -> bool {
                    return msg.get("type") == "ping";
                }), fn);
            }
            std::size_t onExternalError(std::function<void(MessageBase&)> fn) {
                return setReceiveEvent(Event([](MessageBase const& msg) -> bool {
                    return msg.get("status") == "error";
                }), fn);
            }
            std::size_t onForwardRequest(std::function<void(MessageBase&)> fn) {
                if (!_forward_overwritten) {
                    _forward_overwritten = true;
                    removeReceiveEvent(_default_forward_event);
                }

                return setReceiveEvent(Event([](MessageBase const& msg) -> bool {
                    return msg.get("action") == "forward";
                }), fn);
            }
            std::size_t onMessageReceived(std::function<void(MessageBase&)> fn) {
                return setReceiveEvent(Event([](MessageBase const& msg) {
                    return true;
                }), fn);
            }
            std::size_t onMessageSent(std::function<void(MessageBase&)> fn) {
                return setSendEvent(Event([](MessageBase const& msg) {
                    return true;
                }), fn);
            }
            std::size_t setProxy(std::string clientID) {
                return setSendEvent(Event([](MessageBase const& msg) -> bool {
                    return true;
                }), [clientID](MessageBase& msg) {
                    msg["forward-to"] = msg["target"];
                    msg["target"] = clientID;
                });
            }
            void removeProxy(std::size_t proxyID) {
                removeSendEvent(proxyID);
            }
            std::size_t setReceiveEvent(Event cd, std::function<void(MessageBase&)> hd) {
                std::size_t it = _receive_evt_count++;
                cd.setHandler(hd);
                _receive_event_handlers[it] = cd;

                return it;
            }
            std::size_t setSendEvent(Event cd, std::function<void(MessageBase&)> hd) {
                std::size_t it = _send_evt_count++;
                cd.setHandler(hd);
                _send_event_handlers[it] = cd;

                return it;
            }
            void removeReceiveEvent(std::size_t evtID) {
                _receive_event_handlers.erase(evtID);
            }
            void removeSendEvent(std::size_t evtID) {
                _send_event_handlers.erase(evtID);
            }
            Message getNextMessage() {
                return getNextMessage(false); //Do not ignore the local queue
            }
            Message getNextMessage(bool ignoreLocalQueue) {
                Message ret; //Create a new empty message
                ret.linkWithNode(_node);
                ret.linkWithServer(this);
                addLink(&ret);
                _node->addLink(&ret);

                if (!ignoreLocalQueue && _message_queue.size() > 1) {
                    ret.setMessageMap(_message_queue[0].getMessageMap());
                    _message_queue.erase(_message_queue.begin());
                } else {
                    ret.setMessageMap((Message(_node->baseGetNextMessage())).getMessageMap());
                }

                ret.linkWithNode(_node);
                ret.linkWithServer(this);
                _node->addLink(&ret);
                this->addLink(&ret);

                processIncomingMessage(ret);

                return ret;
            }
            Message waitForMessage() {
                return waitForMessage(0); //No timeout
            }
            Message waitForMessage(std::size_t timeout) {
                std::size_t ms_count = 0;
                while (!_node->hasNewMessages()) {
                    Sleep(20);
                    ms_count += 20;

                    if (timeout > 0 && ms_count > timeout) {
                        throw std::runtime_error("waitForMessage - Operation timed out.");
                    }
                }

                return getNextMessage();
            }
            std::string getCliendID() {
                return _node->getCliendID();
            }
            unsigned int numNewMessages() {
                return numNewMessages(false);
            }
            unsigned int numNewMessages(bool ignoreLocalQueue) {
                unsigned int ret = 0;
                if (!ignoreLocalQueue) {
                    ret += _message_queue.size();
                }

                ret += _node->numNewMessages();

                return ret;
            }
            bool hasNewMessages() {
                return hasNewMessages(false);
            }
            bool hasNewMessages(bool ignoreLocalQueue) {
                return numNewMessages(ignoreLocalQueue) > 0;
            }
            std::size_t ping(std::string target) {
                return ping(target, 2000); //2s timeout
            }
            std::size_t ping(std::string target, std::size_t timeout) {
                return ping(target, timeout, 10); //10ms accuracy
            }
            std::size_t ping(std::string target, std::size_t timeout, std::size_t accuracy) {
                if (accuracy == 0) {
                    throw std::runtime_error("Server::ping - Accuracy must be at least 1");
                }

                Message msgPing;
                msgPing.linkWithNode(_node);
                msgPing.linkWithServer(this);
                _node->addLink(&msgPing);
                this->addLink(&msgPing);

                msgPing["type"] = "ping";
                bool response = false;
                auto c = clock();
                clock_t duration;
                msgPing.send(target);

                std::size_t time_tally = 0;
                while (!response) {
                    //Check newest messages
                    while (hasNewMessages(true)) { //Ignoring the local queue
                        auto msg = getNextMessage(true); //Get next message, ignoring the local queue to avoid infinite loops
                        if (msg["type"] == "reply" && msg["reply-to"] == msgPing["uid"]) {
                            response = true;
                            duration = clock() - c;
                        } else {
                            _message_queue.push_back(msg);
                        }
                    }

                    Sleep(accuracy);

                    time_tally += accuracy;
                    if (timeout > 0 && time_tally >= timeout) {
                        return 0;
                    }
                }

                return static_cast<std::size_t>(round(((double)(duration) / CLOCKS_PER_SEC) * 1000)); //Get the milliseconds taken
            }
            void listen() {
                listen(20); //Default 20ms interval
            }
            void listen(std::size_t interval) {
                _listening = true;
                while (_listening) {
                    while (!_node->hasNewMessages()) {
                        Sleep(interval);
                    }

                    getNextMessage(); //Doesn't return the message.
                    //Note: When using 'listen', it's expected that the programmer defines an 'onMessageReceived' event to handle the message
                }
            }

            bool isListening() {
                return _listening;
            }
            void stopListening() {
                _listening = false;
            }

            static bool Exists(std::string path) {
                return NodeBase::Exists(path);
            }

        protected:
            std::vector<Message*> _msg_links;

            void addLink(Message* msg) {
                _msg_links.push_back(msg);
            }
            void removeLink(Message* msg) {
                for (int i = 0; i < _msg_links.size(); ++i) {
                    if (_msg_links[i] == msg) {
                        _msg_links.erase(_msg_links.begin() + i);
                    }
                }
            }
            void changeLink(Message* msg1, Message* msg2) {
                for (int i = 0; i < _msg_links.size(); ++i) {
                    if (_msg_links[i] == msg1) {
                        _msg_links[i] = msg2;
                    }
                }
            }
        };

        class LocalServer: public ServerBase {
        public:
            LocalServer(std::string clientID) {
                _node = new LocalNode(clientID);
            }
            virtual ~LocalServer() {
                delete _node;
            }

            //Initializer can be l or r value refs to string or map
            template <class Init>
            Message createMessage(Init initializer) {
                auto msg = Message(initializer);
                msg.linkWithServer(this);
                msg.linkWithNode(_node);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                _node->addLink(&msg);
                msg["scope"] = "local";
                return msg;
            }
            Message createMessage() {
                auto msg = Message();
                msg.linkWithServer(this);
                msg.linkWithNode(_node);
                this->addLink(&msg); //The link is tracked across move/copy constructors
                _node->addLink(&msg);
                msg["scope"] = "local";
                return msg;
            }
        };
        class RemoteServer: public ServerBase {
        public:
            RemoteServer(std::string clientID) {
                _node = new RemoteNode(clientID);
            }
            virtual ~RemoteServer() {
                delete _node;
            }

            //Initializer can be l or r value refs to string or map
            template <class Init>
            Message createMessage(Init initializer) {
                auto msg = Message(initializer);
                msg.linkWithNode(this->_node);
                this->_node->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "remote";
                return msg;
            }
            Message createMessage() {
                auto msg = Message();
                msg.linkWithNode(this->_node);
                this->_node->addLink(&msg); //The link is tracked across move/copy constructors
                msg["scope"] = "remote";
                return msg;
            }
        };
    }
}

#endif //SULLY_COMMS_H