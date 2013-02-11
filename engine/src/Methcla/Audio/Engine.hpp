// Copyright 2012-2013 Samplecount S.L.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef METHCLA_AUDIO_ENGINE_HPP_INCLUDED
#define METHCLA_AUDIO_ENGINE_HPP_INCLUDED

#include <Methcla/Audio.hpp>
#include <Methcla/Audio/AudioBus.hpp>
#include <Methcla/Audio/IO/Client.hpp>
#include <Methcla/Audio/Node.hpp>
#include <Methcla/Audio/SynthDef.hpp>
#include <Methcla/Exception.hpp>
#include <Methcla/Memory/Manager.hpp>
#include <Methcla/Utility/MessageQueue.hpp>

#include <boost/filesystem.hpp>
#include <boost/utility.hpp>

#include <cstddef>
#include <vector>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"

#define METHCLA_ENGINE_PREFIX "http://methc.la/engine#"

namespace Methcla { namespace Audio
{
    // class ControlBus : boost::noncopyable
    // {
    // public:
    //     typedef sample_t ValueType;
    // 
    // private:
    //     BusId       m_id;
    //     Epoch       m_epoch;
    //     ValueType   m_data;
    // };

    using std::size_t;

    struct EngineException : virtual Methcla::Exception { };
    struct InvalidNodeId : virtual EngineException { };
    struct DuplicateNodeId : virtual EngineException { };
    struct ErrorInfoNodeIdTag { };
    typedef boost::error_info<ErrorInfoNodeIdTag, NodeId> ErrorInfoNodeId;

    class Node;

    class NodeMap
    {
        typedef std::vector<Node*> Nodes;

    public:
        typedef Nodes::const_reference const_reference;

        NodeMap(size_t maxNumNodes)
            : m_nodes(maxNumNodes, 0)
        { }

        void insert(Node* node);
        const_reference lookup(const NodeId& nodeId) const { return m_nodes.at(nodeId); }
        void release(const NodeId& nodeId);

    private:
        Nodes m_nodes;
    };

    class Group;

    // Mapped URIs needed by the realtime thread
    struct Uris
    {
        // atom
        const LV2_URID atom_Blank;
        const LV2_URID atom_Resource;
        const LV2_URID atom_Sequence;
        const LV2_URID atom_URID;
        // patch
        const LV2_URID patch_Insert;
        const LV2_URID patch_subject;
        const LV2_URID patch_body;
        // methcla
        const LV2_URID methcla_Group;
        const LV2_URID methcla_Synth;
        const LV2_URID methcla_Target;
        const LV2_URID methcla_addToHead;
        const LV2_URID methcla_addToTail;
        const LV2_URID methcla_plugin;

        Uris(LV2::URIDMap& uris)
            : atom_Blank    ( uris.map(LV2_ATOM__Blank) )
            , atom_Resource ( uris.map(LV2_ATOM__Resource) )
            , atom_Sequence ( uris.map(LV2_ATOM__Sequence) )
            , atom_URID     ( uris.map(LV2_ATOM__URID) )
            , patch_Insert  ( uris.map(LV2_PATCH_PREFIX "Insert") )
            , patch_subject ( uris.map(LV2_PATCH__subject) )
            , patch_body    ( uris.map(LV2_PATCH__body) )
            , methcla_Group ( uris.map(METHCLA_ENGINE_PREFIX "Group") )
            , methcla_Synth ( uris.map(METHCLA_ENGINE_PREFIX "Synth") )
            , methcla_Target ( uris.map(METHCLA_ENGINE_PREFIX "Target") )
            , methcla_addToHead ( uris.map(METHCLA_ENGINE_PREFIX "addToHead") )
            , methcla_addToTail ( uris.map(METHCLA_ENGINE_PREFIX "addToTail") )
            , methcla_plugin ( uris.map(METHCLA_ENGINE_PREFIX "plugin") )
        { }

        bool isObject(const LV2_Atom* atom) const
        {
            return (atom->type == atom_Blank)
                || (atom->type == atom_Resource);
        }
        const LV2_Atom_Object* toObject(const LV2_Atom* atom) const
        {
            return isObject(atom) ? reinterpret_cast<const LV2_Atom_Object*>(atom) : nullptr;
        }
        bool isNode(const LV2_Atom_Object* obj) const
        {
            return (obj->body.otype == methcla_Group) || (obj->body.otype == methcla_Synth);
        }
    };

    class Environment : public boost::noncopyable
    {
    public:
        struct Options
        {
            Options()
                : maxNumNodes(1024)
                , maxNumAudioBuses(128)
                , maxNumControlBuses(4096)
                , sampleRate(44100)
                , blockSize(64)
                , numHardwareInputChannels(2)
                , numHardwareOutputChannels(2)
            { }

            size_t maxNumNodes;
            size_t maxNumAudioBuses;
            size_t maxNumControlBuses;
            size_t sampleRate;
            size_t blockSize;
            size_t numHardwareInputChannels;
            size_t numHardwareOutputChannels;
        };

        Environment(PluginManager& pluginManager, const Options& options);
        ~Environment();

        const PluginManager& plugins() const { return m_plugins; }
        PluginManager& plugins() { return m_plugins; }

        Group* rootNode() { return m_rootNode; }

        size_t sampleRate() const { return m_sampleRate; }
        size_t blockSize() const { return m_blockSize; }

        //* Return audio bus with id.
        AudioBus* audioBus(const AudioBusId& id);
        //* Return external audio output bus at index.
        AudioBus& externalAudioOutput(size_t index);
        //* Return external audio input bus at index.
        AudioBus& externalAudioInput(size_t index);

        Memory::RTMemoryManager& rtMem() { return m_rtMem; }

        const Epoch& epoch() const { return m_epoch; }

        void process(size_t numFrames, sample_t** inputs, sample_t** outputs);

        // URIs and messages
        const LV2::URIDMap& uriMap() const { return plugins().uriMap(); }
        LV2::URIDMap& uriMap() { return plugins().uriMap(); }

        LV2_URID mapUri(const char* uri) { return uriMap().map(uri); }
        const char* unmapUri(LV2_URID urid) const { return uriMap().unmap(urid); }

        const Uris& uris() const { return m_uris; }

        // Request queue
        typedef Utility::MessageQueue<8192,8192> MessageQueue;
        void request(const LV2_Atom* msg, const MessageQueue::Respond& respond, void* data);

        // Worker thread
        typedef Utility::WorkerThread<8192,8192> Worker;

        LV2_Atom_Forge* prepare(const Worker::Perform& perform, void* data)
        {
            return m_worker.toWorker().prepare(perform, data);
        }
        void commit()
        {
            m_worker.toWorker().commit();
        }

    protected:
        void processRequests();
        void handleRequest(MessageQueue::Message& request);
        void handleMessageRequest(MessageQueue::Message& request, const LV2_Atom_Object* msg);
        void handleSequenceRequest(MessageQueue::Message& request, const LV2_Atom_Sequence* bdl);

    protected:
        friend class Node;
        ResourceMap<NodeId,Node>& nodes() { return m_nodes; }

    private:
        const size_t                        m_sampleRate;
        const size_t                        m_blockSize;
        Memory::RTMemoryManager             m_rtMem;
        PluginManager&                      m_plugins;
        ResourceMap<AudioBusId,AudioBus>    m_audioBuses;
        ResourceMap<AudioBusId,AudioBus>    m_freeAudioBuses;
        ResourceMap<NodeId,Node>            m_nodes;
        Group*                              m_rootNode;
        std::vector<ExternalAudioBus*>      m_audioInputChannels;
        std::vector<ExternalAudioBus*>      m_audioOutputChannels;
        Epoch                               m_epoch;
        MessageQueue                        m_requests;
        Worker                              m_worker;
        Uris                                m_uris;
    };

    class Engine : public IO::Client
    {
    public:
        Engine(Methcla::Plugin::Loader* pluginLoader, const boost::filesystem::path& lv2Directory);
        virtual ~Engine();

        virtual void configure(const IO::Driver& driver);
        virtual void process(size_t numFrames, sample_t** inputs, sample_t** outputs);
    
        Environment& env()
        {
            BOOST_ASSERT( m_env != 0 );
            return *m_env;
        }
        const Environment& env() const
        {
            BOOST_ASSERT( m_env != 0 );
            return *m_env;
        }

    private:
        Methcla::Plugin::Loader* m_pluginLoader;
        PluginManager            m_pluginManager;
        Environment*             m_env;
    };
}; };

#endif // METHCLA_AUDIO_ENGINE_HPP_INCLUDED